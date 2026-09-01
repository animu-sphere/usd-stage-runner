cmake_minimum_required(VERSION 3.25)

project(
  stageRuntime
  VERSION 0.1.0
  DESCRIPTION "Reusable OpenUSD Stage play-session runtime"
  LANGUAGES CXX
)

include(CTest)

if(NOT TARGET runtimeCore::runtimeCore)
  find_package(runtimeCore CONFIG REQUIRED)
endif()
if(NOT TARGET inputCore::inputCore)
  find_package(inputCore CONFIG REQUIRED)
endif()
if(NOT TARGET physicsCore::physicsCore)
  find_package(physicsCore CONFIG REQUIRED)
endif()
if(NOT TARGET characterCore::characterCore)
  find_package(characterCore CONFIG REQUIRED)
endif()
if(NOT TARGET cameraCore::cameraCore)
  find_package(cameraCore CONFIG REQUIRED)
endif()

if(NOT USD_STAGE_RUNNER_OPENUSD_TARGET OR NOT USD_STAGE_RUNNER_USDGEOM_TARGET)
  find_package(pxr CONFIG REQUIRED)
  foreach(candidate IN ITEMS usd pxr::usd)
    if(TARGET ${candidate})
      set(USD_STAGE_RUNNER_OPENUSD_TARGET ${candidate})
      break()
    endif()
  endforeach()
  foreach(candidate IN ITEMS usdGeom pxr::usdGeom)
    if(TARGET ${candidate})
      set(USD_STAGE_RUNNER_USDGEOM_TARGET ${candidate})
      break()
    endif()
  endforeach()
endif()
if(NOT USD_STAGE_RUNNER_OPENUSD_TARGET OR NOT USD_STAGE_RUNNER_USDGEOM_TARGET)
  message(FATAL_ERROR "stageRuntime requires OpenUSD usd and usdGeom CMake targets")
endif()

add_library(stageRuntime_warnings INTERFACE)
if(MSVC)
  target_compile_options(stageRuntime_warnings INTERFACE /W4 /permissive- /EHsc)
else()
  target_compile_options(stageRuntime_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_library(stageRuntime
  src/stage_session.cpp
)
add_library(stageRuntime::stageRuntime ALIAS stageRuntime)

target_compile_features(stageRuntime PUBLIC cxx_std_17)
target_include_directories(stageRuntime
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(stageRuntime
  PUBLIC
    runtimeCore::runtimeCore
    inputCore::inputCore
    physicsCore::physicsCore
    characterCore::characterCore
    cameraCore::cameraCore
    ${USD_STAGE_RUNNER_OPENUSD_TARGET}
    ${USD_STAGE_RUNNER_USDGEOM_TARGET}
  PRIVATE
    $<BUILD_INTERFACE:stageRuntime_warnings>
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/stageRuntimeConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/stageRuntimeConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/stageRuntimeConfig.cmake"
  INSTALL_DESTINATION lib/cmake/stageRuntime
)

install(TARGETS stageRuntime EXPORT stageRuntimeTargets)
install(DIRECTORY include/ DESTINATION include)
install(
  EXPORT stageRuntimeTargets
  FILE stageRuntimeTargets.cmake
  NAMESPACE stageRuntime::
  DESTINATION lib/cmake/stageRuntime
)
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/stageRuntimeConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/stageRuntimeConfigVersion.cmake"
  DESTINATION lib/cmake/stageRuntime
)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
