# GhostProducer — CMake raíz (dueño: arquitecto). Ver docs/adr/0001-stack.md § Build.
# Las tarjetas NO editan este archivo: añaden .cpp en su carpeta y gp_add_module los recoge.
cmake_minimum_required(VERSION 3.25)

# Debe fijarse antes de project().
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Versión mínima de macOS")

project(GhostProducer VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GP_HEADLESS "Solo librerías sin GUI ni dispositivos, y sus tests (CI Linux)" OFF)
option(GP_BUILD_TESTS "Compilar los tests (Catch2)" ON)
option(GP_RT_CHECKS "Detector de violaciones RT (hook de operator new). Nunca en release" OFF)
option(GP_WARNINGS_AS_ERRORS "Warnings como errores en nuestro código" OFF)
set(GP_SANITIZERS "" CACHE STRING "Lista para -fsanitize (p. ej. address,undefined o thread)")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GhostHelpers)
gp_enable_sanitizers("${GP_SANITIZERS}")

# --- Dependencias (versiones fijadas; hashes en THIRD_PARTY.md) -------------------------
# Usa CPM_SOURCE_CACHE (variable de entorno o -D) para no volver a clonar.
include(CPM)

# En headless no se construye juceaide (en Linux exige cabeceras X11/XInput): solo módulos.
if(GP_HEADLESS)
    set(GP_JUCE_OPTIONS "JUCE_MODULES_ONLY ON")
else()
    set(GP_JUCE_OPTIONS "JUCE_MODULES_ONLY OFF")
endif()
CPMAddPackage(
    NAME JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.6  # 654bdbc7a9db0dae90849b44abe91f339e9f0768
    GIT_SHALLOW TRUE
    OPTIONS "${GP_JUCE_OPTIONS}")

CPMAddPackage(
    NAME nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0  # 55f93686c01528224f448c19128836e7df245f72
    GIT_SHALLOW TRUE
    OPTIONS "JSON_BuildTests OFF" "JSON_Install OFF")

if(GP_BUILD_TESTS)
    CPMAddPackage(
        NAME Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.16.1  # 08092139210881f01a34e60c5f2fd6afa0ea4024
        GIT_SHALLOW TRUE)
    list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")  # include(Catch)
endif()

# --- JUCE 9 como UNA librería estática compartida -------------------------------------------
# Los módulos JUCE son librerías INTERFACE con fuentes: si varios targets los enlazan, se
# compilan varias veces (símbolos duplicados). Todos nuestros targets enlazan gp_juce.
if(GP_HEADLESS)
    set(GP_JUCE_MODULES juce_core juce_audio_basics)
else()
    set(GP_JUCE_MODULES
        juce_core juce_events juce_data_structures juce_audio_basics juce_audio_devices
        juce_audio_formats juce_audio_processors juce_audio_utils juce_dsp juce_graphics
        juce_gui_basics juce_gui_extra)
endif()
list(TRANSFORM GP_JUCE_MODULES PREPEND "juce::" OUTPUT_VARIABLE GP_JUCE_TARGETS)

add_library(gp_juce STATIC)
target_link_libraries(gp_juce PRIVATE ${GP_JUCE_TARGETS})
if(TARGET juce::juce_recommended_config_flags)  # no existe con JUCE_MODULES_ONLY
    target_link_libraries(gp_juce PUBLIC juce::juce_recommended_config_flags)
endif()
target_compile_definitions(gp_juce
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_MODAL_LOOPS_PERMITTED=0
    INTERFACE $<TARGET_PROPERTY:gp_juce,COMPILE_DEFINITIONS>)
target_include_directories(gp_juce SYSTEM INTERFACE
    $<TARGET_PROPERTY:gp_juce,INCLUDE_DIRECTORIES>)
set_target_properties(gp_juce PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- Contratos (include/) -------------------------------------------------------------------
add_library(gp_contracts INTERFACE)
target_include_directories(gp_contracts INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(gp_contracts INTERFACE nlohmann_json::nlohmann_json)
target_compile_features(gp_contracts INTERFACE cxx_std_20)
target_compile_definitions(gp_contracts INTERFACE
    GP_RT_CHECKS=$<BOOL:${GP_RT_CHECKS}>
    GP_APP_VERSION="${PROJECT_VERSION}")

# --- Módulos (ver mapa de propiedad en CLAUDE.md y docs/ARQUITECTURA.md) -----------------
# Modelo y comandos: C++ puro, sin JUCE.
# gp_rt va primero: modelo y comandos usan GP_RT_ASSERT_NOT_REALTIME (símbolos de gp_rt).
gp_add_module(gp_rt       src/engine/rt)
gp_add_module(gp_model    src/model     DEPS gp_rt)
gp_add_module(gp_commands src/commands  DEPS gp_model gp_rt)
# Motor: núcleo de render (sin dispositivos) y capa de dispositivos.
gp_add_module(gp_engine_core src/engine/core DEPS gp_rt gp_model gp_juce)

if(NOT GP_HEADLESS)
    gp_add_module(gp_engine_device src/engine/device DEPS gp_engine_core gp_juce)

    # Catálogos i18n embebidos (assets/i18n/*.json → BinaryData).
    file(GLOB GP_I18N_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/i18n/*.json")
    set(GP_ASSET_TARGETS "")
    if(GP_I18N_FILES)
        juce_add_binary_data(gp_assets NAMESPACE GhostAssets SOURCES ${GP_I18N_FILES})
        set(GP_ASSET_TARGETS gp_assets)
    endif()

    gp_add_module(gp_ui src/ui DEPS gp_commands gp_juce ${GP_ASSET_TARGETS})
    # Permite `#include "ui/MainComponent.h"` desde src/app y tests (propuesta de F0-T6).
    get_target_property(GP_UI_TYPE gp_ui TYPE)
    if(GP_UI_TYPE STREQUAL "INTERFACE_LIBRARY")
        target_include_directories(gp_ui INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    else()
        target_include_directories(gp_ui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
    endif()

    # Aplicación (src/app). Se crea cuando existan sus fuentes.
    file(GLOB_RECURSE GP_APP_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/app/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/app/*.h")
    if(GP_APP_SOURCES)
        juce_add_gui_app(GhostProducerApp
            PRODUCT_NAME "GhostProducer"
            COMPANY_NAME "GhostProducer"
            BUNDLE_ID "org.ghostproducer.app"
            VERSION "${PROJECT_VERSION}"
            MICROPHONE_PERMISSION_ENABLED TRUE
            MICROPHONE_PERMISSION_TEXT "GhostProducer usa la entrada de audio para grabar y monitorizar.")
        target_sources(GhostProducerApp PRIVATE ${GP_APP_SOURCES})
        target_link_libraries(GhostProducerApp PRIVATE
            gp_ui gp_engine_device gp_engine_core gp_commands gp_model gp_rt gp_juce)
        gp_apply_warnings(GhostProducerApp)
    endif()
endif()

# --- Tests ------------------------------------------------------------------------------------
if(GP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
