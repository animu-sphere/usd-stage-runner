cmake_minimum_required(VERSION 3.25)

project(physicsJolt VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

if(NOT TARGET physicsCore::physicsCore)
  find_package(physicsCore CONFIG REQUIRED)
endif()

option(USD_STAGE_RUNNER_REQUIRE_JOLT "Fail configuration when Jolt Physics is unavailable" OFF)

add_library(physicsJolt_warnings INTERFACE)
if(MSVC)
  target_compile_options(physicsJolt_warnings INTERFACE /W4 /permissive-)
else()
  target_compile_options(physicsJolt_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

set(PHYSICS_JOLT_BACKEND "none")
set(PHYSICS_JOLT_BACKEND_TARGET "")
find_package(Jolt CONFIG QUIET)
if(TARGET Jolt::Jolt)
  set(PHYSICS_JOLT_BACKEND "Jolt::Jolt")
  set(PHYSICS_JOLT_BACKEND_TARGET Jolt::Jolt)
elseif(TARGET Jolt)
  set(PHYSICS_JOLT_BACKEND "Jolt")
  set(PHYSICS_JOLT_BACKEND_TARGET Jolt)
endif()

if(USD_STAGE_RUNNER_REQUIRE_JOLT AND NOT PHYSICS_JOLT_BACKEND_TARGET)
  message(FATAL_ERROR "Jolt Physics was requested but no Jolt CMake target was found")
endif()

if(PHYSICS_JOLT_BACKEND_TARGET)
  set(PHYSICS_JOLT_SOURCE src/jolt_physics_world.cpp)
else()
  set(PHYSICS_JOLT_SOURCE src/unavailable_jolt_physics_world.cpp)
endif()

add_library(physicsJolt ${PHYSICS_JOLT_SOURCE})
add_library(physicsJolt::physicsJolt ALIAS physicsJolt)
target_compile_features(physicsJolt PUBLIC cxx_std_17)
target_include_directories(physicsJolt PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(physicsJolt PUBLIC physicsCore::physicsCore)
target_link_libraries(physicsJolt PRIVATE $<BUILD_INTERFACE:physicsJolt_warnings>)
if(PHYSICS_JOLT_BACKEND_TARGET)
  target_link_libraries(physicsJolt PRIVATE ${PHYSICS_JOLT_BACKEND_TARGET})
endif()
message(STATUS "physicsJolt: backend ${PHYSICS_JOLT_BACKEND}")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/physicsJoltConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/physicsJoltConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/physicsJoltConfig.cmake"
  INSTALL_DESTINATION lib/cmake/physicsJolt
)
install(TARGETS physicsJolt EXPORT physicsJoltTargets)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT physicsJoltTargets FILE physicsJoltTargets.cmake NAMESPACE physicsJolt::
  DESTINATION lib/cmake/physicsJolt)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/physicsJoltConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/physicsJoltConfigVersion.cmake"
  DESTINATION lib/cmake/physicsJolt
)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
