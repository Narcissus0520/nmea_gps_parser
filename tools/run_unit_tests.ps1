param(
    [string]$QmakePath = "C:\msys64\ucrt64\bin\qmake.exe",
    [string]$GxxPath = "C:\msys64\ucrt64\bin\g++.exe"
)

$ErrorActionPreference = "Stop"

function Get-ProjectRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

$projectRoot = Get-ProjectRoot
$qtBins = (& $QmakePath -query QT_INSTALL_BINS).Trim()
$qtHostTools = (& $QmakePath -query QT_HOST_LIBEXECS).Trim()
$qtHeaders = (& $QmakePath -query QT_INSTALL_HEADERS).Trim()
$qtLibs = (& $QmakePath -query QT_INSTALL_LIBS).Trim()

if (-not (Test-Path $GxxPath)) {
    throw "Cannot find GCC executable: $GxxPath"
}

$buildDir = Join-Path $projectRoot "tests\unit_tests\build"
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

$mocPath = Join-Path $qtHostTools "moc.exe"
$mocOutput = Join-Path $buildDir "moc_nmea_parser.cpp"
$exePath = Join-Path $buildDir "unit_tests.exe"

& $mocPath `
    "-DQT_CORE_LIB" `
    "-I$projectRoot" `
    "-I$qtHeaders" `
    "-I$(Join-Path $qtHeaders "QtCore")" `
    (Join-Path $projectRoot "src\nmea\nmea_parser.h") `
    "-o" $mocOutput

$sources = @(
    (Join-Path $projectRoot "tests\unit_tests\unit_tests.cpp"),
    (Join-Path $projectRoot "src\serial\command_encoder.cpp"),
    (Join-Path $projectRoot "src\nmea\nmea_parser.cpp"),
    $mocOutput
)

$compileArgs = @(
    "-std=gnu++17",
    "-Wall",
    "-Wextra",
    "-Wno-sfinae-incomplete",
    "-DQT_CORE_LIB",
    "-DQT_NO_DEBUG",
    "-I$projectRoot",
    "-I$qtHeaders",
    "-I$(Join-Path $qtHeaders "QtCore")",
    "-o",
    $exePath
) + $sources + @(
    (Join-Path $qtLibs "libQt6Core.dll.a")
)

& $GxxPath @compileArgs

$oldPath = $env:Path
try {
    $env:Path = "$qtBins;$(Split-Path $GxxPath);$env:Path"
    & $exePath
    if ($LASTEXITCODE -ne 0) {
        throw "Unit tests failed with exit code $LASTEXITCODE"
    }
} finally {
    $env:Path = $oldPath
}
