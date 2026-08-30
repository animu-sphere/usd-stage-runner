cmake_minimum_required(VERSION 3.25)

project(
  usd-stage-runner
  VERSION 0.1.0
  DESCRIPTION "A small real-time runtime host for OpenUSD stages"
  LANGUAGES CXX
)

include(CTest)

option(USD_STAGE_RUNNER_BUILD_APP "Build the stage_runner host" ON)
option(USD_STAGE_RUNNER_REQUIRE_OPENUSD "Fail configuration when OpenUSD is unavailable" OFF)

add_library(usd_stage_runner_warnings INTERFACE)
if(MSVC)
  target_compile_options(usd_stage_runner_warnings INTERFACE /W4 /permissive-)
else()
  target_compile_options(usd_stage_runner_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_subdirectory(libs/runtimeCore)
add_subdirectory(libs/inputCore)
add_subdirectory(libs/physicsCore)
add_subdirectory(backends/inputSdl)
add_subdirectory(backends/physicsJolt)

if(USD_STAGE_RUNNER_BUILD_APP)
  add_subdirectory(apps/stage_runner)
endif()

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
