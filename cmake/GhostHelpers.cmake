# Funciones de build de GhostProducer (dueño: arquitecto). Ver ADR 0001 § Build.

# Opciones de compilación para NUESTRO código (no se aplican a JUCE ni a terceros).
function(gp_apply_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
        if(GP_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow
                                                 -Wnon-virtual-dtor -Wold-style-cast)
        if(GP_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

# gp_add_module(<nombre> <directorio> [DEPS <targets...>])
# Crea una librería estática con todos los .cpp de <directorio> (GLOB CONFIGURE_DEPENDS: las
# tarjetas añaden archivos sin tocar CMake). Si aún no hay .cpp, crea una librería INTERFACE
# para que los dependientes configuren igualmente (arranque de F0).
function(gp_add_module name dir)
    cmake_parse_arguments(ARG "" "" "DEPS" ${ARGN})
    file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${dir}/*.cpp" "${dir}/*.h")
    set(cpp_sources ${sources})
    list(FILTER cpp_sources INCLUDE REGEX "\\.cpp$")
    if(cpp_sources)
        add_library(${name} STATIC ${sources})
        target_link_libraries(${name} PUBLIC gp_contracts ${ARG_DEPS})
        target_include_directories(${name} PRIVATE "${dir}")
        gp_apply_warnings(${name})
    else()
        add_library(${name} INTERFACE)
        target_link_libraries(${name} INTERFACE gp_contracts ${ARG_DEPS})
    endif()
endfunction()

# Sanitizers globales (antes de añadir dependencias, para instrumentar también JUCE).
function(gp_enable_sanitizers list)
    if(NOT list)
        return()
    endif()
    if(MSVC)
        if(list MATCHES "address")
            add_compile_options(/fsanitize=address)
        endif()
        return()
    endif()
    add_compile_options(-fsanitize=${list} -fno-omit-frame-pointer -fno-sanitize-recover=all)
    add_link_options(-fsanitize=${list})
endfunction()
