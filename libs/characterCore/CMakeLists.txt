cmake_minimum_required(VERSION 3.25)

project(characterCore VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

if(NOT TARGET runtimeCore::runtimeCore)
  find_package(runtimeCore CONFIG REQUIRED)
endif()
if(NOT TARGET physicsCore::physicsCore)
  find_package(physicsCore CONFIG REQUIRED)
endif()

add_library(characterCore_warnings INTERFACE)
if(MSVC)
  target_compile_options(characterCore_warnings INTERFACE /W4 /permissive- /EHsc)
else()
  target_compile_options(characterCore_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_library(characterCore src/character_controller.cpp)
add_library(characterCore::characterCore ALIAS characterCore)
target_compile_features(characterCore PUBLIC cxx_std_17)
target_include_directories(characterCore PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(characterCore PUBLIC
  runtimeCore::runtimeCore
  physicsCore::physicsCore
)
target_link_libraries(characterCore PRIVATE $<BUILD_INTERFACE:characterCore_warnings>)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/characterCoreConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/characterCoreConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/characterCoreConfig.cmake"
  INSTALL_DESTINATION lib/cmake/characterCore
)

install(TARGETS characterCore EXPORT characterCoreTargets)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT characterCoreTargets FILE characterCoreTargets.cmake
  NAMESPACE characterCore:: DESTINATION lib/cmake/characterCore)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/characterCoreConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/characterCoreConfigVersion.cmake"
  DESTINATION lib/cmake/characterCore
)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
