add_executable(stage_runtime_session_tests stage_session_tests.cpp)
target_compile_features(stage_runtime_session_tests PRIVATE cxx_std_17)
target_link_libraries(stage_runtime_session_tests PRIVATE
  stageRuntime::stageRuntime
  $<BUILD_INTERFACE:stageRuntime_warnings>
)
add_test(NAME stage_runtime.session COMMAND stage_runtime_session_tests)

add_executable(stage_runtime_physics_tests physics_runtime_tests.cpp)
target_compile_features(stage_runtime_physics_tests PRIVATE cxx_std_17)
target_link_libraries(stage_runtime_physics_tests PRIVATE
  stageRuntime::stageRuntime
  $<BUILD_INTERFACE:stageRuntime_warnings>
)
add_test(NAME stage_runtime.physics_bridge COMMAND stage_runtime_physics_tests)
if(USD_STAGE_RUNNER_ENABLE_PHYSICS_USD AND USD_STAGE_RUNNER_JOLT_AVAILABLE)
  add_executable(stage_runtime_standard_physics_tests standard_physics_tests.cpp)
  target_link_libraries(stage_runtime_standard_physics_tests PRIVATE
    stageRuntime::stageRuntime physicsJolt::physicsJolt ${USD_STAGE_RUNNER_PLUG_TARGET})
  target_compile_definitions(stage_runtime_standard_physics_tests PRIVATE
    TEST_SCHEMA_PATH="${USD_STAGE_RUNNER_SCHEMA_RESOURCE_PATH}"
    TEST_FIXTURE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../../../tests/fixtures")
  add_test(NAME stage_runtime.standard_physics_parity COMMAND stage_runtime_standard_physics_tests)
endif()
if(WIN32)
  set(_stage_runtime_test_paths
    "PATH=path_list_prepend:${pxr_DIR}/lib"
    "PATH=path_list_prepend:${pxr_DIR}/bin"
  )
  if(Python3_RUNTIME_LIBRARY_DIRS)
    list(APPEND _stage_runtime_test_paths
      "PATH=path_list_prepend:${Python3_RUNTIME_LIBRARY_DIRS}")
  endif()
  set_tests_properties(stage_runtime.session stage_runtime.physics_bridge PROPERTIES
    ENVIRONMENT_MODIFICATION "${_stage_runtime_test_paths}"
  )
  if(TEST stage_runtime.standard_physics_parity)
    set_tests_properties(stage_runtime.standard_physics_parity PROPERTIES
      ENVIRONMENT_MODIFICATION "${_stage_runtime_test_paths}")
  endif()
endif()
