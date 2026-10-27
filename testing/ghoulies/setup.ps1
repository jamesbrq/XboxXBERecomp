param([Parameter(Mandatory = $true)][string]$GameFiles)
$ErrorActionPreference = 'Stop'
$source = (Resolve-Path -LiteralPath $GameFiles).Path
if (!(Test-Path -LiteralPath (Join-Path $source 'default.xbe'))) {
    throw 'GameFiles must be the extracted game folder containing default.xbe.'
}
$assets = Join-Path $PSScriptRoot 'assets'
if (Test-Path -LiteralPath $assets) {
    throw "Assets already exist at $assets. Keep them or choose a fresh test checkout."
}
# A directory junction avoids copying the disc. Build and run outputs live in this project.
New-Item -ItemType Junction -Path $assets -Target $source | Out-Null
if (!(Test-Path -LiteralPath (Join-Path $PSScriptRoot 'config\game.toml'))) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'config\game.example.toml') -Destination (Join-Path $PSScriptRoot 'config\game.toml')
}
Write-Host "Game files linked. Run $PSScriptRoot\build.bat next."
