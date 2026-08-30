set(USD_STAGE_RUNNER_OPENUSD_TARGET "")
set(USD_STAGE_RUNNER_USDGEOM_TARGET "")
find_package(pxr CONFIG QUIET)

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

if(USD_STAGE_RUNNER_OPENUSD_TARGET AND NOT USD_STAGE_RUNNER_USDGEOM_TARGET)
  set(USD_STAGE_RUNNER_OPENUSD_TARGET "")
endif()

set(USD_STAGE_RUNNER_OPENUSD_TARGET "${USD_STAGE_RUNNER_OPENUSD_TARGET}" PARENT_SCOPE)

if(USD_STAGE_RUNNER_REQUIRE_OPENUSD AND NOT USD_STAGE_RUNNER_OPENUSD_TARGET)
  message(FATAL_ERROR "OpenUSD was requested but no usd CMake target was found")
endif()

add_executable(stage_runner main.cpp)
target_compile_features(stage_runner PRIVATE cxx_std_17)
target_link_libraries(stage_runner PRIVATE runtimeCore::runtimeCore inputCore::inputCore
  inputSdl::inputSdl usd_stage_runner_warnings)

if(USD_STAGE_RUNNER_OPENUSD_TARGET)
  target_compile_definitions(stage_runner PRIVATE USD_STAGE_RUNNER_HAS_OPENUSD=1)
  target_link_libraries(stage_runner PRIVATE ${USD_STAGE_RUNNER_OPENUSD_TARGET}
    ${USD_STAGE_RUNNER_USDGEOM_TARGET})
  message(STATUS "stage_runner: OpenUSD support enabled (${USD_STAGE_RUNNER_OPENUSD_TARGET})")
else()
  target_compile_definitions(stage_runner PRIVATE USD_STAGE_RUNNER_HAS_OPENUSD=0)
  message(STATUS "stage_runner: OpenUSD not found; host will report that Stage loading is unavailable")
endif()

install(TARGETS stage_runner RUNTIME DESTINATION bin)
