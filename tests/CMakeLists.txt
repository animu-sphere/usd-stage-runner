if(TARGET stage_runner AND USD_STAGE_RUNNER_OPENUSD_TARGET)
  add_test(
    NAME stage_runner.minimal_stage
    COMMAND stage_runner
      ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/minimal.usda
      --frames 4
      --deterministic
  )
  set_tests_properties(stage_runner.minimal_stage PROPERTIES
    PASS_REGULAR_EXPRESSION "with 4 prims.*frames=4, fixed_steps=4, dropped_steps=0"
  )
endif()

if(TARGET stage_runner)
  add_test(
    NAME stage_runner.rejects_negative_frames
    COMMAND ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:stage_runner>
      -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/minimal.usda
      -DOPTION=--frames
      -DVALUE=-1
      "-DEXPECTED=--frames requires a positive integer"
      -P ${CMAKE_CURRENT_SOURCE_DIR}/assert_command_fails.cmake
  )

  add_test(
    NAME stage_runner.rejects_non_finite_fixed_dt
    COMMAND ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:stage_runner>
      -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/minimal.usda
      -DOPTION=--fixed-dt
      -DVALUE=nan
      "-DEXPECTED=--fixed-dt requires a positive number"
      -P ${CMAKE_CURRENT_SOURCE_DIR}/assert_command_fails.cmake
  )
endif()
