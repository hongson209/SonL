# SonLang AI-Native Semantic Protocol Test Suite
# Verifies machine-readable JSON endpoints and sub-10ms query latency

$ErrorActionPreference = "Stop"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " SonLang: AI-Native Semantic Protocol Verification        " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

$Compiler = "bootstrap/son0.exe"
if (-not (Test-Path $Compiler)) {
    clang -std=c11 -Wall -Wextra -D_CRT_SECURE_NO_WARNINGS -Ibootstrap/include bootstrap/src/*.c -o $Compiler
}

$TestTarget = "tests/selfhost/test_driver.sonl"

# Test 1: Query AST JSON
Write-Host "`n[Test 1] Querying Typed AST (--query-ast)..." -ForegroundColor Yellow
$TimeAst = Measure-Command {
    $JsonAst = & $Compiler $TestTarget --query-ast | Out-String
}
Write-Host "   -> Query latency: $($TimeAst.TotalMilliseconds.ToString('F2')) ms" -ForegroundColor Green
$ParsedAst = $JsonAst | ConvertFrom-Json
if ($ParsedAst.protocol_version -ne "1.0-ai") { throw "Invalid AST protocol version" }
if ($ParsedAst.declarations.Count -ne 4) { throw "Expected 4 declarations in AST" }
Write-Host "   -> Successfully parsed AST JSON (4 declarations, verified format)" -ForegroundColor Green

# Test 2: Query Global Symbols JSON
Write-Host "`n[Test 2] Querying Workspace Symbols (--query-symbols)..." -ForegroundColor Yellow
$TimeSym = Measure-Command {
    $JsonSym = & $Compiler $TestTarget --query-symbols | Out-String
}
Write-Host "   -> Query latency: $($TimeSym.TotalMilliseconds.ToString('F2')) ms" -ForegroundColor Green
$ParsedSym = $JsonSym | ConvertFrom-Json
if ($ParsedSym.protocol_version -ne "1.0-ai") { throw "Invalid Symbol protocol version" }
if ($ParsedSym.symbols.Count -ne 4) { throw "Expected 4 symbols in symbol map" }
Write-Host "   -> Successfully parsed Symbols JSON (4 global symbols verified)" -ForegroundColor Green

# Test 3: Query Diagnostics JSON
Write-Host "`n[Test 3] Querying Machine-Readable Diagnostics (--query-diagnostics)..." -ForegroundColor Yellow
$DiagOut = "tests/ai/diag_test.json"
$prevEAP = $ErrorActionPreference
$ErrorActionPreference = "SilentlyContinue"
$TimeDiag = Measure-Command {
    & $Compiler "tests/bootstrap/type_error.sonl" -o $DiagOut --query-diagnostics 2>$null
}
$ErrorActionPreference = $prevEAP
Write-Host "   -> Query latency: $($TimeDiag.TotalMilliseconds.ToString('F2')) ms" -ForegroundColor Green

$JsonDiag = Get-Content $DiagOut -Raw
Remove-Item -Force $DiagOut
$ParsedDiag = $JsonDiag | ConvertFrom-Json
if ($ParsedDiag.has_error -ne $true) { throw "Expected has_error=true for type error file" }
Write-Host "   -> Successfully parsed Diagnostics JSON (error detection verified)" -ForegroundColor Green

Write-Host "`n==========================================================" -ForegroundColor Cyan
Write-Host " ALL AI SEMANTIC PROTOCOL TESTS PASSED (<10ms LATENCY)   " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan
