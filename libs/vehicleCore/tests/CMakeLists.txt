add_executable(vehicle_core_tests vehicle_tests.cpp)
target_compile_features(vehicle_core_tests PRIVATE cxx_std_17)
target_link_libraries(vehicle_core_tests PRIVATE
  vehicleCore::vehicleCore
  vehicleCore_warnings
)
add_test(NAME vehicleCore.contracts COMMAND vehicle_core_tests)
