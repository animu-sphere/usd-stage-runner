cmake_minimum_required(VERSION 3.25)

project(
  usdviewStageRunner
  VERSION 0.1.0
  DESCRIPTION "usdview adapter for the shared Stage Runner play session"
  LANGUAGES CXX
)

set(USD_STAGE_RUNNER_OST_PLUGIN_BUNDLE_DIR "" CACHE PATH
  "OpenStrata bundle root that receives the usdview adapter for `ost plugin view`")

if(NOT TARGET stageRuntime::stageRuntime)
  find_package(stageRuntime CONFIG REQUIRED)
endif()
if(NOT TARGET physicsJolt::physicsJolt)
  find_package(physicsJolt CONFIG REQUIRED)
endif()
if(NOT TARGET python OR NOT TARGET tf)
  find_package(pxr CONFIG REQUIRED)
endif()
if(NOT TARGET python OR NOT TARGET tf)
  message(FATAL_ERROR "usdviewStageRunner requires an OpenUSD build with Python support")
endif()

set(_python_package_dir "${CMAKE_CURRENT_BINARY_DIR}/python/usdviewStageRunner")
file(MAKE_DIRECTORY "${_python_package_dir}")
foreach(_python_file IN ITEMS __init__.py controller.py)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/python/usdviewStageRunner/${_python_file}"
    "${_python_package_dir}/${_python_file}"
    COPYONLY
  )
endforeach()
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/plugin/plugInfo.json"
  "${_python_package_dir}/plugInfo.json"
  COPYONLY
)

add_library(usdviewStageRunnerPython MODULE src/wrap_session.cpp)
set_target_properties(usdviewStageRunnerPython PROPERTIES
  PREFIX ""
  OUTPUT_NAME "_usdviewStageRunner"
  LIBRARY_OUTPUT_DIRECTORY "${_python_package_dir}"
  RUNTIME_OUTPUT_DIRECTORY "${_python_package_dir}"
)
if(WIN32)
  set_target_properties(usdviewStageRunnerPython PROPERTIES SUFFIX ".pyd")
endif()
target_compile_features(usdviewStageRunnerPython PRIVATE cxx_std_17)
target_link_libraries(usdviewStageRunnerPython PRIVATE
  stageRuntime::stageRuntime
  physicsJolt::physicsJolt
  python
  tf
  usd_stage_runner_warnings
)

if(TARGET runnerSchema_schema)
  add_dependencies(usdviewStageRunnerPython runnerSchema_schema)
endif()
if(USD_STAGE_RUNNER_SCHEMA_RESOURCE_PATH)
  add_custom_command(TARGET usdviewStageRunnerPython POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory
      "${_python_package_dir}/resources/runnerSchema"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${USD_STAGE_RUNNER_SCHEMA_RESOURCE_PATH}/plugInfo.json"
      "${USD_STAGE_RUNNER_SCHEMA_RESOURCE_PATH}/generatedSchema.usda"
      "${_python_package_dir}/resources/runnerSchema"
    COMMENT "Staging runnerSchema resources with the usdview plugin"
    VERBATIM
  )
endif()

if(USD_STAGE_RUNNER_OST_PLUGIN_BUNDLE_DIR)
  set(_ost_bundle_dir "${USD_STAGE_RUNNER_OST_PLUGIN_BUNDLE_DIR}")
  if(NOT IS_ABSOLUTE "${_ost_bundle_dir}")
    cmake_path(ABSOLUTE_PATH _ost_bundle_dir
      BASE_DIRECTORY "${CMAKE_SOURCE_DIR}"
      NORMALIZE
    )
  endif()
  if(NOT EXISTS "${_ost_bundle_dir}/openstrata.plugin.yaml")
    message(FATAL_ERROR
      "USD_STAGE_RUNNER_OST_PLUGIN_BUNDLE_DIR must name an OpenStrata plugin bundle: ${_ost_bundle_dir}")
  endif()

  set(_ost_python_package_dir "${_ost_bundle_dir}/python/usdviewStageRunner")
  add_custom_target(usdviewStageRunnerOstPluginView ALL
    COMMAND "${CMAKE_COMMAND}" -E make_directory
      "${_ost_python_package_dir}"
      "${_ost_python_package_dir}/resources/runnerSchema"
    COMMAND "${CMAKE_COMMAND}" -E rm -f
      "${_ost_python_package_dir}/_usdviewStageRunner.pyd"
      "${_ost_python_package_dir}/_usdviewStageRunner.so"
      "${_ost_python_package_dir}/_usdviewStageRunner.dylib"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/python/usdviewStageRunner/__init__.py"
      "${CMAKE_CURRENT_SOURCE_DIR}/python/usdviewStageRunner/controller.py"
      "${CMAKE_CURRENT_SOURCE_DIR}/plugin/plugInfo.json"
      "$<TARGET_FILE:usdviewStageRunnerPython>"
      "${_ost_python_package_dir}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${USD_STAGE_RUNNER_SCHEMA_RESOURCE_PATH}/plugInfo.json"
      "${USD_STAGE_RUNNER_SCHEMA_RESOURCE_PATH}/generatedSchema.usda"
      "${_ost_python_package_dir}/resources/runnerSchema"
    DEPENDS usdviewStageRunnerPython
    COMMENT "Staging usdviewStageRunner for OpenStrata plugin view"
    VERBATIM
  )
