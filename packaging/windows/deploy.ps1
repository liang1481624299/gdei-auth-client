# packaging/windows/deploy.ps1
# Windows 部署打包：windeployqt 收集 Qt 运行库 / 插件 / QML，再归档 zip。
# Windows packaging: windeployqt collects Qt runtime, plugins and QML, then zips.
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

# 由对应编译套件的 windeployqt 收集 platforms / qml / imageformats 等插件目录。
# Use the windeployqt matching the build kit to collect plugins and QML.
windeployqt --release --qmldir $QmlDir $exe.FullName

New-Item -ItemType Directory -Force $DistDir | Out-Null

$exeDir = $exe.DirectoryName
$staging = Join-Path $DistDir "gdei-auth-client"
if (Test-Path $staging) { Remove-Item $staging -Recurse -Force }
New-Item -ItemType Directory -Force $staging | Out-Null

# 递归拷贝 windeployqt 收集的 DLL 与 platforms / styles / imageformats / qml 等插件子目录。
# Recursively copy deployed DLLs and plugin subdirs (platforms/styles/imageformats/qml).
# 排除 CMake/Ninja/Qt 构建中间产物目录与源码中间文件；项目 QML、图标、字体已内嵌 qrc，不拷磁盘副本。
# Exclude CMake/Ninja/Qt build intermediates; app QML/icons/fonts are baked into the qrc.
$excludeDirs = @(
    ".qt", ".rcc", "CMakeFiles", "gdei-auth-client_autogen", "GdeiConnect",
    "meta_types", "qmltooling", "qmltypes"
)
$excludeExt = @(".cpp", ".a", ".ninja", ".qrc", ".qmltypes")
$excludeFiles = @(
    ".ninja_deps", ".ninja_log", "build.ninja", "cmake_install.cmake",
    "CMakeCache.txt", "CPackConfig.cmake", "CPackSourceConfig.cmake"
)

Get-ChildItem $exeDir -Force |
    Where-Object {
        if ($_.PSIsContainer) {
            $excludeDirs -notcontains $_.Name
        } else {
            ($excludeExt -notcontains $_.Extension) -and
            ($excludeFiles -notcontains $_.Name)
        }
    } |
    Copy-Item -Destination $staging -Recurse -Force

Compress-Archive -Path "$staging/*" -DestinationPath "$DistDir/gdei-auth-client-windows-x64.zip" -Force
