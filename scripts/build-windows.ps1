# build-windows.ps1 — 一键在 Windows 上构建并打包一个自包含、双击即跑的产物。
#
# 自动完成：
#   1) 在 %USERPROFILE%\.cache\gdei-toolchain 部署便携工具链
#      （embeddable Python，再用 pip 装 cmake / ninja / aqtinstall，如已存在则跳过）；
#   2) 优先复用本地已有的 Qt6 (MinGW) 与 MinGW 编译器；
#      缺失时回退：Qt 走 aqtinstall，编译器走 MSYS2 (tuna 镜像，速度快)；
#   3) CMake 配置 + Ninja 编译；
#   4) CMake 配置 + Ninja 编译（编译树保持干净，不在树内部署运行库）；
#   5) 调用 deploy.ps1：exe 复制到暂存目录，对暂存副本跑 windeployqt 收集
#      Qt 依赖 / 插件 / QML 模块，刷新编译器运行库，最后输出 zip。
#
# 产物： artifacts\windows\gdei-auth-client\gdei-auth-client.exe
#        整个文件夹拷到任何 Windows 都能直接双击运行（不依赖本机是否装了 Qt）。
#
# 用法（在项目根目录，PowerShell 5+）：
#   powershell -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1
# 可选： -QtVersion 6.8.3  -Arch win64_mingw  -BuildDir build  -DistDir artifacts/windows
param(
    [string]$QtVersion = "",   # 留空 = 自动选最新 6.x (仅在需 aqt 安装时生效)
    [string]$Arch      = "",   # 留空 = 自动选 mingw 套件 (仅在需 aqt 安装时生效)
    [string]$BuildDir  = "build",
    [string]$DistDir   = "artifacts/windows",
    [string]$Toolchain = ""
)

$ErrorActionPreference = "Stop"
$SRC = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
if (-not $Toolchain) { $Toolchain = Join-Path $env:USERPROFILE ".cache\gdei-toolchain" }
$dl = Join-Path $Toolchain "dl"
New-Item -ItemType Directory -Force $dl | Out-Null

function Die($m) { Write-Error $m; exit 1 }
function Run($cmd, $cmdArgs) {
    Write-Host ">> $cmd $($cmdArgs -join ' ')"
    & $cmd @cmdArgs
    if ($LASTEXITCODE -ne 0) { Die "命令失败 (exit $LASTEXITCODE): $cmd $($cmdArgs -join ' ')" }
}
function Dl($url, $out) {
    if (Test-Path $out) { Write-Host "已存在: $out"; return }
    Write-Host "下载 $url"
    Invoke-WebRequest -Uri $url -OutFile $out -UseBasicParsing
}
function Write-Utf8NoBom($path, $text) {
    [System.IO.File]::WriteAllText($path, $text, (New-Object System.Text.UTF8Encoding $false))
}

# ---------- 1. 便携 Python ----------
$pyVer = "3.12.8"
$pyDir = Join-Path $Toolchain "python"
$py = Join-Path $pyDir "python.exe"
if (-not (Test-Path $py)) {
    Dl "https://www.python.org/ftp/python/$pyVer/python-$pyVer-embed-amd64.zip" (Join-Path $dl "python.zip")
    if (-not (Test-Path $pyDir)) { New-Item -ItemType Directory -Force $pyDir | Out-Null }
    Expand-Archive -Path (Join-Path $dl "python.zip") -DestinationPath $pyDir -Force
}
if (-not (Test-Path $py)) { Die "python.exe 不存在: $py" }

