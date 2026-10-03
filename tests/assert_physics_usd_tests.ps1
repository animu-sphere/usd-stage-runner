param(
    [Parameter(Mandatory)][string]$BuildDirectory,
    [switch]$PluginView
)

$ErrorActionPreference = 'Stop'
$listing = ctest --test-dir $BuildDirectory -C Release --show-only=json-v1
if ($LASTEXITCODE -ne 0) { throw 'Could not enumerate the configured tests' }
$names = @((($listing -join "`n") | ConvertFrom-Json).tests.name)
$required = @(
    'stage_runtime.standard_physics_parity',
    'stage_runner.standard_falling_cube',
    'stage_runner.standard_character_walk_and_ground',
    'stage_runner.standard_character_jump',
    'stage_runner.standard_camera_collision_and_sync',
    'stage_runner.physics_falling_cube',
    'stage_runner.character_walk_and_ground',
    'stage_runner.character_jump',
    'stage_runner.camera_collision_and_sync',
    'usdviewStageRunner.native_session',
    'usdviewStageRunner.standard_character'
)
if ($PluginView) {
    $required += @(
        'usdviewStageRunner.ost_plugin_view_character',
        'usdviewStageRunner.ost_plugin_view_standard_character'
    )
}
$missing = @($required | Where-Object { $_ -notin $names })
if ($missing.Count) {
    throw "Standard/compatibility host coverage is missing: $($missing -join ', ')"
}
Write-Host "Standard/compatibility host coverage present ($($required.Count) required tests)"
