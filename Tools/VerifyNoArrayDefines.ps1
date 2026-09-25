param(
    [string[]]$Roots = @('src', 'lib\librrf')
)

$ErrorActionPreference = 'Stop'
$pattern = '^\s*#\s*define\s+array(?:\s|$)'
$hits = @()

foreach ($root in $Roots) {
    if (-not (Test-Path -LiteralPath $root)) {
        continue
    }
    $files = Get-ChildItem -LiteralPath $root -Recurse -File -ErrorAction Stop |
        Where-Object { $_.Extension -in '.h', '.hpp', '.hh', '.hxx' }
    foreach ($file in $files) {
        $matches = Select-String -LiteralPath $file.FullName -Pattern $pattern -ErrorAction Stop
        if ($matches) { $hits += $matches }
    }
}

if ($hits.Count -gt 0) {
    foreach ($hit in $hits) {
        Write-Host ($hit.Path + ':' + $hit.LineNumber + ': ' + $hit.Line)
    }
    exit 2
}

Write-Host '      Source-tree macro scan passed: no active #define array found.'
exit 0
