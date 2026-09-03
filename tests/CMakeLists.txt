add_executable(core_regression_test core_regression_test.cpp)
target_link_libraries(core_regression_test PRIVATE deepiri_core deepiri_models)
add_test(NAME core_regression_test COMMAND core_regression_test)

add_executable(ac_analysis_test ac_analysis_test.cpp)
target_link_libraries(ac_analysis_test PRIVATE deepiri_core deepiri_models)
add_test(NAME ac_analysis_test COMMAND ac_analysis_test)

add_executable(analog_ota_test analog_ota_test.cpp)
target_link_libraries(analog_ota_test PRIVATE deepiri_analog)
add_test(NAME analog_ota_test COMMAND analog_ota_test)

add_executable(analog_gilbert_cell_test analog_gilbert_cell_test.cpp)
target_link_libraries(analog_gilbert_cell_test PRIVATE deepiri_analog)
add_test(NAME analog_gilbert_cell_test COMMAND analog_gilbert_cell_test)

add_executable(analog_opamp_neuron_test analog_opamp_neuron_test.cpp)
target_link_libraries(analog_opamp_neuron_test PRIVATE deepiri_analog)
add_test(NAME analog_opamp_neuron_test COMMAND analog_opamp_neuron_test)

add_executable(analog_noise_test analog_noise_test.cpp)
target_link_libraries(analog_noise_test PRIVATE deepiri_analog)
add_test(NAME analog_noise_test COMMAND analog_noise_test)

add_executable(analog_hopfield_test analog_hopfield_test.cpp)
target_link_libraries(analog_hopfield_test PRIVATE deepiri_analog)
add_test(NAME analog_hopfield_test COMMAND analog_hopfield_test)

add_executable(analog_ising_test analog_ising_test.cpp)
target_link_libraries(analog_ising_test PRIVATE deepiri_analog)
add_test(NAME analog_ising_test COMMAND analog_ising_test)

add_executable(nsp_test nsp_test.cpp)
target_link_libraries(nsp_test PRIVATE deepiri_ai)
add_test(NAME nsp_test COMMAND nsp_test)

add_executable(reservoir_test reservoir_test.cpp)
target_link_libraries(reservoir_test PRIVATE deepiri_ai)
add_test(NAME reservoir_test COMMAND reservoir_test)

add_executable(eii_pipeline_test eii_pipeline_test.cpp)
target_link_libraries(eii_pipeline_test PRIVATE deepiri_eii deepiri_core)
add_test(NAME eii_pipeline_test COMMAND eii_pipeline_test)

add_executable(rtl_shadow_sim_test rtl_shadow_sim_test.cpp)
target_link_libraries(rtl_shadow_sim_test PRIVATE deepiri_logic)
add_test(NAME rtl_shadow_sim_test COMMAND rtl_shadow_sim_test)

if(TARGET deepiri_gui)
    add_executable(test_gui_dc test_gui_dc.cpp)
    target_link_libraries(test_gui_dc PRIVATE deepiri_gui)
    add_test(NAME gui_dc_milestone COMMAND test_gui_dc)
    set_tests_properties(gui_dc_milestone PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()