endif()

set(_usdview_install_dir "${CMAKE_INSTALL_LIBDIR}/usd-stage-runner/usdview")
install(TARGETS usdviewStageRunnerPython
  LIBRARY DESTINATION "${_usdview_install_dir}/usdviewStageRunner"
  RUNTIME DESTINATION "${_usdview_install_dir}/usdviewStageRunner"
)
install(FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/python/usdviewStageRunner/__init__.py"
  "${CMAKE_CURRENT_SOURCE_DIR}/python/usdviewStageRunner/controller.py"
  "${CMAKE_CURRENT_SOURCE_DIR}/plugin/plugInfo.json"
  DESTINATION "${_usdview_install_dir}/usdviewStageRunner"
)
if(USD_STAGE_RUNNER_SCHEMA_RESOURCE_PATH)
  install(FILES
    "${USD_STAGE_RUNNER_SCHEMA_RESOURCE_PATH}/plugInfo.json"
    "${USD_STAGE_RUNNER_SCHEMA_RESOURCE_PATH}/generatedSchema.usda"
    DESTINATION "${_usdview_install_dir}/usdviewStageRunner/resources/runnerSchema"
  )
endif()

if(BUILD_TESTING AND Python3_EXECUTABLE)
  get_filename_component(_python_runtime_dir "${Python3_EXECUTABLE}" DIRECTORY)
  add_test(
    NAME usdviewStageRunner.native_session
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/native_session_smoke.py"
      "${CMAKE_SOURCE_DIR}/tests/fixtures/minimal.usda"
  )
  set_tests_properties(usdviewStageRunner.native_session PROPERTIES
    ENVIRONMENT_MODIFICATION
      "PYTHONPATH=path_list_prepend:${CMAKE_CURRENT_BINARY_DIR}/python;PYTHONPATH=path_list_prepend:${pxr_DIR}/lib/python;PATH=path_list_prepend:$<TARGET_FILE_DIR:usdviewStageRunnerPython>;PATH=path_list_prepend:${pxr_DIR}/lib;PATH=path_list_prepend:${pxr_DIR}/bin;PATH=path_list_prepend:${_python_runtime_dir}"
  )

  add_test(
    NAME usdviewStageRunner.python_sources
    COMMAND "${Python3_EXECUTABLE}" -m py_compile
      "${CMAKE_CURRENT_SOURCE_DIR}/python/usdviewStageRunner/__init__.py"
      "${CMAKE_CURRENT_SOURCE_DIR}/python/usdviewStageRunner/controller.py"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/controller_smoke.py"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/native_session_smoke.py"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/ost_plugin_view_smoke.py"
  )

  add_test(
    NAME usdviewStageRunner.controller
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/controller_smoke.py"
  )

  if(USD_STAGE_RUNNER_OST_PLUGIN_BUNDLE_DIR)
    add_test(
      NAME usdviewStageRunner.ost_plugin_view
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/ost_plugin_view_smoke.py"
        "${_ost_bundle_dir}/plugin/resources/runnerSchema"
        "${CMAKE_SOURCE_DIR}/tests/fixtures/minimal.usda"
    )
    set_tests_properties(usdviewStageRunner.ost_plugin_view PROPERTIES
      ENVIRONMENT_MODIFICATION
        "PYTHONPATH=path_list_prepend:${_ost_bundle_dir}/python;PYTHONPATH=path_list_prepend:${pxr_DIR}/lib/python;PATH=path_list_prepend:${_ost_python_package_dir};PATH=path_list_prepend:${pxr_DIR}/lib;PATH=path_list_prepend:${pxr_DIR}/bin;PATH=path_list_prepend:${_python_runtime_dir};PXR_PLUGINPATH_NAME=path_list_prepend:${_ost_bundle_dir}/plugin/resources/runnerSchema;PYTHONDONTWRITEBYTECODE=set:1"
    )
  endif()
endif()
