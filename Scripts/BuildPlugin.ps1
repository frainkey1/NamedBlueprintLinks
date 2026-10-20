[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [string]$OutputPath,
    [string]$LogPath
)
$ErrorActionPreference = 'Stop'
$pluginRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$descriptor = Join-Path $pluginRoot 'NamedBlueprintLinks.uplugin'
$uat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
if (-not (Test-Path -LiteralPath $uat -PathType Leaf)) { throw "RunUAT.bat not found under $EngineRoot" }
if (-not $OutputPath) {
    $OutputPath = Join-Path ([System.IO.Path]::GetTempPath()) ('NBL-Build-' + [guid]::NewGuid().ToString('N'))
}
$OutputPath = [System.IO.Path]::GetFullPath($OutputPath)
# UAT may clean its package destination. Only use a new, separate directory.
if ((Test-Path -LiteralPath $OutputPath) -or
    $OutputPath.TrimEnd('\','/').Equals($pluginRoot,[System.StringComparison]::OrdinalIgnoreCase) -or
    $OutputPath.StartsWith($pluginRoot + [System.IO.Path]::DirectorySeparatorChar,[System.StringComparison]::OrdinalIgnoreCase)) {
    throw 'OutputPath must be a new directory outside the source plugin folder.'
}
if (-not $LogPath) { $LogPath = Join-Path $pluginRoot 'Saved\NBL-Build.log' }
$LogPath = [System.IO.Path]::GetFullPath($LogPath)
New-Item -ItemType Directory -Force -Path (Split-Path $LogPath -Parent) | Out-Null
Write-Host "Building source prototype. Package destination: $OutputPath"
& $uat 'BuildPlugin' "-Plugin=$descriptor" "-Package=$OutputPath" '-TargetPlatforms=Win64' '-StrictIncludes' 2>&1 |
    Tee-Object -FilePath $LogPath
$buildExit = $LASTEXITCODE
if ($buildExit -ne 0) { throw "Unreal build failed with exit $buildExit. Log: $LogPath" }
Write-Host "Build finished. Package: $OutputPath"
Write-Host "Build log: $LogPath"
Write-Host 'This proves compilation only; run the compiler automation before using routing in a project.'
