# build-android.ps1 — 构建 Qt 6.8.3 Android APK（arm64-v8a / x86_64）。
#
# 前置条件：
#   1. Qt 6.8.3 for Android 及同版本 host Qt（含 androiddeployqt）。
#   2. Android SDK，以及 Qt 6.8.3 推荐的 NDK r26b（26.1.10909125）。
#   3. JDK 17、CMake、Ninja。
#
# 默认产物：artifacts\android\gdei-auth-client-android-<abi>-debug.apk
# 检测到 QT_ANDROID_KEYSTORE_PATH 等签名环境变量时按正式签名打包，产物名
# 标记 release-signed；否则 Release 包标记 release-unsigned（debug 兜底）。
param(
    [ValidateSet("arm64-v8a", "x86_64")]
    [string]$Abi = "arm64-v8a",
    [ValidateSet("Debug", "Release")]
    [string]$BuildType = "Debug",
    [string]$BuildDir = "build-android",
    [string]$DistDir = "artifacts/android",
    [string]$QT_ANDROID = $env:QT_ANDROID,
    [string]$QT_HOST_PATH = $env:QT_HOST_PATH,
    [string]$ANDROID_SDK_ROOT = $env:ANDROID_SDK_ROOT,
    [string]$ANDROID_NDK_ROOT = $env:ANDROID_NDK_ROOT,
    [string]$JAVA_HOME = $env:JAVA_HOME
)

$ErrorActionPreference = "Stop"
$SRC = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$ExpectedQtVersion = "6.8.3"
$ExpectedNdkVersion = "26.1.10909125"

function NeedDirectory($LiteralPath, $Name) {
    if ([string]::IsNullOrWhiteSpace($LiteralPath)) {
        throw "未设置 $Name，请通过同名参数或环境变量提供路径。"
    }
    if (-not (Test-Path -LiteralPath $LiteralPath -PathType Container)) {
        throw "缺少 $Name 目录：$LiteralPath"
    }
}

function NeedFile($LiteralPath, $Name) {
    if ([string]::IsNullOrWhiteSpace($LiteralPath)) {
        throw "未设置 $Name，请通过同名参数或环境变量提供路径。"
    }
    if (-not (Test-Path -LiteralPath $LiteralPath -PathType Leaf)) {
        throw "缺少 $Name 文件：$LiteralPath"
    }
}

function NeedCommand($Name) {
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($null -eq $command) {
        throw "找不到 $Name，请先安装并加入 PATH。"
    }
    return $command
}

