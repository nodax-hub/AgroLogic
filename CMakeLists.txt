cmake_minimum_required(VERSION 3.14)
project(mylib
    VERSION 1.0.0
    DESCRIPTION "Template for C++ library built with CMake"
    LANGUAGES CXX)

#----------------------------------------------------------------------------------------------------------------------
# general settings and options
#----------------------------------------------------------------------------------------------------------------------

include(cmake/utils.cmake)
include(GNUInstallDirs)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" is_top_level)

# MYLIB_SHARED_LIBS option (undefined by default) can be used to force shared/static build
option(MYLIB_BUILD_TESTS "Build mylib tests" OFF)
option(MYLIB_BUILD_EXAMPLES "Build mylib examples" OFF)
option(MYLIB_BUILD_DOCS "Build mylib documentation" OFF)
option(MYLIB_INSTALL "Generate target for installing mylib" ${is_top_level})
set_if_undefined(MYLIB_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/mylib" CACHE STRING
    "Install path for mylib package-related CMake files")

if(DEFINED MYLIB_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ${MYLIB_SHARED_LIBS})
endif()

if(NOT DEFINED CMAKE_BUILD_TYPE AND NOT DEFINED CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set_if_undefined(CMAKE_CXX_VISIBILITY_PRESET hidden)
set_if_undefined(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

add_library(mylib) # initialized below
add_library(mylib::mylib ALIAS mylib)

#----------------------------------------------------------------------------------------------------------------------
# mylib dependencies
#----------------------------------------------------------------------------------------------------------------------

# Опция: собирать с PROJ (для AEQD/UTM). По умолчанию OFF.
option(MYLIB_WITH_PROJ "Build mylib with vendored static PROJ and use it privately" OFF)

include(FetchContent)

if(MYLIB_WITH_PROJ)
    include(FetchContent)

    # Новая политика по умолчанию для FetchContent (чтобы не было предупреждений)
    if(POLICY CMP0169)
        cmake_policy(SET CMP0169 NEW)
    endif()

    # Сборка статической libproj и PIC (если mylib — SHARED)
    set(_SAVED_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    # --- КЛЮЧЕВОЕ: задать опции подпроекта ДО его подключения ---
    # названия опций зависят от версии PROJ; ставим все релевантные в OFF
    set(ENABLE_SQLITE3      OFF CACHE BOOL "" FORCE)
    set(PROJ_ENABLE_SQLITE3 OFF CACHE BOOL "" FORCE)
    set(ENABLE_SQLITE       OFF CACHE BOOL "" FORCE)

    set(ENABLE_TIFF         OFF CACHE BOOL "" FORCE)
    set(ENABLE_CURL         OFF CACHE BOOL "" FORCE)

    # CLI-утилиты PROJ нам не нужны внутри библиотеки
    set(BUILD_PROJ          OFF CACHE BOOL "" FORCE)
    set(BUILD_PROJINFO      OFF CACHE BOOL "" FORCE)
    set(BUILD_CS2CS         OFF CACHE BOOL "" FORCE)
    set(BUILD_CCT           OFF CACHE BOOL "" FORCE)
    set(BUILD_GIE           OFF CACHE BOOL "" FORCE)
    set(BUILD_PROJSYNC      OFF CACHE BOOL "" FORCE)

    # установка нам тоже не обязательна
    set(PROJ_INSTALL        OFF CACHE BOOL "" FORCE)

    # Объявляем и подключаем ПРОЕКТ стандартным способом
    FetchContent_Declare(
            PROJ
            GIT_REPOSITORY https://github.com/OSGeo/PROJ.git
            GIT_TAG        9.4.0
            UPDATE_DISCONNECTED YES
    )
    FetchContent_MakeAvailable(PROJ)  # <-- без Populate

    # Восстановить исходное значение, если важно для остальной сборки
    set(BUILD_SHARED_LIBS ${_SAVED_BUILD_SHARED_LIBS} CACHE BOOL "" FORCE)

    # Линкуем приватно
    target_compile_definitions(mylib PRIVATE MYLIB_WITH_PROJ)
    target_link_libraries(mylib PRIVATE PROJ::proj)

    # Прячем символы статически прилинкованных архивов (не актуально для MSVC)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_link_options(mylib PRIVATE "LINKER:--exclude-libs,ALL")
    endif()
endif()

#----------------------------------------------------------------------------------------------------------------------
# mylib sources
#----------------------------------------------------------------------------------------------------------------------

include(GenerateExportHeader)
set(export_file_name "export_shared.h")

if(NOT BUILD_SHARED_LIBS)
    set(export_file_name "export_static.h")
endif()

generate_export_header(mylib EXPORT_FILE_NAME include/mylib/${export_file_name})

set(sources
    include/mylib/export.h
    include/mylib/mylib.h       src/mylib.cpp
    include/mylib/geometry.h    src/geometry.cpp
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})

#----------------------------------------------------------------------------------------------------------------------
# mylib target
#----------------------------------------------------------------------------------------------------------------------

include(CMakePackageConfigHelpers)

target_sources(mylib PRIVATE ${sources})
target_compile_definitions(mylib PUBLIC "$<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:MYLIB_STATIC_DEFINE>")

if(MYLIB_WITH_PROJ)
    target_compile_definitions(mylib PRIVATE MYLIB_WITH_PROJ)
    target_link_libraries(mylib PRIVATE ${_MYLIB_PROJ_TARGET})

    # Сокрытие символов статически прилинкованных архивов в ELF (GNU ld, LLD):
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_link_options(mylib PRIVATE "LINKER:--exclude-libs,ALL")
    endif()
endif()

target_include_directories(mylib
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>")

set_target_properties(mylib PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    SOVERSION ${PROJECT_VERSION_MAJOR}
    VERSION ${PROJECT_VERSION})

if(MYLIB_INSTALL AND NOT CMAKE_SKIP_INSTALL_RULES)
    configure_package_config_file(cmake/mylib-config.cmake.in mylib-config.cmake
        INSTALL_DESTINATION "${MYLIB_INSTALL_CMAKEDIR}")

    write_basic_package_version_file(mylib-config-version.cmake
        COMPATIBILITY SameMajorVersion)

    install(TARGETS mylib EXPORT mylib_export
        RUNTIME COMPONENT mylib
        LIBRARY COMPONENT mylib NAMELINK_COMPONENT mylib-dev
        ARCHIVE COMPONENT mylib-dev
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    install(DIRECTORY include/
        TYPE INCLUDE
        COMPONENT mylib-dev)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/mylib/${export_file_name}"
        COMPONENT mylib-dev
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mylib")

    set(targets_file "mylib-shared-targets.cmake")

    if(NOT BUILD_SHARED_LIBS)
        set(targets_file "mylib-static-targets.cmake")
    endif()

    install(EXPORT mylib_export
        COMPONENT mylib-dev
        FILE "${targets_file}"
        DESTINATION "${MYLIB_INSTALL_CMAKEDIR}"
        NAMESPACE mylib::)

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/mylib-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/mylib-config-version.cmake"
        COMPONENT mylib-dev
        DESTINATION "${MYLIB_INSTALL_CMAKEDIR}")

    if(MSVC)
        set(pdb_file "")
        set(pdb_file_destination "")

        if(BUILD_SHARED_LIBS)
            set(pdb_file "$<TARGET_PDB_FILE:mylib>")
            set(pdb_file_destination "${CMAKE_INSTALL_BINDIR}")
        else()
            # TARGET_PDB_FILE does not work for pdb file generated for static library build, determining it manually
            set(pdb_file "$<TARGET_FILE_DIR:mylib>/$<TARGET_FILE_PREFIX:mylib>$<TARGET_FILE_BASE_NAME:mylib>.pdb")
            set(pdb_file_destination "${CMAKE_INSTALL_LIBDIR}")
        endif()

        install(FILES "${pdb_file}"
            COMPONENT mylib-dev
            CONFIGURATIONS Debug RelWithDebInfo
            DESTINATION "${pdb_file_destination}"
            OPTIONAL)
    endif()
endif()

#----------------------------------------------------------------------------------------------------------------------
# other targets
#----------------------------------------------------------------------------------------------------------------------

if(MYLIB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(MYLIB_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(MYLIB_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    doxygen_add_docs(docs include)
endif()
