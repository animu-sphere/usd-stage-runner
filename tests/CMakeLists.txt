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

  add_test(
    NAME stage_runner.input_transform_sync
    COMMAND stage_runner
      ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/minimal.usda
      --frames 4
      --deterministic
      --move-x 1
  )
  set_tests_properties(stage_runner.input_transform_sync PROPERTIES
    PASS_REGULAR_EXPRESSION
      "fixed_steps=4, dropped_steps=0, synchronized_transforms=4, player_translation=\\(0.2, 1, 0\\)"
  )

  add_test(
    NAME stage_runner.float_transform_sync
    COMMAND stage_runner
      ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/float_translate.usda
      --frames 4
      --deterministic
      --move-x 1
  )
  set_tests_properties(stage_runner.float_transform_sync PROPERTIES
    PASS_REGULAR_EXPRESSION
      "fixed_steps=4, dropped_steps=0, synchronized_transforms=4, player_translation=\\(1.2, 1, 0\\)"
  )

  if(USD_STAGE_RUNNER_JOLT_AVAILABLE)
    add_test(
      NAME stage_runner.physics_falling_cube
      COMMAND stage_runner
        ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/falling_cube.usda
        --frames 180
        --deterministic
    )
    set_tests_properties(stage_runner.physics_falling_cube PROPERTIES
      PASS_REGULAR_EXPRESSION
        "fixed_steps=180, dropped_steps=0, synchronized_transforms=[1-9][0-9]*, player_translation=\\([^,]+, 0\\.48, [^)]+\\), physics_shapes=2, physics_bodies=2, physics_body_updates=[1-9][0-9]*"
    )

    add_test(
      NAME stage_runner.physics_vertical_slice
      COMMAND stage_runner
        ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/falling_cube.usda
        --frames 90
        --deterministic
        --move-x 1
    )
    set_tests_properties(stage_runner.physics_vertical_slice PROPERTIES
      PASS_REGULAR_EXPRESSION
        "fixed_steps=90, dropped_steps=0, synchronized_transforms=[1-9][0-9]*, player_translation=\\([1-9][0-9.]*, 0\\.48, [^)]+\\), physics_shapes=2, physics_bodies=2, physics_body_updates=[1-9][0-9]*"
    )

    add_test(
      NAME stage_runner.rejects_non_meter_physics_stage
      COMMAND ${CMAKE_COMMAND}
        -DEXECUTABLE=$<TARGET_FILE:stage_runner>
        -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/physics_centimeters.usda
        -DOPTION=--frames
        -DVALUE=1
        "-DEXPECTED=temporary physics import requires metersPerUnit = 1"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/assert_command_fails.cmake
    )

    add_test(
      NAME stage_runner.rejects_inherited_physics_transform
      COMMAND ${CMAKE_COMMAND}
        -DEXECUTABLE=$<TARGET_FILE:stage_runner>
        -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/physics_parent_transform.usda
        -DOPTION=--frames
        -DVALUE=1
        "-DEXPECTED=temporary physics import requires identity parent transforms or resetXformStack"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/assert_command_fails.cmake
    )
  else()
    add_test(
      NAME stage_runner.rejects_physics_stage_without_jolt
      COMMAND ${CMAKE_COMMAND}
        -DEXECUTABLE=$<TARGET_FILE:stage_runner>
        -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/falling_cube.usda
        -DOPTION=--frames
        -DVALUE=1
        "-DEXPECTED=Stage declares physics bodies, but Jolt Physics is unavailable"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/assert_command_fails.cmake
    )
  endif()
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

  add_test(
    NAME stage_runner.rejects_out_of_range_move_axis
    COMMAND ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:stage_runner>
      -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/minimal.usda
      -DOPTION=--move-x
      -DVALUE=2
      "-DEXPECTED=--move-x requires a number from -1 to 1"
      -P ${CMAKE_CURRENT_SOURCE_DIR}/assert_command_fails.cmake
  )

  add_test(
    NAME stage_runner.rejects_move_axis_without_deterministic
    COMMAND ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:stage_runner>
      -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/minimal.usda
      -DOPTION=--move-x
      -DVALUE=1
      "-DEXPECTED=--move-x and --move-y require --deterministic"
      -P ${CMAKE_CURRENT_SOURCE_DIR}/assert_command_fails.cmake
  )
endif()
