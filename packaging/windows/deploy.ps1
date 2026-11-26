# packaging/windows/deploy.ps1
# Windows 部署打包：先把构建产物复制到暂存目录，再由 windeployqt 收集 Qt 运行库 /
# 插件 / QML 到暂存目录，最后归档 zip。windeployqt 不得作用于编译树内的 exe，
# 否则运行时 DLL 会被倾倒进 build 目录，污染后续编译器子进程的 DLL 加载。
# Windows packaging: copy the build output to a staging dir first, then let windeployqt
# collect Qt runtime / plugins / QML into that staging dir, and finally zip it.
# windeployqt must NOT target the exe inside the build tree; otherwise runtime DLLs are
# dumped into the build directory and break the compiler subprocess DLL resolution.
param(
    [string]$BuildDir = "build",
    [string]$DistDir  = "artifacts/windows",
    [string]$QmlDir   = "qml"
)

$ErrorActionPreference = "Stop"

$exe = Get-ChildItem -Recurse $BuildDir -Filter "gdei-auth-client.exe" |
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

# 由对应编译套件的 windeployqt 收集 platforms / qml / imageformats 等插件目录。
# Use the windeployqt matching the build kit to collect plugins and QML.
windeployqt --release --qmldir $QmlDir $stagingExe

Compress-Archive -Path "$staging/*" -DestinationPath "$DistDir/gdei-auth-client-windows-x64.zip" -Force
