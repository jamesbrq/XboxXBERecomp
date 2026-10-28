param(
    [ValidateSet('patched', 'release')][string]$Build = 'patched',
    [ValidateRange(1, 3600)][int]$Seconds = 120,
    [ValidateRange(0, 3600)][int]$CaptureEvery = 0,
    [switch]$Diagnostics,
    [switch]$AudioDump,
    [switch]$DebugGpu,
    [switch]$NoHrtf,
    [switch]$NoReverb,
    [switch]$LeaveOpen,
    [switch]$Timed,
    [string]$InputScript
)
$ErrorActionPreference = 'Stop'
$exe = Join-Path $PSScriptRoot "out\build\$Build\Ghoulies.exe"
if (!(Test-Path -LiteralPath $exe)) { throw 'Build Ghoulies with build.bat first.' }
$run = Join-Path $PSScriptRoot ('runs\' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $run -Force | Out-Null
$vars = @{
    XBE_VERBOSE = $(if ($Diagnostics) { '1' } else { $null })
    XBE_PERF = $(if ($Diagnostics) { '1' } else { $null })
    XBE_TRACE_KERNEL = $(if ($Diagnostics) { '1' } else { $null })
    XBE_CAPTURE_DIR = $run; XBE_CAPTURE_EVERY = [string]$CaptureEvery
    XBE_AUDIO_DUMP = $(if ($AudioDump) { Join-Path $run 'audio.wav' } else { $null })
    XBE_D3D12_DEBUG = $(if ($DebugGpu) { '1' } else { $null })
    XBE_NO_HRTF = $(if ($NoHrtf) { '1' } else { $null })
    XBE_NO_REVERB = $(if ($NoReverb) { '1' } else { $null })
    XBE_INPUT_SCRIPT = $(if ($InputScript) { (Get-Content -LiteralPath $InputScript -Raw).Trim() } else { $null })
}
$saved = @{}
try {
    foreach ($key in $vars.Keys) {
        $saved[$key] = [Environment]::GetEnvironmentVariable($key, 'Process')
        [Environment]::SetEnvironmentVariable($key, $vars[$key], 'Process')
    }
    $process = Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe) -WindowStyle Hidden -PassThru
    if ($LeaveOpen -or !$Timed) {
        $process.WaitForExit()
        $finished = $true
    } else {
        $finished = $process.WaitForExit($Seconds * 1000)
    }
    if (!$finished) {
        # Ask for a normal close so audio dumps and logs can finish writing.
        $null = $process.CloseMainWindow()
        $finished = $process.WaitForExit(10000)
    }
    $forced = !$finished
    if ($forced) { Stop-Process -Id $process.Id -Force; $process.WaitForExit() }
    $process.Refresh()
    $result = [ordered]@{
        sdk = (& 'C:\Program Files\Git\cmd\git.exe' -C $PSScriptRoot rev-parse HEAD)
        sdkDirty = [bool](& 'C:\Program Files\Git\cmd\git.exe' -C $PSScriptRoot status --porcelain --untracked-files=normal)
        executableSha256 = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
        configSha256 = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'config\game.toml') -Algorithm SHA256).Hash
        seconds = $Seconds; exitCode = $process.ExitCode; forcedTermination = $forced
        build = $Build; leaveOpen = [bool]($LeaveOpen -or !$Timed)
        diagnostics = [bool]$Diagnostics; audioDump = [bool]$AudioDump; captureEvery = $CaptureEvery
        noHrtf = [bool]$NoHrtf; noReverb = [bool]$NoReverb; debugGpu = [bool]$DebugGpu
        xbeSha1 = (Get-FileHash (Join-Path $PSScriptRoot 'assets\default.xbe') -Algorithm SHA1).Hash
    }
    $result | ConvertTo-Json | Set-Content (Join-Path $run 'result.json')
    $logs = Join-Path (Split-Path $exe) 'logs'
    if (Test-Path $logs) { Copy-Item -LiteralPath $logs -Destination $run -Recurse }
    Write-Host "Diagnostics saved to $run"
    if ($forced -or $process.ExitCode -ne 0) { throw 'Run ended abnormally; inspect result.json and logs.' }
} finally {
    foreach ($key in $saved.Keys) { [Environment]::SetEnvironmentVariable($key, $saved[$key], 'Process') }
}
