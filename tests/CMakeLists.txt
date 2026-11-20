# Tests (dueño: qa; creado por el arquitecto en F0-G1). Un único ejecutable Catch2 por grupo.
# Cada tarjeta añade tests en su subcarpeta; se recogen por GLOB sin tocar este archivo.
#   tests/model/ commands/ engine/rt/ engine/core/ integration/  → headless (CI Linux)
#   tests/engine/device/ ui/                                       → solo con GUI (JUCE completo)
include(Catch)

function(gp_add_test_suite name)
    cmake_parse_arguments(ARG "" "" "DIRS;DEPS" ${ARGN})
    set(sources "")
    foreach(dir IN LISTS ARG_DIRS)
        file(GLOB_RECURSE found CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/*.cpp")
        list(APPEND sources ${found})
    endforeach()
    if(NOT sources)
        return()
    endif()
    add_executable(${name} ${sources})
    target_link_libraries(${name} PRIVATE Catch2::Catch2WithMain ${ARG_DEPS})
    target_include_directories(${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    gp_apply_warnings(${name})
    catch_discover_tests(${name} DISCOVERY_MODE PRE_TEST
        PROPERTIES LABELS "${name}")
endfunction()

gp_add_test_suite(gp_tests_core
    DIRS model commands engine/rt engine/core integration
    DEPS gp_model gp_commands gp_rt gp_engine_core)

if(NOT GP_HEADLESS)
    gp_add_test_suite(gp_tests_gui
        DIRS engine/device ui
        DEPS gp_ui gp_engine_device gp_engine_core gp_commands gp_model gp_rt gp_juce)
endif()
