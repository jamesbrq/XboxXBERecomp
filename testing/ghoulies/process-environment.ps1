function Set-GhouliesProcessEnvironment([string]$Name, [AllowNull()][object]$Value) {
    # Null must mean absent: empty, present switches enable the native runtime.
    if ($null -eq $Value) {
        Remove-Item -LiteralPath "Env:$Name" -ErrorAction SilentlyContinue
    } else {
        [Environment]::SetEnvironmentVariable($Name, [string]$Value, 'Process')
    }
}
