cmake_minimum_required(VERSION 3.25)

project(physicsCore VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

if(NOT TARGET runtimeCore::runtimeCore)
  find_package(runtimeCore CONFIG REQUIRED)
endif()

add_library(physicsCore_warnings INTERFACE)
if(MSVC)
  target_compile_options(physicsCore_warnings INTERFACE /W4 /permissive-)
else()
  target_compile_options(physicsCore_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_library(physicsCore
  src/descriptors.cpp
  src/physics_runtime.cpp
  src/physics_world.cpp
)
add_library(physicsCore::physicsCore ALIAS physicsCore)
target_compile_features(physicsCore PUBLIC cxx_std_17)
target_include_directories(physicsCore PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(physicsCore PUBLIC runtimeCore::runtimeCore)
target_link_libraries(physicsCore PRIVATE $<BUILD_INTERFACE:physicsCore_warnings>)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/physicsCoreConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/physicsCoreConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/physicsCoreConfig.cmake"
  INSTALL_DESTINATION lib/cmake/physicsCore
)

install(TARGETS physicsCore EXPORT physicsCoreTargets)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT physicsCoreTargets FILE physicsCoreTargets.cmake NAMESPACE physicsCore::
  DESTINATION lib/cmake/physicsCore)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/physicsCoreConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/physicsCoreConfigVersion.cmake"
  DESTINATION lib/cmake/physicsCore
)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
