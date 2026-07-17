cmake_minimum_required(VERSION 3.16)
project(deepiri-egottol VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(ENABLE_QT "Build Qt GUI" ON)
option(ENABLE_AVIONICS "Build avionics" ON)
option(BUILD_PYTHON_BINDINGS "Build pybind11 module egottol._native" OFF)

find_package(Qt6 COMPONENTS Core Gui Widgets Network QUIET)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CORE_SOURCES
    core/matrix.cpp
    core/integrator.cpp
    core/newton_raphson.cpp
    core/dc_op.cpp
    core/transient.cpp
    core/mna_solver.cpp
    core/solver.cpp
    core/circuit.cpp
    core/device_mgr.cpp
)

set(MODELS_SOURCES
    models/device.cpp
    models/resistor.cpp
    models/capacitor.cpp
    models/inductor.cpp
    models/diode.cpp
    models/bjt.cpp
    models/mosfet.cpp
    models/vsrc.cpp
    models/isrc.cpp
    models/vccs.cpp
    models/vcvs.cpp
    models/opamp.cpp
    models/transmission_line.cpp
    models/josephson_junction.cpp
    models/subcircuit.cpp
)

set(LOGIC_SOURCES
    logic/event_queue.cpp
    logic/logic_gate.cpp
    logic/flip_flop.cpp
    logic/signal_buffer.cpp
    logic/vhdl_lexer.cpp
    logic/vhdl_parser.cpp
    logic/subckt.cpp
)

set(IO_SOURCES
    io/netlist_parser.cpp
    io/project_loader.cpp
    io/symbol_library.cpp
    io/waveform_writer.cpp
    io/simulation_data.cpp
)

if(Qt6_FOUND AND ENABLE_QT)
    list(APPEND GUI_SOURCES
        gui/scene.cpp
        gui/component_item.cpp
        gui/wire_item.cpp
        gui/schematic_view.cpp
        gui/wire_tool.cpp
        gui/selection_tool.cpp
        gui/main_window.cpp
        gui/waveform_plotter.cpp
        gui/property_editor.cpp
    )
endif()

if(ENABLE_AVIONICS)
    list(APPEND AVIONICS_SOURCES
        avionics/adsb_decoder.cpp
        avionics/gdl90_encoder.cpp
        avionics/sdr_buffer.cpp
    )
endif()

set(INFRA_SOURCES
    infra/zepgpu_client.cpp
    infra/uqe_bridge.cpp
    infra/config_manager.cpp
)

add_library(deepiri_core STATIC ${CORE_SOURCES})
target_include_directories(deepiri_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(BUILD_PYTHON_BINDINGS)
    # Required on Linux: static .a objects must be PIC to link into the _native .so
    set_target_properties(deepiri_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

add_library(deepiri_models STATIC ${MODELS_SOURCES})
target_link_libraries(deepiri_models PUBLIC deepiri_core)

add_library(deepiri_logic STATIC ${LOGIC_SOURCES})

add_library(deepiri_io STATIC ${IO_SOURCES})

if(ENABLE_AVIONICS)
    add_library(deepiri_avionics STATIC ${AVIONICS_SOURCES})
    target_link_libraries(deepiri_avionics PUBLIC deepiri_core)
endif()

add_library(deepiri_infra STATIC ${INFRA_SOURCES})

if(Qt6_FOUND AND ENABLE_QT)
    add_library(deepiri_gui STATIC ${GUI_SOURCES})
    target_link_libraries(deepiri_gui PUBLIC 
        deepiri_core deepiri_models deepiri_io Qt6::Core Qt6::Gui Qt6::Widgets
    )
    
    # No standalone C++ executable — app is launched via Python (egottol.ui.main)
endif()

if(BUILD_PYTHON_BINDINGS)
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

    include(FetchContent)
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.13.6
    )
    FetchContent_MakeAvailable(pybind11)

    pybind11_add_module(_native bindings/pybind_module.cpp)
    target_include_directories(_native PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/core)
    target_link_libraries(_native PRIVATE deepiri_core)
    target_compile_features(_native PRIVATE cxx_std_20)

    set_target_properties(_native PROPERTIES
        OUTPUT_NAME "_native"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/egottol"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/egottol"
    )
endif()

enable_testing()
add_subdirectory(tests)