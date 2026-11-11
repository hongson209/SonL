# SonLang Comprehensive Multi-Language Benchmark Runner
# Compares SonLang against C (Clang -O3), Rust (rustc -O), and Zig (zig -O ReleaseFast)

$ErrorActionPreference = "Stop"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " SonLang vs C vs Rust vs Zig Benchmark Suite             " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# 1. Environment Metadata Capture
$OsName = [System.Environment]::OSVersion.VersionString
$Arch = [System.Environment]::GetEnvironmentVariable("PROCESSOR_ARCHITECTURE")
$CpuModel = (Get-CimInstance Win32_Processor | Select-Object -First 1).Name.Trim()
$CompilerVersion = (clang --version | Select-Object -First 1)
$RustVersion = (rustc --version)
$ZigVersion = (zig version)

Write-Host "`nEnvironment Metadata:" -ForegroundColor DarkGray
Write-Host "   OS: $OsName ($Arch)" -ForegroundColor DarkGray
Write-Host "   CPU: $CpuModel" -ForegroundColor DarkGray
Write-Host "   Clang: $CompilerVersion" -ForegroundColor DarkGray
Write-Host "   Rust:  $RustVersion" -ForegroundColor DarkGray
Write-Host "   Zig:   $ZigVersion" -ForegroundColor DarkGray

# 2. Build Tools
$BuildDir = "benchmarks/bin"
if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
New-Item -ItemType Directory -Path $BuildDir | Out-Null

$Son0 = "$BuildDir/son0.exe"
if (-not (Test-Path "bin/son.exe")) {
    throw "bin/son.exe not found"
}
Copy-Item "bin/son.exe" $Son0 -Force

$Workloads = @(
    @{ 
        Name = "01_Integer_Math"
        Sonl = "benchmarks/workloads/01_integer_math/prime_sieve.sonl"
        C    = "benchmarks/workloads/01_integer_math/prime_sieve.c"
        Rs   = "benchmarks/workloads/01_integer_math/prime_sieve.rs"
        Zig  = "benchmarks/workloads/01_integer_math/prime_sieve.zig"
    },
    @{ 
        Name = "02_Dense_Matrix"
        Sonl = "benchmarks/workloads/02_matrix/matrix_mult.sonl"
        C    = "benchmarks/workloads/02_matrix/matrix_mult.c"
        Rs   = "benchmarks/workloads/02_matrix/matrix_mult.rs"
        Zig  = "benchmarks/workloads/02_matrix/matrix_mult.zig"
    },
    @{ 
        Name = "03_Sorting_Pass"
        Sonl = "benchmarks/workloads/03_sorting/quicksort.sonl"
        C    = "benchmarks/workloads/03_sorting/quicksort.c"
        Rs   = "benchmarks/workloads/03_sorting/quicksort.rs"
        Zig  = "benchmarks/workloads/03_sorting/quicksort.zig"
    }
)

$Results = @()

foreach ($w in $Workloads) {
    Write-Host "`n--> Benchmarking Workload: $($w.Name)..." -ForegroundColor Yellow

    # Compile SonLang (Maximum CPU AVX2/FMA/Vectorization + LLD LTO flags)
    $SonlC = "$BuildDir/$($w.Name)_sonl.c"
    $SonlExe = "$BuildDir/$($w.Name)_sonl.exe"
    & $Son0 $w.Sonl -o $SonlC
    clang -O3 -march=native -mtune=native -mavx2 -mfma -ffast-math -fomit-frame-pointer -flto -fuse-ld=lld -funroll-loops -fvectorize $SonlC -o $SonlExe

    # Compile C
    $CExe = "$BuildDir/$($w.Name)_c.exe"
    clang -O3 -march=native -ffast-math -funroll-loops $w.C -o $CExe

    # Compile Rust
    $RsExe = "$BuildDir/$($w.Name)_rs.exe"
    rustc -O -C target-cpu=native $w.Rs -o $RsExe

    # Compile Zig
    $ZigExe = "$BuildDir/$($w.Name)_zig.exe"
    zig build-exe -O ReleaseFast -mcpu=native $w.Zig
    Move-Item "$([System.IO.Path]::GetFileNameWithoutExtension($w.Zig)).exe" $ZigExe -Force
    if (Test-Path "$([System.IO.Path]::GetFileNameWithoutExtension($w.Zig)).pdb") {
        Remove-Item "$([System.IO.Path]::GetFileNameWithoutExtension($w.Zig)).pdb" -Force
    }

    # Warm-up all
    & $SonlExe; & $CExe; & $RsExe; & $ZigExe

    # Measure function (5 iterations average)
    function Measure-Exe($exe) {
        $times = @()
        for ($i = 0; $i -lt 5; $i++) {
            $sw = [System.Diagnostics.Stopwatch]::StartNew()
            & $exe
            $sw.Stop()
            $times += $sw.Elapsed.TotalMilliseconds
        }
        return [math]::Round(($times | Measure-Object -Average).Average, 2)
    }

    $SonlTime = Measure-Exe $SonlExe
    $CTime    = Measure-Exe $CExe
    $RsTime   = Measure-Exe $RsExe
    $ZigTime  = Measure-Exe $ZigExe

    Write-Host "   SonLang: $SonlTime ms | C: $CTime ms | Rust: $RsTime ms | Zig: $ZigTime ms" -ForegroundColor Green

    $Results += [PSCustomObject]@{
        Workload = $w.Name
        SonLang_ms = $SonlTime
        C_ms       = $CTime
        Rust_ms    = $RsTime
        Zig_ms     = $ZigTime
    }
}

# 3. Export JSON report
$ResultsDir = "benchmarks/results"
if (-not (Test-Path $ResultsDir)) { New-Item -ItemType Directory -Path $ResultsDir | Out-Null }

$Report = @{
    environment = @{
        os = $OsName
        arch = $Arch
        cpu = $CpuModel
        clang = $CompilerVersion
        rust = $RustVersion
        zig = $ZigVersion
    }
    benchmarks = $Results
}
$Report | ConvertTo-Json -Depth 4 | Set-Content "$ResultsDir/benchmark_report.json"
Write-Host "`n[Report Exported] $ResultsDir/benchmark_report.json" -ForegroundColor Cyan

# 4. Display Markdown Summary Table
Write-Host "`n| Workload | SonLang | C (-O3) | Rust (-O) | Zig (ReleaseFast) |" -ForegroundColor White
Write-Host "|---|---|---|---|---|" -ForegroundColor White
foreach ($r in $Results) {
    Write-Host "| $($r.Workload) | $($r.SonLang_ms) ms | $($r.C_ms) ms | $($r.Rust_ms) ms | $($r.Zig_ms) ms |" -ForegroundColor White
}

Write-Host "`n==========================================================" -ForegroundColor Cyan
Write-Host " MULTI-LANGUAGE BENCHMARK COMPLETED SUCCESSFULLY          " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan
