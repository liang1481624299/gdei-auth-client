# packaging/windows/deploy.ps1
# Windows 部署打包：先把构建产物复制到暂存目录，再由 windeployqt 收集 Qt 运行库 /
# 插件 / QML 到暂存目录，兜底补齐编译器运行时，最后归档 zip。windeployqt 不得作用于
# 编译树内的 exe，否则运行时 DLL 会被倾倒进 build 目录，污染后续编译器子进程的 DLL 加载。
#
# Windows packaging: copy the build output to a staging dir first, let windeployqt collect
# Qt runtime / plugins / QML into it, back-fill the compiler runtime, then zip. windeployqt
# must NOT target the in-tree exe; otherwise runtime DLLs are dumped into the build tree and
# break the compiler subprocess DLL resolution.
#
# 产物目标：绿色自包含目录，解压后双击 exe 即可在干净 Windows（未装 Qt、未单独装
# VC++ redist 的 x64 机器）运行。
# Target: a self-contained green folder that runs by double-click on a clean x64 Windows
# machine without Qt or a separately installed VC++ redistributable.

param(
    [string]$BuildDir = "build",
    [string]$DistDir  = "artifacts/windows",
    [string]$QmlDir   = "qml"
)

$ErrorActionPreference = "Stop"

# ---- 1. 定位构建产物 / Locate the build output ----
$exe = Get-ChildItem -Recurse $BuildDir -Filter "gdei-auth-client.exe" -ErrorAction SilentlyContinue |
    Select-Object -First 1
if (-not $exe) {
    throw "gdei-auth-client.exe not found under $BuildDir"
}

$staging = Join-Path $DistDir "gdei-auth-client"
if (Test-Path $staging) { Remove-Item $staging -Recurse -Force }
New-Item -ItemType Directory -Force $staging | Out-Null

# 先把 exe 复制到暂存目录，再对暂存副本做收集，编译树保持干净。
# Copy the exe into staging first, then collect against that staging copy,
# leaving the build tree untouched.
$stagingExe = Join-Path $staging $exe.Name
Copy-Item $exe.FullName $stagingExe -Force

# ---- 2. windeployqt 收集 Qt 运行库 / 插件 / QML / 编译器运行时 ----
# Use the windeployqt matching the build kit to collect plugins, QML and the compiler runtime.
windeployqt --release --compiler-runtime --qmldir $QmlDir $stagingExe

# ---- 3. MSVC CRT 兜底补齐 / Back-fill MSVC CRT ----
# 部分套件下 windeployqt 的 --compiler-runtime 不会把 vcruntime/msvcp 拷进目录；干净机器
# 缺这些 DLL 时双击会直接起不来（无窗口、无明显弹窗）。这里从 VC Redist / VC Tools
# 目录再补一轮；MinGW 链路下 VCINSTALLDIR 为空，整段自动跳过。
# Some kits do not copy vcruntime/msvcp into the folder even with --compiler-runtime; on a
# clean machine the exe then fails to start. Back-fill from the VC Redist / VC Tools dirs.
# Under a MinGW toolchain VCINSTALLDIR is empty and this block is skipped.
$crtNames = @(
    "vcruntime140.dll",
    "vcruntime140_1.dll",
    "msvcp140.dll",
    "msvcp140_1.dll",
    "msvcp140_2.dll",
    "concrt140.dll"
)

$crtSources = @()
# 收集候选 VC 安装根：优先环境变量，再扫 VS2022/2019 标准安装目录。
# Gather candidate VC roots: env var first, then standard VS2022/2019 install paths.
$vcRoots = @()
if ($env:VCINSTALLDIR) { $vcRoots += $env:VCINSTALLDIR }
$vsRoots = @(
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC"
)
foreach ($vr in $vsRoots) { if (Test-Path $vr) { $vcRoots += $vr } }

