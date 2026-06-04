param(
    [string]$PackageName = "GnssCyberpunkHost",
    [string]$MakePath = "C:\msys64\usr\bin\make.exe"
)

$ErrorActionPreference = "Stop"

function Get-ProjectRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Assert-InsideProject {
    param(
        [string]$Path,
        [string]$ProjectRoot
    )

    $resolved = [System.IO.Path]::GetFullPath($Path)
    $root = [System.IO.Path]::GetFullPath($ProjectRoot)
    if (-not $resolved.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to modify path outside project: $resolved"
    }
}

function Get-DllDependencies {
    param([string]$BinaryPath)

    $lines = & objdump -p $BinaryPath 2>$null | Select-String "DLL Name:"
    foreach ($line in $lines) {
        $text = $line.ToString().Trim()
        $name = $text -replace "^DLL Name:\s*", ""
        if ($name) {
            $name.Trim()
        }
    }
}

function Is-SystemDll {
    param([string]$Name)

    $upper = $Name.ToUpperInvariant()
    if ($upper.StartsWith("API-MS-")) {
        return $true
    }

    $systemDlls = @(
        "ADVAPI32.DLL", "AUTHZ.DLL", "BCRYPT.DLL", "COMDLG32.DLL", "CRYPT32.DLL",
        "CRYPTSP.DLL", "D3D9.DLL", "D3D11.DLL", "D3D12.DLL", "DNSAPI.DLL",
        "DWMAPI.DLL", "DWRITE.DLL", "DXGI.DLL", "GDI32.DLL",
        "IMM32.DLL", "IPHLPAPI.DLL", "KERNEL32.DLL", "MPR.DLL", "NETAPI32.DLL",
        "NCRYPT.DLL", "NTDLL.DLL", "OLE32.DLL", "OLEAUT32.DLL", "RPCRT4.DLL",
        "SECUR32.DLL", "SETUPAPI.DLL", "SHCORE.DLL", "SHELL32.DLL", "SHLWAPI.DLL",
        "USER32.DLL", "USERENV.DLL", "USP10.DLL", "UXTHEME.DLL", "VERSION.DLL",
        "WINHTTP.DLL", "WINMM.DLL", "WS2_32.DLL", "WTSAPI32.DLL"
    )
    return $systemDlls -contains $upper
}

function Find-DependencySource {
    param(
        [string]$Name,
        [string[]]$SearchDirs
    )

    foreach ($dir in $SearchDirs) {
        if (-not $dir -or -not (Test-Path $dir)) {
            continue
        }
        $candidate = Join-Path $dir $Name
        if (Test-Path $candidate) {
            return $candidate
        }
    }
    return $null
}

$projectRoot = Get-ProjectRoot
Set-Location $projectRoot

$qtBins = (& qmake -query QT_INSTALL_BINS).Trim()
if (-not $qtBins) {
    throw "Cannot locate Qt bin directory with qmake -query QT_INSTALL_BINS."
}

if (-not (Test-Path $MakePath)) {
    throw "Cannot find make executable: $MakePath"
}

Write-Host "Building release..."
& qmake "$PackageName.pro"
& $MakePath release

$distRoot = Join-Path $projectRoot "dist"
$packageDir = Join-Path $distRoot $PackageName
$zipPath = Join-Path $distRoot "$PackageName-portable.zip"
Assert-InsideProject -Path $distRoot -ProjectRoot $projectRoot
Assert-InsideProject -Path $packageDir -ProjectRoot $projectRoot

if (Test-Path $packageDir) {
    Remove-Item -LiteralPath $packageDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $packageDir | Out-Null

$exeSource = Join-Path $projectRoot "release\$PackageName.exe"
$exeDest = Join-Path $packageDir "$PackageName.exe"
Copy-Item -LiteralPath $exeSource -Destination $exeDest -Force

Write-Host "Deploying Qt runtime..."
& windeployqt $exeDest

$tileSource = Join-Path $projectRoot "release\tiles"
if (Test-Path $tileSource) {
    Copy-Item -LiteralPath $tileSource -Destination (Join-Path $packageDir "tiles") -Recurse -Force
}

$searchDirs = @(
    $packageDir,
    $qtBins,
    "C:\msys64\ucrt64\bin",
    "C:\msys64\usr\bin"
)

Write-Host "Copying non-system DLL dependencies..."
$changed = $true
while ($changed) {
    $changed = $false
    $binaries = Get-ChildItem -Path $packageDir -Recurse -File |
        Where-Object { $_.Extension -ieq ".exe" -or $_.Extension -ieq ".dll" }

    foreach ($binary in $binaries) {
        $deps = Get-DllDependencies -BinaryPath $binary.FullName
        foreach ($dep in $deps) {
            if (Is-SystemDll -Name $dep) {
                continue
            }

            $alreadyPresent = Get-ChildItem -Path $packageDir -Recurse -File -Filter $dep -ErrorAction SilentlyContinue |
                Select-Object -First 1
            if ($alreadyPresent) {
                continue
            }

            $source = Find-DependencySource -Name $dep -SearchDirs $searchDirs
            if ($source) {
                Copy-Item -LiteralPath $source -Destination (Join-Path $packageDir $dep) -Force
                Write-Host "  copied $dep"
                $changed = $true
            } else {
                Write-Warning "Missing non-system dependency: $dep required by $($binary.Name)"
            }
        }
    }
}

if (Test-Path $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}
Compress-Archive -Path (Join-Path $packageDir "*") -DestinationPath $zipPath -Force

Write-Host "Portable package ready:"
Write-Host "  $packageDir"
Write-Host "  $zipPath"
