if(NOT DEFINED EXECUTABLE OR NOT DEFINED STAGE OR NOT DEFINED OPTION OR
   NOT DEFINED EXPECTED)
  message(FATAL_ERROR "assert_command_fails.cmake is missing a required argument")
endif()

set(command "${EXECUTABLE}" "${STAGE}" "${OPTION}")
if(DEFINED VALUE)
  list(APPEND command "${VALUE}")
endif()

execute_process(
  COMMAND ${command}
  RESULT_VARIABLE result
  OUTPUT_VARIABLE stdout
  ERROR_VARIABLE stderr
)

if(result EQUAL 0)
  message(FATAL_ERROR "command unexpectedly succeeded\n${stdout}${stderr}")
endif()

set(output "${stdout}${stderr}")
if(NOT output MATCHES "${EXPECTED}")
  message(FATAL_ERROR
    "command failed without the expected diagnostic '${EXPECTED}'\n${output}"
  )
endif()
