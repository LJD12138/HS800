# Final cleanup

$dirPrefix = "g:\1-Baiku_Projects\25-HS800\1.software\HS800\APP\Docs\"

# Remove script files
$scriptFiles = @("final_fix.ps1")
foreach ($f in $scriptFiles) {
    $p = Join-Path $dirPrefix $f
    if (Test-Path $p) {
        Remove-Item $p -Force -ErrorAction SilentlyContinue
    }
}

# Show final state
Get-ChildItem $dirPrefix -Filter "*.docx" | Select-Object Name, Length