# 启用 site + 让 embeddable Python 加载 site-packages（必须无 BOM，否则 python312.zip 路径损坏）
Write-Utf8NoBom (Join-Path $pyDir "python312._pth") `
    "python312.zip`r`n.`r`n`r`n# Uncomment to run site.main() automatically`r`nimport site`r`nLib\site-packages`r`n"

if (-not (Test-Path (Join-Path $pyDir "Scripts\pip.exe"))) {
    Dl "https://bootstrap.pypa.io/get-pip.py" (Join-Path $dl "get-pip.py")
    Run $py @((Join-Path $dl "get-pip.py"), "--no-warn-script-location")
}

# ---------- 2. CMake / Ninja / aqtinstall（pip 安装，稳定不依赖 GitHub 大文件） ----------
Run $py @("-m", "pip", "install", "--quiet", "--no-warn-script-location", "cmake", "ninja", "aqtinstall")
$cmake = Get-Item (Join-Path $pyDir "Scripts\cmake.exe") -ErrorAction SilentlyContinue
$ninja = Get-Item (Join-Path $pyDir "Scripts\ninja.exe") -ErrorAction SilentlyContinue
if (-not $cmake) { Die "cmake.exe 未安装到 $pyDir\Scripts" }
if (-not $ninja) { Die "ninja.exe 未安装到 $pyDir\Scripts" }
$cmakeDir = $cmake.DirectoryName
$ninjaDir = $ninja.DirectoryName
Write-Host "工具链就绪: python=$py  cmake=$($cmake.FullName)  ninja=$($ninja.FullName)"

# aqt 配置：其下载 worker 从配置文件读 response_timeout（默认仅 10 秒），
# 大文件在慢镜像上一读停顿就被掐断 -> 调到 1200s 并加重试，保证 Qt 大包能下完。
$aqtCfg = Join-Path $Toolchain "aqt-settings.ini"
Write-Utf8NoBom $aqtCfg @"
[requests]
connection_timeout = 30
response_timeout = 1200
max_retries_on_connection_error = 10
max_retries_on_checksum_error = 10
max_retries_to_retrieve_hash = 10
retry_backoff = 2
"@

# ---------- 3. 探测已有 Qt6 (MinGW)；没有再用 aqt 安装 ----------
function Find-QtMingw {
    # 递归找 mingw 套件的 qmake.exe，返回套件根目录（bin 的父目录）
    $roots = @(
        (Join-Path $Toolchain "mirrortest"),   # 镜像缓存（如 berkeley 下载的 Qt）
        (Join-Path $Toolchain "Qt"),
        (Join-Path $env:USERPROFILE "Qt"),
        "C:\Qt"
    ) | Where-Object { Test-Path $_ }
    foreach ($r in $roots) {
        $q = Get-ChildItem $r -Recurse -Filter "qmake.exe" -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match '\\mingw(_64)?\\bin\\qmake\.exe$' } |
            Select-Object -First 1
        if ($q) { return $q.Directory.Parent.FullName }
    }
    return $null
}

$qt = Find-QtMingw
if ($qt) {
    Write-Host "复用已有 Qt6 mingw 套件: $qt"
} else {
    Write-Host "本地未发现 Qt6 mingw 套件，回退到 aqtinstall 安装（需可访问 Qt 镜像）..."
    if (-not $QtVersion) {
        $versions = & $py -m aqt list-qt windows desktop 2>$null
        $QtVersion = ($versions -split '\s+' | Where-Object { $_ -match '^6\.\d+\.\d+$' } |
            Sort-Object { [version]$_ } -Descending | Select-Object -First 1)
        if (-not $QtVersion) { Die "无法列出 Qt 版本（aqt 不可用且本地无 Qt）" }
        Write-Host "自动选择 Qt 版本: $QtVersion"
    }
    if (-not $Arch) {
        $arches = & $py -m aqt list-qt windows desktop --arch $QtVersion 2>$null
        $Arch = ($arches -split '\s+' | Where-Object { $_ -match '^win64_mingw$' } | Select-Object -First 1)
        if (-not $Arch) { $Arch = ($arches -split '\s+' | Where-Object { $_ -match 'mingw' } | Select-Object -First 1) }
        if (-not $Arch) { Die "未找到 mingw 套件 (Qt $QtVersion)" }
        Write-Host "自动选择套件: $Arch"
    }
    Run $py @("-m", "aqt", "-c", $aqtCfg, "install-qt", "windows", "desktop", $QtVersion, $Arch, "-O", (Join-Path $Toolchain "Qt"), "--modules", "qt5compat")
    $qt = Find-QtMingw
}
if (-not $qt) { Die "未找到 Qt6 mingw 套件" }

# Core5Compat 提供 GBK QTextCodec；复用旧 Qt 时只补装该模块，不重下基础套件。
$core5CompatConfig = Join-Path $qt "lib\cmake\Qt6Core5Compat\Qt6Core5CompatConfig.cmake"
if (-not (Test-Path $core5CompatConfig)) {
    $qtVersionDir = Split-Path -Parent $qt
    $installedQtVersion = Split-Path -Leaf $qtVersionDir
    $qtArchFolder = Split-Path -Leaf $qt
    $installedQtArch = if ($qtArchFolder -eq "mingw_64") { "win64_mingw" } else { "" }
    if (-not $installedQtArch) { Die "无法识别 Qt 套件架构，请手动安装 qt5compat: $qt" }
    $qtInstallRoot = Split-Path -Parent $qtVersionDir
    Write-Host "为现有 Qt $installedQtVersion 补装 qt5compat..."
    Run $py @("-m", "aqt", "-c", $aqtCfg, "install-qt", "windows", "desktop",
        $installedQtVersion, $installedQtArch, "-O", $qtInstallRoot,
        "--noarchives", "--modules", "qt5compat")
}
if (-not (Test-Path $core5CompatConfig)) { Die "Qt6Core5Compat 未安装，无法提供 GBK 编码支持" }
Write-Host "Qt 套件根: $qt"

# ---------- 3b. 探测已有 MinGW 编译器；没有则用 MSYS2 (tuna 镜像) 安装 ----------
function Find-Gpp {
    $cands = @(
        (Join-Path $Toolchain "msys64\mingw64\bin\g++.exe"),
        (Join-Path $Toolchain "Qt\Tools")
    )
    foreach ($c in $cands) {
        if (Test-Path $c) {
            if ($c -like "*g++.exe") { return (Get-Item $c).DirectoryName }
            $g = Get-ChildItem $c -Recurse -Filter "g++.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($g) { return $g.DirectoryName }
        }
    }
    $cmd = Get-Command g++.exe -ErrorAction SilentlyContinue
    if ($cmd) { return (Split-Path $cmd.Source) }
    return $null
}

$mingwBin = Find-Gpp
if (-not $mingwBin) {
    Write-Host "未发现 g++.exe，用 MSYS2 (tuna 镜像) 安装 mingw-w64-x86_64-gcc ..."
    $msys = Join-Path $Toolchain "msys64"
    if (-not (Test-Path (Join-Path $msys "usr\bin\pacman.exe"))) {
        $msysTar = Join-Path $dl "msys2-base.tar.xz"
        Dl "https://mirrors.tuna.tsinghua.edu.cn/msys2/distrib/x86_64/msys2-base-x86_64-latest.tar.xz" $msysTar
        Write-Host "解压 MSYS2 base (Python tarfile 处理 .tar.xz，Windows tar 缺 xz 过滤)..."
        & $py -c "import tarfile,sys; tarfile.open(sys.argv[1]).extractall(sys.argv[2])" $msysTar $Toolchain
    }
    # pacman 镜像切到 tuna（速度快）
    Write-Utf8NoBom (Join-Path $msys "etc\pacman.d\mirrorlist.mingw64") 'Server = https://mirrors.tuna.tsinghua.edu.cn/msys2/mingw/x86_64/
'
    Write-Utf8NoBom (Join-Path $msys "etc\pacman.d\mirrorlist.msys") 'Server = https://mirrors.tuna.tsinghua.edu.cn/msys2/msys/$arch/
'
    $pacman = Join-Path $msys "usr\bin\pacman.exe"
    Run $pacman @("-Sy", "--noconfirm")
    Run $pacman @("-S", "--noconfirm", "--needed", "mingw-w64-x86_64-gcc")
    $mingwBin = Join-Path $msys "mingw64\bin"
}
if (-not (Test-Path (Join-Path $mingwBin "g++.exe"))) { Die "g++.exe 仍不存在: $mingwBin" }
Write-Host "MinGW bin: $mingwBin"

# ---------- 4. 配置环境并编译 ----------
$env:PATH = "$cmakeDir;$ninjaDir;$mingwBin;$qt\bin;$env:PATH"

Write-Host "==> 配置 CMake"
Run $cmake.FullName @("-S", $SRC, "-B", $BuildDir, "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_PREFIX_PATH=$qt",
    "-DCMAKE_C_COMPILER=gcc",
    "-DCMAKE_CXX_COMPILER=g++")

Write-Host "==> 编译（POST_BUILD 会自动运行 windeployqt 打包全部依赖）"
Run $cmake.FullName @("--build", $BuildDir, "--config", "Release")

# ---------- 5. 暂存打包：exe 复制到暂存目录，对暂存副本跑 windeployqt，出 zip ----------
# 编译树内不再部署 Qt 运行库（POST_BUILD 自动打包已移除）；否则 DLL 倾倒进 build
# 目录会污染后续编译器子进程的 DLL 加载（MinGW g++ 静默退出）。捆绑统一交由
# packaging/windows/deploy.ps1 在暂存目录完成，与 CI release.yml 流程一致。
$deployScript = Join-Path $SRC "packaging\windows\deploy.ps1"
& $deployScript -BuildDir $BuildDir -DistDir $DistDir -QmlDir (Join-Path $SRC "qml")
if ($LASTEXITCODE -ne 0) { Die "打包脚本 deploy.ps1 失败 (exit $LASTEXITCODE)" }

$staging = Join-Path $DistDir "gdei-auth-client"
$zip     = Join-Path $DistDir "gdei-auth-client-windows-x64.zip"

Write-Host ""
Write-Host "==================== 完成 ===================="
Write-Host "自包含目录 : $staging"
Write-Host "  -> 双击其中的 gdei-auth-client.exe 即可运行（已打包全部 Qt 依赖）"
Write-Host "分发包 zip : $zip"
Write-Host "  -> 拷到任何 Windows 机器解压即用"

# 显式归零退出码，避免调用方把成功构建误判为失败。
exit 0
