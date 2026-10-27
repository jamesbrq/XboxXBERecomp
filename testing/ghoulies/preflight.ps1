$ErrorActionPreference = 'Stop'
$xbe = Join-Path $PSScriptRoot 'assets\default.xbe'
if (!(Test-Path -LiteralPath $xbe)) { throw 'Run setup.ps1 with your extracted game folder first.' }
$bytes = [IO.File]::ReadAllBytes($xbe)
if ($bytes.Length -lt 0x178 -or [Text.Encoding]::ASCII.GetString($bytes, 0, 4) -ne 'XBEH') {
    throw 'default.xbe is not a valid XBE header.'
}
$base = [BitConverter]::ToUInt32($bytes, 0x104)
$count = [BitConverter]::ToUInt32($bytes, 0x160)
$address = [BitConverter]::ToUInt32($bytes, 0x164)
$offset = [long]$address - $base
if (!$count -or $offset -lt 0 -or $offset + [long]$count * 16 -gt $bytes.Length) {
    throw 'XBE library version table is missing or outside the file.'
}
$builds = @()
for ($i = 0; $i -lt $count; $i++) {
    $entry = [int]($offset + $i * 16)
    $name = [Text.Encoding]::ASCII.GetString($bytes, $entry, 8).TrimEnd([char]0)
    $build = [BitConverter]::ToUInt16($bytes, $entry + 12)
    Write-Host "$name : XDK $build"
    if ($name -eq 'XAPILIB') { $builds += $build }
}
if (!$builds.Count) { throw 'No XAPILIB version found; inspect this release manually.' }
$sdk = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
foreach ($build in $builds) {
    foreach ($file in @('layout.toml', 'signatures.toml')) {
        if (!(Test-Path -LiteralPath (Join-Path $sdk "xdk\$build\$file"))) {
            throw "XDK $build is unsupported: missing xdk/$build/$file. Add verified layout and signatures before building Ghoulies."
        }
    }
}
Write-Host 'XBE and XDK tables found. A successful scan/build still requires resolved symbols.'
