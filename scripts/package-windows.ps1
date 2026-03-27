param(
    [Parameter(Mandatory = $true)]
    [string]$Version,

    [Parameter(Mandatory = $false)]
    [string]$BuildDir = "build"
)

$ErrorActionPreference = "Stop"

$RootDir = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if (-not [System.IO.Path]::IsPathRooted($BuildDir)) {
    $BuildDir = Join-Path $RootDir $BuildDir
}

$ExePath = Join-Path $BuildDir "AMidiOrgan_artefacts/Release/AMidiOrgan.exe"
if (-not (Test-Path $ExePath)) {
    throw "Executable not found: $ExePath"
}

$DistDir = Join-Path $RootDir "dist"
$PackageRootName = "AMidiOrgan-$Version-windows-x64"
$PackageDir = Join-Path $DistDir $PackageRootName
$ZipPath = Join-Path $DistDir "$PackageRootName.zip"

if (Test-Path $PackageDir) { Remove-Item -Recurse -Force $PackageDir }
if (Test-Path $ZipPath) { Remove-Item -Force $ZipPath }

New-Item -ItemType Directory -Path $PackageDir -Force | Out-Null

Copy-Item -Path $ExePath -Destination (Join-Path $PackageDir "AMidiOrgan.exe") -Force
Copy-Item -Path (Join-Path $RootDir "docs") -Destination (Join-Path $PackageDir "docs") -Recurse -Force
Copy-Item -Path (Join-Path $RootDir "USER_MANUAL.md") -Destination (Join-Path $PackageDir "USER_MANUAL.md") -Force

$readmeUser = @"
AMidiOrgan packaged release

How to start:
1) Run AMidiOrgan.exe
2) If SmartScreen appears, click More info -> Run anyway (trusted source only)
3) Read USER_MANUAL.md for full setup and operating guidance

The docs\ folder is included for runtime seed data and reference assets.
"@
Set-Content -Path (Join-Path $PackageDir "README-USER.txt") -Value $readmeUser -Encoding UTF8

New-Item -ItemType Directory -Path $DistDir -Force | Out-Null
Compress-Archive -Path $PackageDir -DestinationPath $ZipPath -CompressionLevel Optimal -Force

Write-Host "Created package: $ZipPath"
