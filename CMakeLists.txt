cmake_minimum_required(VERSION 3.25)

project(
  usd-stage-runner
  VERSION 0.1.0
  DESCRIPTION "A small real-time runtime host for OpenUSD stages"
  LANGUAGES CXX
)

include(CTest)
include(GNUInstallDirs)

set(USD_STAGE_RUNNER_SCHEMA_INSTALL_DIR
  "${CMAKE_INSTALL_DATADIR}/usd-stage-runner/runnerSchema")

option(USD_STAGE_RUNNER_BUILD_APP "Build the stage_runner host" ON)
option(USD_STAGE_RUNNER_BUILD_USDVIEW_PLUGIN "Build the usdview host adapter" ON)
option(USD_STAGE_RUNNER_REQUIRE_OPENUSD "Fail configuration when OpenUSD is unavailable" OFF)

set(USD_STAGE_RUNNER_OPENUSD_TARGET "")
set(USD_STAGE_RUNNER_USDGEOM_TARGET "")
set(USD_STAGE_RUNNER_PLUG_TARGET "")
if(USD_STAGE_RUNNER_REQUIRE_OPENUSD)
  find_package(pxr CONFIG REQUIRED)
else()
  find_package(pxr CONFIG QUIET)
endif()

foreach(candidate IN ITEMS usd pxr::usd)
  if(TARGET ${candidate})
    set(USD_STAGE_RUNNER_OPENUSD_TARGET ${candidate})
    break()
  endif()
endforeach()
foreach(candidate IN ITEMS usdGeom pxr::usdGeom)
  if(TARGET ${candidate})
    set(USD_STAGE_RUNNER_USDGEOM_TARGET ${candidate})
    break()
  endif()
endforeach()
foreach(candidate IN ITEMS plug pxr::plug)
  if(TARGET ${candidate})
    set(USD_STAGE_RUNNER_PLUG_TARGET ${candidate})
    break()
  endif()
endforeach()

if(USD_STAGE_RUNNER_OPENUSD_TARGET AND
   (NOT USD_STAGE_RUNNER_USDGEOM_TARGET OR NOT USD_STAGE_RUNNER_PLUG_TARGET))
  set(USD_STAGE_RUNNER_OPENUSD_TARGET "")
endif()
if(USD_STAGE_RUNNER_REQUIRE_OPENUSD AND NOT USD_STAGE_RUNNER_OPENUSD_TARGET)
  message(FATAL_ERROR "OpenUSD was requested but no usd CMake target was found")
endif()

add_library(usd_stage_runner_warnings INTERFACE)
if(MSVC)
  target_compile_options(usd_stage_runner_warnings INTERFACE /W4 /permissive- /EHsc)
else()
  target_compile_options(usd_stage_runner_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_subdirectory(libs/runtimeCore)
add_subdirectory(libs/inputCore)
add_subdirectory(libs/physicsCore)
add_subdirectory(libs/characterCore)
add_subdirectory(libs/cameraCore)
add_subdirectory(libs/vehicleCore)
add_subdirectory(backends/inputSdl)
add_subdirectory(backends/physicsJolt)
add_subdirectory(plugins/runnerSchema)
if(USD_STAGE_RUNNER_OPENUSD_TARGET)
  add_subdirectory(libs/stageRuntime)
  if(USD_STAGE_RUNNER_BUILD_USDVIEW_PLUGIN)
    if(TARGET python AND TARGET tf)
      add_subdirectory(plugins/usdviewStageRunner)
    else()
      message(STATUS
        "usdviewStageRunner: OpenUSD Python support unavailable; plugin disabled")
    endif()
  endif()
endif()

if(USD_STAGE_RUNNER_BUILD_APP)
  add_subdirectory(apps/stage_runner)
endif()

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
