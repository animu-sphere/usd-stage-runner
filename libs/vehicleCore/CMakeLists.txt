cmake_minimum_required(VERSION 3.25)

project(vehicleCore VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

if(NOT TARGET runtimeCore::runtimeCore)
  find_package(runtimeCore CONFIG REQUIRED)
endif()
if(NOT TARGET physicsCore::physicsCore)
  find_package(physicsCore CONFIG REQUIRED)
endif()

add_library(vehicleCore_warnings INTERFACE)
if(MSVC)
  target_compile_options(vehicleCore_warnings INTERFACE /W4 /permissive- /EHsc)
else()
  target_compile_options(vehicleCore_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_library(vehicleCore src/vehicle_controller.cpp)
add_library(vehicleCore::vehicleCore ALIAS vehicleCore)
target_compile_features(vehicleCore PUBLIC cxx_std_17)
target_include_directories(vehicleCore PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(vehicleCore PUBLIC
  runtimeCore::runtimeCore
  physicsCore::physicsCore
)
target_link_libraries(vehicleCore PRIVATE $<BUILD_INTERFACE:vehicleCore_warnings>)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/vehicleCoreConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/vehicleCoreConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vehicleCoreConfig.cmake"
  INSTALL_DESTINATION lib/cmake/vehicleCore
)

install(TARGETS vehicleCore EXPORT vehicleCoreTargets)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT vehicleCoreTargets FILE vehicleCoreTargets.cmake
  NAMESPACE vehicleCore:: DESTINATION lib/cmake/vehicleCore)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/vehicleCoreConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/vehicleCoreConfigVersion.cmake"
  DESTINATION lib/cmake/vehicleCore
)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
