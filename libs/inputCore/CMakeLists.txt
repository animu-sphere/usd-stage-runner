cmake_minimum_required(VERSION 3.25)

project(inputCore VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

if(NOT TARGET runtimeCore::runtimeCore)
  find_package(runtimeCore CONFIG REQUIRED)
endif()

add_library(inputCore_warnings INTERFACE)
if(MSVC)
  target_compile_options(inputCore_warnings INTERFACE /W4 /permissive- /EHsc)
else()
  target_compile_options(inputCore_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_library(inputCore src/action_state.cpp src/movement_controller.cpp)
add_library(inputCore::inputCore ALIAS inputCore)
target_compile_features(inputCore PUBLIC cxx_std_17)
target_include_directories(inputCore PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(inputCore PUBLIC runtimeCore::runtimeCore)
target_link_libraries(inputCore PRIVATE $<BUILD_INTERFACE:inputCore_warnings>)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/inputCoreConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/inputCoreConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/inputCoreConfig.cmake"
  INSTALL_DESTINATION lib/cmake/inputCore
)

install(TARGETS inputCore EXPORT inputCoreTargets)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT inputCoreTargets FILE inputCoreTargets.cmake NAMESPACE inputCore::
  DESTINATION lib/cmake/inputCore)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/inputCoreConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/inputCoreConfigVersion.cmake"
  DESTINATION lib/cmake/inputCore
)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
