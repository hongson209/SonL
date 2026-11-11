# SonLang Unified Toolchain CLI (`son`)

param(
    [Parameter(Position=0)]
    [string]$Command = "help",

    [Parameter(Position=1)]
    [string]$Target = "",

    [Parameter(ValueFromRemainingArguments=$true)]
    [string[]]$RemainingArgs
)

$ErrorActionPreference = "Stop"

function Show-Help {
    Write-Host "SonLang Unified Toolchain (son) v0.1" -ForegroundColor Cyan
    Write-Host "Usage: son <command> [options]`n"
    Write-Host "Commands:"
    Write-Host "  build <file.sonl>     Compile a SonLang source file to native binary (--native for direct LLVM)"
    Write-Host "  run <file.sonl>       Compile and immediately run the executable"
    Write-Host "  emit-ir <file.sonl>   Emit SonIR (Intermediate Representation) SSA format"
    Write-Host "  emit-llvm <file.sonl> Emit native LLVM IR (.ll) representation"
    Write-Host "  new <project_name>    Create a new SonLang project with son.toml manifest"
    Write-Host "  test                  Run comprehensive test suite across all 31 workloads + 3 negative tests"
    Write-Host "  verify-selfhost       Execute 3-stage self-hosting convergence verification"
    Write-Host "  query <file.sonl>     Run AI-Native semantic query (--ast, --symbols)"
    Write-Host "  help                  Display this help message"
}

if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

switch ($Command.ToLower()) {
    "build" {
        if (-not $Target) { Write-Error "Missing target file. Usage: son build <file.sonl>"; exit 1 }
        $outExe = "build/" + [System.IO.Path]::GetFileNameWithoutExtension($Target) + ".exe"
        $isNative = ($RemainingArgs -contains "--native" -or $RemainingArgs -contains "-native")
        $isFreestanding = ($RemainingArgs -contains "--freestanding")
        $linkerFlags = @($RemainingArgs | Where-Object { $_ -like "-l*" -or $_ -like "-L*" -or $_ -like "-Wl*" })
        
        Write-Host "Compiling $Target -> $outExe..." -ForegroundColor Cyan
        if ($isFreestanding) {
            $outLL = "build/" + [System.IO.Path]::GetFileNameWithoutExtension($Target) + ".ll"
            & ./bin/son.exe $Target --emit-llvm -o $outLL
            clang -O3 -nostdlib -fuse-ld=lld -Wno-override-module "-Wl,/entry:main" "-Wl,/subsystem:console" -lkernel32 $outLL -o $outExe $linkerFlags
        } elseif ($isNative) {
            $outLL = "build/" + [System.IO.Path]::GetFileNameWithoutExtension($Target) + ".ll"
            & ./bin/son.exe $Target --emit-llvm -o $outLL
            clang -Wno-override-module $outLL -o $outExe $linkerFlags
        } else {
            $outC = "build/" + [System.IO.Path]::GetFileNameWithoutExtension($Target) + ".c"
            & ./bin/son.exe $Target -o $outC
            clang $outC -o $outExe $linkerFlags
        }
        Write-Host "Build complete: $outExe" -ForegroundColor Green
    }
    "run" {
        if (-not $Target) { Write-Error "Missing target file. Usage: son run <file.sonl>"; exit 1 }
        $outExe = "build/" + [System.IO.Path]::GetFileNameWithoutExtension($Target) + ".exe"
        $isNative = ($RemainingArgs -contains "--native" -or $RemainingArgs -contains "-native")
        $linkerFlags = @($RemainingArgs | Where-Object { $_ -like "-l*" -or $_ -like "-L*" -or $_ -like "-Wl*" })
        $runArgs = @($RemainingArgs | Where-Object { $_ -notlike "-l*" -and $_ -notlike "-L*" -and $_ -notlike "-Wl*" -and $_ -ne "--native" -and $_ -ne "-native" })
        
        if ($isNative) {
            $outLL = "build/" + [System.IO.Path]::GetFileNameWithoutExtension($Target) + ".ll"
            & ./bin/son.exe $Target --emit-llvm -o $outLL
            clang -Wno-override-module $outLL -o $outExe $linkerFlags
        } else {
            $outC = "build/" + [System.IO.Path]::GetFileNameWithoutExtension($Target) + ".c"
            & ./bin/son.exe $Target -o $outC
            clang $outC -o $outExe $linkerFlags
        }
        & $outExe $runArgs
    }
    "emit-ir" {
        if (-not $Target) { Write-Error "Missing target file. Usage: son emit-ir <file.sonl>"; exit 1 }
        & ./bin/son.exe $Target --emit-ir
    }
    "emit-llvm" {
        if (-not $Target) { Write-Error "Missing target file. Usage: son emit-llvm <file.sonl>"; exit 1 }
        & ./bin/son.exe $Target --emit-llvm
    }
    "test" {
        & ./tests/run_all.ps1
    }
    "verify-selfhost" {
        & ./tests/selfhost/verify_selfhost.ps1
    }
    "new" {
        if (-not $Target) { Write-Error "Missing project name. Usage: son new <project_name>"; exit 1 }
        $projDir = $Target
        if (Test-Path $projDir) { Write-Error "Directory '$projDir' already exists."; exit 1 }
        New-Item -ItemType Directory -Path "$projDir/src" -Force | Out-Null
        
        $tomlContent = @"
[package]
name = "$Target"
version = "0.1.0"
edition = "2026"
entry = "src/main.sonl"
"@
        Set-Content -Path "$projDir/son.toml" -Value $tomlContent
        
        $mainContent = @"
// Entrypoint for $Target (.sonl)
import "library/std/io.sonl";

fn main() -> i32 {
    return 0;
}
"@
        Set-Content -Path "$projDir/src/main.sonl" -Value $mainContent
        Write-Host "Created new SonLang project: $projDir" -ForegroundColor Green
    }
    "query" {
        if (-not $Target) { Write-Error "Missing target file. Usage: son query <file.sonl> [--ast|--symbols]"; exit 1 }
        if ($RemainingArgs -contains "--symbols") {
            & ./bin/son.exe $Target --query-symbols
        } else {
            & ./bin/son.exe $Target --query-ast
        }
    }
    default {
        Show-Help
    }
}
