cmake_minimum_required(VERSION 3.20)

project(runnerSchema_schema NONE)

if(NOT DEFINED USD_STAGE_RUNNER_SCHEMA_INSTALL_DIR)
  set(USD_STAGE_RUNNER_SCHEMA_INSTALL_DIR
    "share/usd-stage-runner/runnerSchema")
endif()

set(_schema_source "${CMAKE_CURRENT_SOURCE_DIR}/schema.usda")
set(_schema_resources "${CMAKE_CURRENT_SOURCE_DIR}/plugin/resources/runnerSchema")

# The standalone host registers the in-tree resources explicitly. OpenStrata
# bundles discover the same directory through the plugin manifest.
if(NOT PROJECT_IS_TOP_LEVEL)
  set(USD_STAGE_RUNNER_SCHEMA_RESOURCE_PATH "${_schema_resources}" PARENT_SCOPE)
endif()

install(FILES
  "${_schema_resources}/plugInfo.json"
  "${_schema_resources}/generatedSchema.usda"
  DESTINATION "${USD_STAGE_RUNNER_SCHEMA_INSTALL_DIR}"
)

find_program(USD_GEN_SCHEMA usdGenSchema)
find_program(USD_SCHEMA_PYTHON NAMES python python3)
set(_can_regenerate_schema FALSE)
if(USD_GEN_SCHEMA AND USD_SCHEMA_PYTHON)
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
      "PYTHONUTF8=1"
      "PYTHONIOENCODING=utf-8"
      "${USD_SCHEMA_PYTHON}" -c "from pxr import Sdf"
    RESULT_VARIABLE _pxr_import_result
    OUTPUT_QUIET
    ERROR_QUIET
  )
  if(_pxr_import_result EQUAL 0)
    set(_can_regenerate_schema TRUE)
  endif()
endif()

if(_can_regenerate_schema)
  add_custom_target(runnerSchema_schema ALL
    COMMAND "${CMAKE_COMMAND}" -E env
      "PYTHONUTF8=1"
      "PYTHONIOENCODING=utf-8"
      "${USD_SCHEMA_PYTHON}" "${USD_GEN_SCHEMA}" "${_schema_source}" "${_schema_resources}"
    BYPRODUCTS
      "${_schema_resources}/plugInfo.json"
      "${_schema_resources}/generatedSchema.usda"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Generating runnerSchema codeless USD resources"
    VERBATIM
  )
else()
  message(STATUS
    "runnerSchema: compatible usdGenSchema/Python unavailable; using committed schema resources")
endif()
