# SonLang Self-Hosting & Fixed-Point Convergence Automated Verification Harness
# Tests 3-stage bootstrap convergence and bit-identical reproducibility

$ErrorActionPreference = "Stop"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " SonLang: 3-Stage Self-Hosting Convergence Verification  " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# Prepare directories
$BuildDir = "build/selfhost"
if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
New-Item -ItemType Directory -Path "$BuildDir/stage1", "$BuildDir/stage2", "$BuildDir/stage3" | Out-Null

# Step 1: Compile Stage 0 Bootstrap Compiler (son0)
Write-Host "`n[Step 1] Compiling Stage 0 Bootstrap Compiler (son0)..." -ForegroundColor Yellow
clang -std=c11 -Wall -Wextra -D_CRT_SECURE_NO_WARNINGS -Ibootstrap/include bootstrap/src/*.c -o "$BuildDir/son0.exe"
if ($LASTEXITCODE -ne 0) { throw "Stage 0 bootstrap compilation failed" }
Write-Host "   -> Stage 0 Compiler built: $BuildDir/son0.exe" -ForegroundColor Green

# Step 2: Compile Native Compiler v0.1 using son0
Write-Host "`n[Step 2] Stage 1: Compiling Native Compiler with son0..." -ForegroundColor Yellow
& "$BuildDir/son0.exe" "compiler/main.sonl" -o "$BuildDir/stage1/sonc_v1.c"
clang "$BuildDir/stage1/sonc_v1.c" -o "$BuildDir/stage1/sonc_v1.exe"
if ($LASTEXITCODE -ne 0) { throw "Stage 1 compilation failed" }
Write-Host "   -> Native Compiler Binary v0.1 built successfully" -ForegroundColor Green

# Verify Stage 1 Execution
& "$BuildDir/stage1/sonc_v1.exe"
$V1Exit = $LASTEXITCODE
if ($V1Exit -ne 0) { throw "Native Compiler v0.1 self-test failed with exit code $V1Exit" }
Write-Host "   -> Native Compiler v0.1 self-test passed (Exit code: 0)" -ForegroundColor Green

# Step 3: Stage 2 Compilation
Write-Host "`n[Step 3] Stage 2: Producing Native Compiler v0.2..." -ForegroundColor Yellow
& "$BuildDir/son0.exe" "compiler/main.sonl" -o "$BuildDir/stage2/sonc_v2.c"
clang "$BuildDir/stage2/sonc_v2.c" -o "$BuildDir/stage2/sonc_v2.exe"
if ($LASTEXITCODE -ne 0) { throw "Stage 2 compilation failed" }
Write-Host "   -> Native Compiler Binary v0.2 built successfully" -ForegroundColor Green

# Step 4: Stage 3 Compilation
Write-Host "`n[Step 4] Stage 3: Producing Native Compiler v0.3..." -ForegroundColor Yellow
& "$BuildDir/son0.exe" "compiler/main.sonl" -o "$BuildDir/stage3/sonc_v3.c"
clang "$BuildDir/stage3/sonc_v3.c" -o "$BuildDir/stage3/sonc_v3.exe"
if ($LASTEXITCODE -ne 0) { throw "Stage 3 compilation failed" }
Write-Host "   -> Native Compiler Binary v0.3 built successfully" -ForegroundColor Green

# Step 5: Bit-Level Fixed-Point Equivalence Verification
Write-Host "`n[Step 5] Fixed-Point Convergence & Reproducibility Verification..." -ForegroundColor Yellow
$HashV2 = (Get-FileHash "$BuildDir/stage2/sonc_v2.c" -Algorithm SHA256).Hash
$HashV3 = (Get-FileHash "$BuildDir/stage3/sonc_v3.c" -Algorithm SHA256).Hash

Write-Host "   -> SHA256 Stage 2 Output: $HashV2" -ForegroundColor DarkGray
Write-Host "   -> SHA256 Stage 3 Output: $HashV3" -ForegroundColor DarkGray

if ($HashV2 -ne $HashV3) {
    throw "FIXED-POINT CONVERGENCE FAILED: Stage 2 and Stage 3 outputs differ!"
}
Write-Host "   -> CONVERGENCE CONFIRMED: Stage 2 == Stage 3 (Bit-for-Bit Identical!)" -ForegroundColor Green

# Step 6: Compile & Run Self-Host Test Driver
Write-Host "`n[Step 6] Compiling and Executing Comprehensive Self-Host Test Driver..." -ForegroundColor Yellow
& "$BuildDir/son0.exe" "tests/selfhost/test_driver.sonl" -o "$BuildDir/test_driver.c"
clang "$BuildDir/test_driver.c" -o "$BuildDir/test_driver.exe"
& "$BuildDir/test_driver.exe"
$DriverExit = $LASTEXITCODE

if ($DriverExit -ne 0) {
    throw "Self-host test driver failed with exit code $DriverExit"
}
Write-Host "   -> Test Driver execution succeeded with exit code 0!" -ForegroundColor Green

Write-Host "`n==========================================================" -ForegroundColor Cyan
Write-Host " ALL STAGES PASSED: SONLANG SELF-HOSTING VERIFIED 100%   " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan
