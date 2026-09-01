add_executable(stage_runner main.cpp)
if(TARGET runnerSchema_schema)
  add_dependencies(stage_runner runnerSchema_schema)
endif()
target_compile_features(stage_runner PRIVATE cxx_std_17)
target_link_libraries(stage_runner PRIVATE runtimeCore::runtimeCore inputCore::inputCore
  physicsCore::physicsCore characterCore::characterCore cameraCore::cameraCore
  physicsJolt::physicsJolt inputSdl::inputSdl usd_stage_runner_warnings)

if(USD_STAGE_RUNNER_OPENUSD_TARGET)
  target_compile_definitions(stage_runner PRIVATE USD_STAGE_RUNNER_HAS_OPENUSD=1)
  if(USD_STAGE_RUNNER_SCHEMA_RESOURCE_PATH)
    file(RELATIVE_PATH _schema_resource_relative_path
      "/${CMAKE_INSTALL_BINDIR}"
      "/${USD_STAGE_RUNNER_SCHEMA_INSTALL_DIR}")
    target_compile_definitions(stage_runner PRIVATE
      "USD_STAGE_RUNNER_SCHEMA_RESOURCE_RELATIVE_PATH=\"${_schema_resource_relative_path}\"")
    add_custom_command(TARGET stage_runner POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E make_directory
        "$<TARGET_FILE_DIR:stage_runner>/${_schema_resource_relative_path}"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${USD_STAGE_RUNNER_SCHEMA_RESOURCE_PATH}/plugInfo.json"
        "${USD_STAGE_RUNNER_SCHEMA_RESOURCE_PATH}/generatedSchema.usda"
        "$<TARGET_FILE_DIR:stage_runner>/${_schema_resource_relative_path}"
      COMMENT "Staging runnerSchema resources beside stage_runner"
      VERBATIM
    )
  endif()
  target_link_libraries(stage_runner PRIVATE stageRuntime::stageRuntime
    ${USD_STAGE_RUNNER_PLUG_TARGET})
  message(STATUS "stage_runner: OpenUSD support enabled (${USD_STAGE_RUNNER_OPENUSD_TARGET})")
else()
  target_compile_definitions(stage_runner PRIVATE USD_STAGE_RUNNER_HAS_OPENUSD=0)
  message(STATUS "stage_runner: OpenUSD not found; host will report that Stage loading is unavailable")
endif()

install(TARGETS stage_runner RUNTIME DESTINATION bin)
