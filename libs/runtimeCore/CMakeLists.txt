cmake_minimum_required(VERSION 3.25)

project(
  runtimeCore
  VERSION 0.1.0
  DESCRIPTION "Backend-neutral runtime primitives for usd-stage-runner"
  LANGUAGES CXX
)

include(CTest)

add_library(runtimeCore_warnings INTERFACE)
if(MSVC)
  target_compile_options(runtimeCore_warnings INTERFACE /W4 /permissive- /EHsc)
else()
  target_compile_options(runtimeCore_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_library(runtimeCore
  src/dirty_prim_queue.cpp
  src/fixed_step_accumulator.cpp
  src/frame_clock.cpp
  src/runtime_world.cpp
)
add_library(runtimeCore::runtimeCore ALIAS runtimeCore)

target_compile_features(runtimeCore PUBLIC cxx_std_17)
target_include_directories(runtimeCore
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(runtimeCore PRIVATE $<BUILD_INTERFACE:runtimeCore_warnings>)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/runtimeCoreConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/runtimeCoreConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/runtimeCoreConfig.cmake"
  INSTALL_DESTINATION lib/cmake/runtimeCore
)

install(TARGETS runtimeCore EXPORT runtimeCoreTargets)
install(DIRECTORY include/ DESTINATION include)
install(
  EXPORT runtimeCoreTargets
  FILE runtimeCoreTargets.cmake
  NAMESPACE runtimeCore::
  DESTINATION lib/cmake/runtimeCore
)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/runtimeCoreConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/runtimeCoreConfigVersion.cmake"
  DESTINATION lib/cmake/runtimeCore
)
