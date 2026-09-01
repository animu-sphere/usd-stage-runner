add_executable(stage_runtime_session_tests stage_session_tests.cpp)
target_compile_features(stage_runtime_session_tests PRIVATE cxx_std_17)
target_link_libraries(stage_runtime_session_tests PRIVATE
  stageRuntime::stageRuntime
  $<BUILD_INTERFACE:stageRuntime_warnings>
)
add_test(NAME stage_runtime.session COMMAND stage_runtime_session_tests)
if(WIN32)
  set(_stage_runtime_test_paths
    "PATH=path_list_prepend:${pxr_DIR}/lib"
    "PATH=path_list_prepend:${pxr_DIR}/bin"
  )
  if(Python3_RUNTIME_LIBRARY_DIRS)
    list(APPEND _stage_runtime_test_paths
      "PATH=path_list_prepend:${Python3_RUNTIME_LIBRARY_DIRS}")
  endif()
  set_tests_properties(stage_runtime.session PROPERTIES
    ENVIRONMENT_MODIFICATION "${_stage_runtime_test_paths}"
  )
endif()
