cmake_minimum_required(VERSION 3.25)

project(cameraCore VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

if(NOT TARGET runtimeCore::runtimeCore)
  find_package(runtimeCore CONFIG REQUIRED)
endif()

add_library(cameraCore_warnings INTERFACE)
if(MSVC)
  target_compile_options(cameraCore_warnings INTERFACE /W4 /permissive- /EHsc)
else()
  target_compile_options(cameraCore_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_library(cameraCore src/camera_rig.cpp)
add_library(cameraCore::cameraCore ALIAS cameraCore)
target_compile_features(cameraCore PUBLIC cxx_std_17)
target_include_directories(cameraCore PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(cameraCore PUBLIC runtimeCore::runtimeCore)
target_link_libraries(cameraCore PRIVATE $<BUILD_INTERFACE:cameraCore_warnings>)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/cameraCoreConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cameraCoreConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cameraCoreConfig.cmake"
  INSTALL_DESTINATION lib/cmake/cameraCore
)

install(TARGETS cameraCore EXPORT cameraCoreTargets)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT cameraCoreTargets FILE cameraCoreTargets.cmake
  NAMESPACE cameraCore:: DESTINATION lib/cmake/cameraCore)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/cameraCoreConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/cameraCoreConfigVersion.cmake"
  DESTINATION lib/cmake/cameraCore
)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
