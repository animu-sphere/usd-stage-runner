param(
    [string]$Target = 'cy2026',
    [string]$Profile = 'usd',
    [string]$ParserSource = '.ci/physics/libs/physicsUsd'
)

$ErrorActionPreference = 'Stop'
# Run after the compatibility OpenStrata build has materialized its toolchain.
$state = @(Get-ChildItem .strata/targets -Directory |
    Where-Object Name -Like "$Target-*-$Profile")
if ($state.Count -ne 1) { throw 'Expected exactly one selected target toolchain' }
$toolchain = Join-Path $state[0].FullName 'toolchain.cmake'
$parserPrefix = Join-Path $PWD '.ci/physics-usd-install'
$externalPrefixes = @($env:OST_EXTERNAL_LIBRARY_PREFIXES -split
    [regex]::Escape([IO.Path]::PathSeparator))
$prefixes = @($externalPrefixes, $env:JOLT_PREFIX) |
    ForEach-Object { $_ } | Where-Object { $_ }

cmake -S $ParserSource -B .ci/physics-usd-build `
    "-DCMAKE_TOOLCHAIN_FILE=$toolchain" "-DCMAKE_PREFIX_PATH=$($prefixes -join ';')" `
    "-DCMAKE_INSTALL_PREFIX=$parserPrefix" -DCMAKE_BUILD_TYPE=Release -DPHYSICSUSD_BUILD_TESTS=ON
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build .ci/physics-usd-build --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
ctest --test-dir .ci/physics-usd-build -C Release --output-on-failure --no-tests=error
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --install .ci/physics-usd-build --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$prefixes = @($parserPrefix) + $prefixes
cmake -S . -B .ci/standard-cmake "-DCMAKE_TOOLCHAIN_FILE=$toolchain" `
    "-DCMAKE_PREFIX_PATH=$($prefixes -join ';')" -DCMAKE_BUILD_TYPE=Release `
    -DBUILD_TESTING=ON -DUSD_STAGE_RUNNER_REQUIRE_OPENUSD=ON `
    -DUSD_STAGE_RUNNER_REQUIRE_JOLT=ON -DUSD_STAGE_RUNNER_ENABLE_PHYSICS_USD=ON
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build .ci/standard-cmake --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& "$PSScriptRoot/assert_physics_usd_tests.ps1" -BuildDirectory .ci/standard-cmake
ctest --test-dir .ci/standard-cmake -C Release --output-on-failure --no-tests=error
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$env:CMAKE_PREFIX_PATH = $prefixes -join [IO.Path]::PathSeparator
ost build --target $Target --profile $Profile --intent physics-usd
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$build = Join-Path 'build' "$($state[0].Name)--physics-usd"
& "$PSScriptRoot/assert_physics_usd_tests.ps1" -BuildDirectory $build -PluginView
ost test --target $Target --profile $Profile --intent physics-usd
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
