$ErrorActionPreference = 'Stop'
$path = Join-Path $PSScriptRoot 'config\game.toml'
$text = Get-Content -LiteralPath $path -Raw
if ($text -notmatch '(?m)^title\s*=\s*0x4D530053\s*$' -or $text -notmatch '(?m)^xdk\s*=\s*5659\s*$') {
    throw 'The Ghoulies overrides require title 4D530053 and XDK 5659.'
}
if ($text -notmatch '(?im)^\s*(?:at\s*=\s*)?0x0*7FFB0\b') {
    Add-Content -LiteralPath $path -Value ("`n" + (Get-Content -LiteralPath (Join-Path $PSScriptRoot 'config\extra-functions.inc') -Raw))
}