function CheckQtVersion($QtRoot, $Name) {
    $versionFile = Join-Path $QtRoot "lib\cmake\Qt6\Qt6ConfigVersionImpl.cmake"
    NeedFile $versionFile "$Name Qt6ConfigVersionImpl.cmake"
    $versionText = [System.IO.File]::ReadAllText($versionFile)
    if ($versionText -notmatch "PACKAGE_VERSION\s+`"$ExpectedQtVersion`"") {
        throw "$Name 必须为 Qt $ExpectedQtVersion：$QtRoot"
    }
}

function ResolveFromSource($PathValue, $Name) {
    if ([string]::IsNullOrWhiteSpace($PathValue)) {
        throw "$Name 不能为空。"
    }
    if ([System.IO.Path]::IsPathRooted($PathValue)) {
        return [System.IO.Path]::GetFullPath($PathValue)
    }
    return [System.IO.Path]::GetFullPath((Join-Path $SRC $PathValue))
}

NeedDirectory $QT_ANDROID "QT_ANDROID"
NeedDirectory $QT_HOST_PATH "QT_HOST_PATH"
NeedDirectory $ANDROID_SDK_ROOT "ANDROID_SDK_ROOT"
NeedDirectory $ANDROID_NDK_ROOT "ANDROID_NDK_ROOT"
NeedDirectory $JAVA_HOME "JAVA_HOME"
CheckQtVersion $QT_ANDROID "QT_ANDROID"
CheckQtVersion $QT_HOST_PATH "QT_HOST_PATH"

$toolchain = Join-Path $QT_ANDROID "lib\cmake\Qt6\qt.toolchain.cmake"
NeedFile $toolchain "Qt Android toolchain"
NeedFile (Join-Path $QT_HOST_PATH "bin\androiddeployqt.exe") "host Qt androiddeployqt"

$cmake = NeedCommand "cmake"
$ninja = NeedCommand "ninja"
$javaExe = Join-Path $JAVA_HOME "bin\java.exe"
NeedFile $javaExe "JDK 17 java"
# java -version 正常写入 stderr；避免 Windows PowerShell 将其提升为终止错误。
$previousErrorActionPreference = $ErrorActionPreference
$ErrorActionPreference = "Continue"
$javaVersionOutput = @(& $javaExe -version 2>&1)
$javaExitCode = $LASTEXITCODE
$ErrorActionPreference = $previousErrorActionPreference
if ($javaExitCode -ne 0) {
    throw "无法执行 Java：$javaExe"
}
$javaVersionText = $javaVersionOutput -join [Environment]::NewLine
if ($javaVersionText -notmatch 'version "17(?:\.|")') {
    throw "JAVA_HOME 必须指向 JDK 17。当前输出：$javaVersionText"
}

$ndkProperties = Join-Path $ANDROID_NDK_ROOT "source.properties"
NeedFile $ndkProperties "Android NDK source.properties"
$ndkRevisionLine = Select-String -LiteralPath $ndkProperties -Pattern '^\s*Pkg.Revision\s*=' |
    Select-Object -First 1
if ($null -eq $ndkRevisionLine) {
    throw "无法从 $ndkProperties 读取 NDK 版本。"
}
$installedNdkVersion = ($ndkRevisionLine.Line -split '=', 2)[1].Trim()
if ($installedNdkVersion -ne $ExpectedNdkVersion) {
    throw "Qt $ExpectedQtVersion 官方 Android 库推荐 NDK r26b ($ExpectedNdkVersion)，当前为 $installedNdkVersion。"
}

$sdkBuildTools = Join-Path $ANDROID_SDK_ROOT "build-tools"
$sdkPlatforms = Join-Path $ANDROID_SDK_ROOT "platforms"
NeedDirectory $sdkBuildTools "Android SDK build-tools"
NeedDirectory $sdkPlatforms "Android SDK platforms"
$buildToolDirs = @(Get-ChildItem -LiteralPath $sdkBuildTools -Directory -ErrorAction Stop)
$platformDirs = @(Get-ChildItem -LiteralPath $sdkPlatforms -Directory -ErrorAction Stop)
if ($buildToolDirs.Count -eq 0) {
    throw "Android SDK 未安装任何 build-tools。"
}
if ($platformDirs.Count -eq 0) {
    throw "Android SDK 未安装任何 platform。"
}
$aaptFound = $false
foreach ($dir in $buildToolDirs) {
    if ((Test-Path -LiteralPath (Join-Path $dir.FullName "aapt2.exe") -PathType Leaf) -or
        (Test-Path -LiteralPath (Join-Path $dir.FullName "aapt.exe") -PathType Leaf)) {
        $aaptFound = $true
        break
    }
}
if (-not $aaptFound) {
    throw "Android SDK build-tools 中找不到 aapt2/aapt。"
}

$BuildDir = ResolveFromSource $BuildDir "BuildDir"
$DistDir = ResolveFromSource $DistDir "DistDir"
$env:QT_ANDROID = $QT_ANDROID
$env:QT_HOST_PATH = $QT_HOST_PATH
$env:ANDROID_SDK_ROOT = $ANDROID_SDK_ROOT
$env:ANDROID_NDK_ROOT = $ANDROID_NDK_ROOT
$env:JAVA_HOME = $JAVA_HOME
$env:Path = "$(Split-Path -Parent $javaExe);$env:Path"

# 检测签名环境变量：配置了 keystore 路径才启用正式签名，否则回退 debug/未签名兜底。
# Enable release signing only when a keystore path is provided via env; otherwise fall back.
$signEnabled = -not [string]::IsNullOrWhiteSpace($env:QT_ANDROID_KEYSTORE_PATH)
$signApk = if ($signEnabled) { "ON" } else { "OFF" }

Write-Host "==> 配置 Qt $ExpectedQtVersion Android $Abi $BuildType（签名=$signApk）"
$configureArgs = @(
    "-S", $SRC,
    "-B", $BuildDir,
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DCMAKE_MAKE_PROGRAM=$($ninja.Source)",
    "-DCMAKE_TOOLCHAIN_FILE=$toolchain",
    "-DQT_HOST_PATH=$QT_HOST_PATH",
    "-DQT_ANDROID_SIGN_APK=$signApk",
    "-DANDROID_ABI=$Abi",
    "-DQT_ANDROID_ABIS=$Abi",
    "-DANDROID_SDK_ROOT=$ANDROID_SDK_ROOT",
    "-DANDROID_NDK_ROOT=$ANDROID_NDK_ROOT",
    "-DANDROID_NDK_HOST=windows-x86_64",
    "-DJAVA_HOME=$JAVA_HOME"
)
& $cmake.Source @configureArgs
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure 失败，退出码 $LASTEXITCODE。"
}

Write-Host "==> 构建并打包 APK（apk target）"
& $cmake.Source --build $BuildDir --target apk --config $BuildType
if ($LASTEXITCODE -ne 0) {
    throw "APK 构建失败，退出码 $LASTEXITCODE。"
}

# Qt 6.8.3 的 apk target 将最终结果复制为目标名，不递归选择其他 APK。
$apkCandidates = @(
    (Join-Path $BuildDir "android-build\gdei-auth-client.apk"),
    (Join-Path $BuildDir "android-build-gdei-auth-client\gdei-auth-client.apk")
)
$apk = $null
foreach ($candidate in $apkCandidates) {
    if (Test-Path -LiteralPath $candidate -PathType Leaf) {
        $apk = Get-Item -LiteralPath $candidate
        break
    }
}
if ($null -eq $apk) {
    $expectedPaths = $apkCandidates -join [Environment]::NewLine
    throw "apk target 已结束，但未找到预期产物：`n$expectedPaths"
}

New-Item -ItemType Directory -Path $DistDir -Force | Out-Null
if ($BuildType -eq "Debug") { $packageType = "debug" }
elseif ($signEnabled)      { $packageType = "release-signed" }
else                        { $packageType = "release-unsigned" }
$destName = "gdei-auth-client-android-$Abi-$packageType.apk"
$destApk = Join-Path $DistDir $destName
Copy-Item -LiteralPath $apk.FullName -Destination $destApk -Force
$hash = Get-FileHash -LiteralPath $destApk -Algorithm SHA256
$hashText = "$($hash.Hash.ToLowerInvariant())  $destName"
$hashPath = "$destApk.sha256"
Set-Content -LiteralPath $hashPath -Value $hashText -Encoding ASCII

Write-Host ""
Write-Host "APK: $destApk"
Write-Host "SHA256: $($hash.Hash)"
Write-Host "校验文件: $hashPath"
if ($BuildType -eq "Release" -and -not $signEnabled) {
    Write-Warning "Release APK 未签名，需完成签名后才能安装或发布。"
} else {
    Write-Host "安装验证：adb install -r `"$destApk`""
}
