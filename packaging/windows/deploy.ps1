# packaging/windows/deploy.ps1
# Windows 部署打包：windeployqt 收集 Qt 运行库 / 插件 / QML，再归档 zip。
# Windows packaging: windeployqt collects Qt runtime, plugins and QML, then zips.
param(
    [string]$BuildDir = "build",
    [string]$DistDir  = "dist",
    [string]$QmlDir   = "qml"
)

$ErrorActionPreference = "Stop"

$exe = Get-ChildItem -Recurse $BuildDir -Filter "gdei-connect.exe" |
    Select-Object -First 1
if (-not $exe) {
    throw "gdei-connect.exe not found under $BuildDir"
}

# 由对应编译套件的 windeployqt 收集 platforms / qml / imageformats 等插件目录。
# Use the windeployqt matching the build kit to collect plugins and QML.
windeployqt --release --qmldir $QmlDir $exe.FullName

New-Item -ItemType Directory -Force $DistDir | Out-Null

$exeDir = $exe.DirectoryName
$staging = Join-Path $DistDir "gdei-connect"
if (Test-Path $staging) { Remove-Item $staging -Recurse -Force }
New-Item -ItemType Directory -Force $staging | Out-Null

# 递归拷贝 windeployqt 收集的 DLL 与 platforms / styles / imageformats / qml 等插件子目录。
# Recursively copy deployed DLLs and plugin subdirs (platforms/styles/imageformats/qml).
Get-ChildItem $exeDir -Force |
    Where-Object { $_.Extension -ne ".pdb" } |
    Copy-Item -Destination $staging -Recurse -Force

Compress-Archive -Path "$staging/*" -DestinationPath "$DistDir/gdei-connect-windows-x64.zip" -Force
