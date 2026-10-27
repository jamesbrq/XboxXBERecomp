$ErrorActionPreference = 'Stop'
$path = Join-Path $PSScriptRoot 'config\game.toml'
$text = Get-Content -LiteralPath $path -Raw
if ($text -notmatch '(?m)^title\s*=\s*0x4D530053\s*$' -or $text -notmatch '(?m)^xdk\s*=\s*5659\s*$') {
    throw 'The Ghoulies overrides require title 4D530053 and XDK 5659.'
}
if ($text -notmatch '(?im)^\s*(?:at\s*=\s*)?0x0*7FFB0\b') {
    Add-Content -LiteralPath $path -Value ("`n" + (Get-Content -LiteralPath (Join-Path $PSScriptRoot 'config\extra-functions.inc') -Raw))
}
if ($text -match '"\.ghmod"' -and $text -notmatch '(?im)^\s*at\s*=\s*0x0*5940A0\b') {
    $image = Join-Path $PSScriptRoot 'assets\default.xbe'
    if ((Get-FileHash -LiteralPath $image -Algorithm SHA1).Hash -ne '7193B60BC75240BDC0E75E184BBBB22B56E8E397') {
        throw 'The full mod callback overrides require the selected walktest XBE.'
    }
    Add-Content -LiteralPath $path -Value ("`n" + (Get-Content -LiteralPath (Join-Path $PSScriptRoot 'config\fullmod-functions.inc') -Raw))
}
