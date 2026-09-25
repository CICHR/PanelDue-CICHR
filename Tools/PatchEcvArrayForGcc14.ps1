param(
    [Parameter(Mandatory=$true)]
    [string[]]$Path
)

$ErrorActionPreference = 'Stop'
$pattern = '^\s*#\s*define\s+array(?:\s|$)'

foreach ($item in $Path) {
    $full = [IO.Path]::GetFullPath($item)
    if (-not (Test-Path -LiteralPath $full -PathType Leaf)) {
        throw "eCv header not found: $full"
    }

    $lines = [IO.File]::ReadAllLines($full)
    $changed = $false
    for ($i = 0; $i -lt $lines.Length; $i++) {
        if ($lines[$i] -match $pattern) {
            $lines[$i] = '// PANELDUE_GCC14_DISABLED_ARRAY_MACRO: ' + $lines[$i]
            $changed = $true
        }
    }

    if ($changed) {
        [IO.File]::WriteAllLines($full, $lines, [Text.UTF8Encoding]::new($false))
        Write-Host "  Patched: $full"
    } else {
        Write-Host "  OK: $full"
    }

    # Do not trust the edit message: verify the final file contents again.
    $verify = [IO.File]::ReadAllLines($full)
    foreach ($line in $verify) {
        if ($line -match $pattern) {
            throw "Active '#define array' remains in $full : $line"
        }
    }
}
