param(
    [string]$Target = 'cy2026',
    [string]$Profile = 'usd'
)

$ErrorActionPreference = 'Stop'
# Run after the compatibility OpenStrata build has materialized its toolchain.
$state = @(Get-ChildItem .strata/targets -Directory |
    Where-Object Name -Like "$Target-*-$Profile")
if ($state.Count -ne 1) { throw 'Expected exactly one selected target toolchain' }
$toolchain = Join-Path $state[0].FullName 'toolchain.cmake'
$externalPrefixes = @($env:OST_EXTERNAL_LIBRARY_PREFIXES -split
    [regex]::Escape([IO.Path]::PathSeparator))
$parserPrefixes = @($externalPrefixes | Where-Object {
    $_ -and (Test-Path (Join-Path $_ 'lib/cmake/physicsUsd/physicsUsdConfig.cmake'))
})
if ($parserPrefixes.Count -ne 1) {
    throw 'Expected exactly one physicsUsd package from ost library pull'
}
$prefixes = @($externalPrefixes, $env:JOLT_PREFIX) |
    ForEach-Object { $_ } | Where-Object { $_ }
$parserConfig = Join-Path $parserPrefixes[0] 'lib/cmake/physicsUsd'
function Assert-PublishedParser([string]$BuildDirectory) {
    $entry = Get-Content (Join-Path $BuildDirectory 'CMakeCache.txt') |
        Where-Object { $_ -match '^physicsUsd_DIR:PATH=' }
    if (@($entry).Count -ne 1 -or
        [IO.Path]::GetFullPath(($entry -split '=', 2)[1]) -ne
        [IO.Path]::GetFullPath($parserConfig)) {
        throw "Build $BuildDirectory did not resolve the published physicsUsd prefix; clear its stale CMake cache"
    }
}

# The parser is a digest-pinned external binary, never a sibling source build.
Write-Host "Using published physicsUsd package: $($parserPrefixes[0])"
cmake -S . -B .ci/standard-cmake "-DCMAKE_TOOLCHAIN_FILE=$toolchain" `
    "-DCMAKE_PREFIX_PATH=$($prefixes -join ';')" -DCMAKE_BUILD_TYPE=Release `
    "-DphysicsUsd_DIR:PATH=$parserConfig" `
    -DBUILD_TESTING=ON -DUSD_STAGE_RUNNER_REQUIRE_OPENUSD=ON `
    -DUSD_STAGE_RUNNER_REQUIRE_JOLT=ON -DUSD_STAGE_RUNNER_ENABLE_PHYSICS_USD=ON
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Assert-PublishedParser .ci/standard-cmake
cmake --build .ci/standard-cmake --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& "$PSScriptRoot/assert_physics_usd_tests.ps1" -BuildDirectory .ci/standard-cmake
ctest --test-dir .ci/standard-cmake -C Release --output-on-failure --no-tests=error
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$env:CMAKE_PREFIX_PATH = $prefixes -join [IO.Path]::PathSeparator
ost build --target $Target --profile $Profile --intent physics-usd
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$build = Join-Path 'build' "$($state[0].Name)--physics-usd"
Assert-PublishedParser $build
& "$PSScriptRoot/assert_physics_usd_tests.ps1" -BuildDirectory $build -PluginView
ost test --target $Target --profile $Profile --intent physics-usd
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
