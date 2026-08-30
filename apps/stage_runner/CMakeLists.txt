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

set(USD_STAGE_RUNNER_OPENUSD_TARGET "${USD_STAGE_RUNNER_OPENUSD_TARGET}" PARENT_SCOPE)

if(USD_STAGE_RUNNER_REQUIRE_OPENUSD AND NOT USD_STAGE_RUNNER_OPENUSD_TARGET)
  message(FATAL_ERROR "OpenUSD was requested but no usd CMake target was found")
endif()

add_executable(stage_runner main.cpp)
if(TARGET runnerSchema_schema)
  add_dependencies(stage_runner runnerSchema_schema)
endif()
target_compile_features(stage_runner PRIVATE cxx_std_17)
target_link_libraries(stage_runner PRIVATE runtimeCore::runtimeCore inputCore::inputCore
  physicsCore::physicsCore physicsJolt::physicsJolt inputSdl::inputSdl
  usd_stage_runner_warnings)

if(USD_STAGE_RUNNER_OPENUSD_TARGET)
  target_compile_definitions(stage_runner PRIVATE USD_STAGE_RUNNER_HAS_OPENUSD=1)
  if(USD_STAGE_RUNNER_SCHEMA_RESOURCE_PATH)
    target_compile_definitions(stage_runner PRIVATE
      "USD_STAGE_RUNNER_SCHEMA_RESOURCE_PATH=\"${USD_STAGE_RUNNER_SCHEMA_RESOURCE_PATH}\"")
  endif()
  target_link_libraries(stage_runner PRIVATE ${USD_STAGE_RUNNER_OPENUSD_TARGET}
    ${USD_STAGE_RUNNER_USDGEOM_TARGET} ${USD_STAGE_RUNNER_PLUG_TARGET})
  message(STATUS "stage_runner: OpenUSD support enabled (${USD_STAGE_RUNNER_OPENUSD_TARGET})")
else()
  target_compile_definitions(stage_runner PRIVATE USD_STAGE_RUNNER_HAS_OPENUSD=0)
  message(STATUS "stage_runner: OpenUSD not found; host will report that Stage loading is unavailable")
endif()

install(TARGETS stage_runner RUNTIME DESTINATION bin)