foreach ($vc in $vcRoots) {
    # Redist\MSVC\<version>\x64\Microsoft.VC143.CRT\*.dll （合并 CRT，首选）
    $crtSources += Get-ChildItem -Path (Join-Path $vc "Redist\MSVC") `
        -Recurse -Directory -Filter "Microsoft.VC143.CRT" -ErrorAction SilentlyContinue |
        ForEach-Object { $_.FullName }
    # 兜底：VC Tools 头/库同级 bin\Hostx64\x64
    $crtSources += (Join-Path $vc "bin\Hostx64\x64")
}
$crtSources = $crtSources | Where-Object { $_ -and (Test-Path $_) } | Select-Object -Unique

foreach ($name in $crtNames) {
    if (Test-Path (Join-Path $staging $name)) { continue }
    foreach ($src in $crtSources) {
        if (-not $src) { continue }
        $candidate = Join-Path $src $name
        if (Test-Path $candidate) {
            Copy-Item $candidate $staging -Force
            Write-Host "CRT back-fill: $name <- $src"
            break
        }
    }
}

# ---- 4. 移除未使用的 SQL 驱动 / Drop unused SQL drivers ----
# 本项目不连接任何数据库（配置走本地 QSettings）。qsqlpsql.dll 依赖第三方 libpq.dll、
# qsqlodbc.dll 依赖 ODBC 驱动管理器；打包它们会留下悬空依赖。仅保留 qsqlite.dll。
# The app keeps no database (config lives in local QSettings). qsqlpsql needs the third-party
# libpq.dll and qsqlodbc needs the ODBC manager; drop them to avoid dangling deps. Keep qsqlite.
$unusedSql = @("qsqlpsql.dll", "qsqlodbc.dll")
foreach ($s in $unusedSql) {
    $p = Join-Path $staging "sqldrivers\$s"
    if (Test-Path $p) { Remove-Item $p -Force }
}

# ---- 5. 完整性校验 / Integrity check ----
# 关键启动依赖必须就位；缺任一项直接失败，而不是放出一个在别的电脑打不开的包。
# Every critical startup dependency must be present; fail early instead of shipping a
# package that will not launch on another machine.
$required = @(
    $stagingExe,
    (Join-Path $staging "platforms\qwindows.dll"),
    (Join-Path $staging "Qt6Core.dll"),
    (Join-Path $staging "Qt6Gui.dll"),
    (Join-Path $staging "Qt6Qml.dll"),
    (Join-Path $staging "Qt6Quick.dll"),
    (Join-Path $staging "Qt6Network.dll")
)
foreach ($r in $required) {
    if (-not (Test-Path $r)) {
        throw "Missing required runtime: $r"
    }
}

# 运行时二选一：MSVC CRT（msvcp140 / vcruntime140）或 MinGW 运行时（libgcc / libstdc++）。
# Either the MSVC CRT or the MinGW runtime must be deployed; otherwise a clean machine fails to start.
$hasMsvc  = (Test-Path (Join-Path $staging "msvcp140.dll")) -or (Test-Path (Join-Path $staging "vcruntime140.dll"))
$hasMingw = (Test-Path (Join-Path $staging "libgcc_s_seh-1.dll")) -or (Test-Path (Join-Path $staging "libstdc++-6.dll"))
if (-not $hasMsvc -and -not $hasMingw) {
    throw "Neither MSVC CRT nor MinGW runtime DLLs were deployed; the app will not start on a clean machine."
}

# 诊断：打印随包部署的运行时，便于在 CI 日志确认自包含完整性。
# Diagnostics: list the runtime DLLs bundled, to confirm self-containment in the CI log.
$runtimeTag = if ($hasMsvc) { "MSVC CRT" } else { "MinGW runtime" }
$runtimeDlls = Get-ChildItem $staging -File -Filter "*.dll" |
    Where-Object { $_.Name -match "^(msvcp|vcruntime|concrt|libgcc|libstdc|libwinpthread)" } |
    ForEach-Object { $_.Name }
Write-Host "Runtime bundled ($runtimeTag): $($runtimeDlls -join ', ')"
Write-Host "Staging file count: $((Get-ChildItem $staging -Recurse -File).Count)"

# ---- 6. 归档 / Zip ----
Compress-Archive -Path "$staging/*" -DestinationPath "$DistDir/gdei-auth-client-windows-x64.zip" -Force
