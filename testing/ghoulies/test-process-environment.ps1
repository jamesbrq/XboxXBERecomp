$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'process-environment.ps1')
$name = 'XBE_ENVIRONMENT_REGRESSION'
$saved = [Environment]::GetEnvironmentVariable($name, 'Process')
try {
    Set-GhouliesProcessEnvironment $name '1'
    if ([Environment]::GetEnvironmentVariable($name, 'Process') -ne '1') { throw 'Enabled switch missing' }
    Set-GhouliesProcessEnvironment $name $null
    # Test inheritance in a native child, not only PowerShell's view of Env:.
    $result = & python -c "import os; print('$name' in os.environ)"
    if ($LASTEXITCODE -ne 0 -or $result -ne 'False') { throw 'Disabled switch inherited by child' }
    Set-GhouliesProcessEnvironment $name 'restored'
    $result = & python -c "import os; print(os.environ.get('$name'))"
    if ($LASTEXITCODE -ne 0 -or $result -ne 'restored') { throw 'Saved value not inherited' }
    Write-Host 'Process environment regression passed (no game launched).'
} finally {
    Set-GhouliesProcessEnvironment $name $saved
}
