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

  add_test(
    NAME stage_runner.camera_schema_import
    COMMAND stage_runner
      ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/camera_import.usda
      --frames 1
      --deterministic
  )
  set_tests_properties(stage_runner.camera_schema_import PROPERTIES
    PASS_REGULAR_EXPRESSION "camera_rigs=1"
  )

  add_test(
    NAME stage_runner.rejects_legacy_camera_attributes
    COMMAND ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:stage_runner>
      -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/legacy_camera_attributes.usda
      -DOPTION=--frames
      -DVALUE=1
      "-DEXPECTED=authored camera rig properties require RunnerCameraRigAPI"
      -P ${CMAKE_CURRENT_SOURCE_DIR}/assert_command_fails.cmake
  )

  add_test(
    NAME stage_runner.rejects_camera_missing_target
    COMMAND ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:stage_runner>
      -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/camera_missing_target.usda
      -DOPTION=--frames
      -DVALUE=1
      "-DEXPECTED=runner:camera:target on /Camera must target exactly one prim"
      -P ${CMAKE_CURRENT_SOURCE_DIR}/assert_command_fails.cmake
  )

  add_test(
    NAME stage_runner.rejects_camera_schema_on_non_camera
    COMMAND ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:stage_runner>
      -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/camera_non_camera.usda
      -DOPTION=--frames
      -DVALUE=1
      "-DEXPECTED=RunnerCameraRigAPI requires a UsdGeomCamera prim"
      -P ${CMAKE_CURRENT_SOURCE_DIR}/assert_command_fails.cmake
  )

  add_test(
    NAME stage_runner.rejects_invalid_camera_config
    COMMAND ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:stage_runner>
      -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/camera_invalid_config.usda
      -DOPTION=--frames
      -DVALUE=1
      "-DEXPECTED=invalid camera rig configuration on /Camera"
      -P ${CMAKE_CURRENT_SOURCE_DIR}/assert_command_fails.cmake
  )

  add_test(
    NAME stage_runner.rejects_legacy_physics_attributes
    COMMAND ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:stage_runner>
      -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/legacy_physics_attributes.usda
      -DOPTION=--frames
      -DVALUE=1
      "-DEXPECTED=authored body attributes require RunnerPhysicsBodyAPI"
      -P ${CMAKE_CURRENT_SOURCE_DIR}/assert_command_fails.cmake
  )

  add_test(
    NAME stage_runner.rejects_incomplete_physics_schemas
    COMMAND ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:stage_runner>
      -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/incomplete_physics_schemas.usda
      -DOPTION=--frames
      -DVALUE=1
      "-DEXPECTED=physics prim must apply both RunnerPhysicsBodyAPI and RunnerColliderAPI"
      -P ${CMAKE_CURRENT_SOURCE_DIR}/assert_command_fails.cmake
  )

  add_test(
    NAME stage_runner.rejects_legacy_character_attributes
    COMMAND ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:stage_runner>
      -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/legacy_character_attributes.usda
      -DOPTION=--frames
      -DVALUE=1
      "-DEXPECTED=authored character attributes require RunnerCharacterAPI"
      -P ${CMAKE_CURRENT_SOURCE_DIR}/assert_command_fails.cmake
  )

  add_test(
    NAME stage_runner.rejects_incomplete_character_schema
    COMMAND ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:stage_runner>
      -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/incomplete_character_schema.usda
      -DOPTION=--frames
      -DVALUE=1
      "-DEXPECTED=character prim must apply RunnerPhysicsBodyAPI and RunnerColliderAPI"
      -P ${CMAKE_CURRENT_SOURCE_DIR}/assert_command_fails.cmake
  )

  add_test(
    NAME stage_runner.rejects_static_character
    COMMAND ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:stage_runner>
      -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/static_character.usda
      -DOPTION=--frames
      -DVALUE=1
      "-DEXPECTED=RunnerCharacterAPI requires a dynamic physics body"
      -P ${CMAKE_CURRENT_SOURCE_DIR}/assert_command_fails.cmake
  )

  if(USD_STAGE_RUNNER_JOLT_AVAILABLE)
    add_test(
      NAME stage_runner.character_schema_import
      COMMAND stage_runner
        ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/character_import.usda
        --frames 1
        --deterministic
    )
    set_tests_properties(stage_runner.character_schema_import PROPERTIES
      PASS_REGULAR_EXPRESSION
        "physics_shapes=1, physics_bodies=1, physics_body_updates=[0-9]+, character_controllers=1"
    )

    add_test(
      NAME stage_runner.character_walk_and_ground
      COMMAND stage_runner
        ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/character_walk.usda
        --frames 60
        --deterministic
        --move-x 1
    )
    set_tests_properties(stage_runner.character_walk_and_ground PROPERTIES
      PASS_REGULAR_EXPRESSION
        "fixed_steps=60, dropped_steps=0, synchronized_transforms=[1-9][0-9]*, player_translation=\\([1-9][0-9.]*, 0\\.5[0-9]*, [^)]+\\).*character_controllers=1, character_grounded=true, character_jump_state=grounded"
    )

    add_test(
      NAME stage_runner.character_jump
      COMMAND stage_runner
        ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/character_walk.usda
        --frames 30
        --deterministic
        --move-x 1
        --jump
    )
    set_tests_properties(stage_runner.character_jump PROPERTIES
      PASS_REGULAR_EXPRESSION
        "fixed_steps=30, dropped_steps=0, synchronized_transforms=[1-9][0-9]*, player_translation=\\([1-9][0-9.]*, [1-9][0-9.]*, [^)]+\\).*character_controllers=1, character_grounded=false, character_jump_state=rising"
    )

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
      NAME stage_runner.installed_physics_schema_defaults
      COMMAND ${CMAKE_COMMAND}
        -DBUILD_DIR=${CMAKE_BINARY_DIR}
        -DINSTALL_DIR=${CMAKE_BINARY_DIR}/tests/install-smoke
        -DCONFIG=$<CONFIG>
        -DEXECUTABLE_RELATIVE_PATH=${CMAKE_INSTALL_BINDIR}/stage_runner${CMAKE_EXECUTABLE_SUFFIX}
        -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/falling_cube.usda
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_installed_stage_runner.cmake
    )

    add_test(
      NAME stage_runner.rejects_non_meter_physics_stage
      COMMAND ${CMAKE_COMMAND}
        -DEXECUTABLE=$<TARGET_FILE:stage_runner>
        -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/physics_centimeters.usda
        -DOPTION=--frames
        -DVALUE=1
        "-DEXPECTED=physics schema import requires metersPerUnit = 1"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/assert_command_fails.cmake
    )

    add_test(
      NAME stage_runner.rejects_inherited_physics_transform
      COMMAND ${CMAKE_COMMAND}
        -DEXECUTABLE=$<TARGET_FILE:stage_runner>
        -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/physics_parent_transform.usda
        -DOPTION=--frames
        -DVALUE=1
        "-DEXPECTED=physics schema import requires identity parent transforms or resetXformStack"
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

  add_test(
    NAME stage_runner.rejects_jump_without_deterministic
    COMMAND ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:stage_runner>
      -DSTAGE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/minimal.usda
      -DOPTION=--jump
      "-DEXPECTED=--jump requires --deterministic"
      -P ${CMAKE_CURRENT_SOURCE_DIR}/assert_command_fails.cmake
  )
endif()
