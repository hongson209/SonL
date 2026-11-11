# Comprehensive SonLang Test Runner
$ErrorActionPreference = "Stop"

$tests = @(
    "tests/bootstrap/hello.sonl",
    "tests/bootstrap/unions_match.sonl",
    "tests/bootstrap/compact_syntax.sonl",
    "tests/bootstrap/match_expression.sonl",
    "tests/bootstrap/generics_option.sonl",
    "tests/selfhost/test_driver.sonl",
    "tests/reference_apps/web_service.sonl",
    "tests/reference_apps/game_server.sonl",
    "library/core/types.sonl",
    "library/std/mem/arena.sonl",
    "library/std/collections/vec.sonl",
    "library/std/collections/map.sonl",
    "tests/bootstrap/test_collections_advanced.sonl",
    "library/std/os/fs.sonl",
    "library/std/os/net.sonl",
    "library/std/math.sonl",
    "library/core/string.sonl",
    "tests/bootstrap/test_string_complete.sonl",
    "library/core/slice.sonl",
    "library/std/sync/atomic.sonl",
    "tests/bootstrap/test_import.sonl",
    "tests/bootstrap/test_atomic.sonl",
    "tests/bootstrap/test_llvm_emit.sonl",
    "tests/bootstrap/test_llvm_full_codegen.sonl",
    "tests/bootstrap/test_native_compile_e2e.sonl",
    "tests/bootstrap/test_parser_native.sonl",
    "library/std/time.sonl",
    "tests/bootstrap/test_time.sonl",
    "library/std/mem/allocator.sonl",
    "tests/bootstrap/test_allocator.sonl",
    "library/std/encoding/json.sonl",
    "tests/bootstrap/test_json.sonl",
    "library/core/fmt.sonl",
    "tests/bootstrap/test_fmt.sonl",
    "library/std/os/process.sonl",
    "library/std/net/http.sonl",
    "tests/bootstrap/test_http.sonl",
    "library/std/crypto/base64.sonl",
    "library/std/os/path.sonl",
    "library/std/cli/args.sonl",
    "tests/bootstrap/test_crypto_path.sonl",
    "library/std/ai/vector.sonl",
    "library/std/finance/orderbook.sonl",
    "library/std/game/ecs.sonl",
    "tests/bootstrap/test_frontiers.sonl",
    "library/std/net/server.sonl",
    "tests/bootstrap/test_server.sonl",
    "library/std/crypto/crc32.sonl",
    "library/std/net/url.sonl",
    "library/std/collections/ring.sonl",
    "library/std/sync/spinlock.sonl",
    "library/std/math/matrix4.sonl",
    "tests/bootstrap/test_advanced_stdlib.sonl",
    "compiler/main.sonl",
    "benchmarks/workloads/01_integer_math/prime_sieve.sonl",
    "benchmarks/workloads/02_matrix/matrix_mult.sonl",
    "benchmarks/workloads/03_sorting/quicksort.sonl",
    "tests/bootstrap/test_c_ffi.sonl",
    "library/std/mem/heap.sonl",
    "tests/bootstrap/test_heap.sonl",
    "library/std/mem/pool.sonl",
    "library/std/protocol/endian.sonl",
    "library/std/protocol/varint.sonl",
    "tests/bootstrap/test_protocol.sonl",
    "tests/bootstrap/test_net_socket.sonl",
    "examples/m1_echo_server.sonl",
    "library/std/protocol/minecraft.sonl",
    "examples/m2_minecraft_ping.sonl",
    "tests/bootstrap/test_native_backend.sonl",
    "tests/bootstrap/test_ai_tensor.sonl",
    "tests/bootstrap/test_channel.sonl",
    "tests/bootstrap/test_graph.sonl",
    "tests/bootstrap/test_pq_sha.sonl"
)

if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

Write-Host "Running SonLang Test Suite..." -ForegroundColor Cyan

foreach ($t in $tests) {
    & bin/son.exe $t -o build/t.c
    if ($LASTEXITCODE -ne 0) { throw "son failed on $t" }

    $src = Get-Content $t -Raw
    if ($src -match "fn main") {
        clang build/t.c -o build/t.exe -lws2_32
        if ($LASTEXITCODE -ne 0) { throw "clang build failed on $t" }
        
        & build/t.exe
        if ($LASTEXITCODE -ne 0) { throw "execution failed on $t with code $LASTEXITCODE" }
    } else {
        clang -c build/t.c -o build/t.o
        if ($LASTEXITCODE -ne 0) { throw "clang library compilation failed on $t" }
    }
    
    Write-Host "  [PASS] $t" -ForegroundColor Green
}

$negative_tests = @(
    "tests/bootstrap/borrow_fail_multiple_mut.sonl",
    "tests/bootstrap/borrow_fail_assign_while_borrowed.sonl",
    "tests/bootstrap/borrow_fail_call_conflict.sonl"
)

Write-Host "`nRunning Negative Borrow Checker Tests..." -ForegroundColor Cyan
foreach ($nt in $negative_tests) {
    $oldEAP = $ErrorActionPreference
    $ErrorActionPreference = "SilentlyContinue"
    & bin/son.exe $nt -o build/t_neg.c *>$null
    $code = $LASTEXITCODE
    $ErrorActionPreference = $oldEAP
    if ($code -eq 0) {
        throw "Expected failure on negative test $nt but it succeeded!"
    }
    Write-Host "  [PASS (Correctly Rejected)] $nt" -ForegroundColor Green
}

Write-Host "`nAll positive and negative test suites verified successfully!" -ForegroundColor Cyan
