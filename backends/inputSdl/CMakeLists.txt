cmake_minimum_required(VERSION 3.25)

project(inputSdl VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

if(NOT TARGET inputCore::inputCore)
  find_package(inputCore CONFIG REQUIRED)
endif()

option(USD_STAGE_RUNNER_REQUIRE_SDL "Fail configuration when SDL2 or SDL3 is unavailable" OFF)

add_library(inputSdl_warnings INTERFACE)
if(MSVC)
  target_compile_options(inputSdl_warnings INTERFACE /W4 /permissive- /EHsc)
else()
  target_compile_options(inputSdl_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

set(INPUT_SDL_BACKEND "none")
set(INPUT_SDL_BACKEND_TARGET "")
find_package(SDL3 CONFIG QUIET)
if(TARGET SDL3::SDL3)
  set(INPUT_SDL_BACKEND "SDL3")
  set(INPUT_SDL_BACKEND_TARGET SDL3::SDL3)
else()
  find_package(SDL2 CONFIG QUIET)
  if(TARGET SDL2::SDL2)
    set(INPUT_SDL_BACKEND "SDL2")
    set(INPUT_SDL_BACKEND_TARGET SDL2::SDL2)
  endif()
endif()

if(USD_STAGE_RUNNER_REQUIRE_SDL AND NOT INPUT_SDL_BACKEND_TARGET)
  message(FATAL_ERROR "SDL input was requested but no SDL2 or SDL3 CMake target was found")
endif()

add_library(inputSdl src/physical_input.cpp src/sdl_input_source.cpp)
add_library(inputSdl::inputSdl ALIAS inputSdl)
target_compile_features(inputSdl PUBLIC cxx_std_17)
target_include_directories(inputSdl PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(inputSdl PUBLIC inputCore::inputCore)
target_link_libraries(inputSdl PRIVATE $<BUILD_INTERFACE:inputSdl_warnings>)
if(INPUT_SDL_BACKEND_TARGET)
  target_link_libraries(inputSdl PRIVATE ${INPUT_SDL_BACKEND_TARGET})
endif()
target_compile_definitions(inputSdl PRIVATE
  USD_STAGE_RUNNER_HAS_SDL3=$<STREQUAL:${INPUT_SDL_BACKEND},SDL3>
  USD_STAGE_RUNNER_HAS_SDL2=$<STREQUAL:${INPUT_SDL_BACKEND},SDL2>
)
message(STATUS "inputSdl: backend ${INPUT_SDL_BACKEND}")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/inputSdlConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/inputSdlConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/inputSdlConfig.cmake"
  INSTALL_DESTINATION lib/cmake/inputSdl
)
install(TARGETS inputSdl EXPORT inputSdlTargets)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT inputSdlTargets FILE inputSdlTargets.cmake NAMESPACE inputSdl::
  DESTINATION lib/cmake/inputSdl)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/inputSdlConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/inputSdlConfigVersion.cmake"
  DESTINATION lib/cmake/inputSdl
)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
