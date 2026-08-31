if(NOT DEFINED BUILD_DIR OR NOT DEFINED INSTALL_DIR OR
   NOT DEFINED EXECUTABLE_RELATIVE_PATH OR NOT DEFINED STAGE)
  message(FATAL_ERROR "installed stage_runner test is missing a required argument")
endif()

file(REMOVE_RECURSE "${INSTALL_DIR}")

set(_install_command "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${INSTALL_DIR}")
if(DEFINED CONFIG AND NOT CONFIG STREQUAL "")
  list(APPEND _install_command --config "${CONFIG}")
endif()
execute_process(
  COMMAND ${_install_command}
  RESULT_VARIABLE _install_result
  OUTPUT_VARIABLE _install_stdout
  ERROR_VARIABLE _install_stderr
)
if(NOT _install_result EQUAL 0)
  message(FATAL_ERROR
    "stage_runner install failed (${_install_result})\n${_install_stdout}\n${_install_stderr}")
endif()

set(_executable "${INSTALL_DIR}/${EXECUTABLE_RELATIVE_PATH}")
execute_process(
  COMMAND "${_executable}" "${STAGE}" --frames 1 --deterministic
  RESULT_VARIABLE _run_result
  OUTPUT_VARIABLE _run_stdout
  ERROR_VARIABLE _run_stderr
)
if(NOT _run_result EQUAL 0)
  message(FATAL_ERROR
    "installed stage_runner failed (${_run_result})\n${_run_stdout}\n${_run_stderr}")
endif()
if(NOT _run_stdout MATCHES "physics_shapes=2, physics_bodies=2")
  message(FATAL_ERROR "installed stage_runner output was unexpected:\n${_run_stdout}")
endif()
