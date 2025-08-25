# examples/<something>/CMakeLists.txt
cmake_minimum_required(VERSION 3.14)

# Имя проекта можно оставить произвольным — на логику не влияет
project(mylib-example LANGUAGES CXX)

include("../../cmake/utils.cmake")

# Определяем, собираемся ли мы как верхнеуровневый проект
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" is_top_level)

# Имя исполняемой цели:
# 1) переопределяется -DMYLIB_EXE_NAME=...,
# 2) иначе — берётся из имени директории с примером.
if(NOT DEFINED MYLIB_EXE_NAME OR MYLIB_EXE_NAME STREQUAL "")
    get_filename_component(MYLIB_EXE_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
endif()

if(is_top_level)
    # Когда пример собирается отдельно
    find_package(mylib REQUIRED CONFIG)
endif()

set(sources
        main.cpp
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})

add_executable(${MYLIB_EXE_NAME})
target_sources(${MYLIB_EXE_NAME} PRIVATE ${sources})
target_link_libraries(${MYLIB_EXE_NAME} PRIVATE mylib::mylib)
target_compile_features(${MYLIB_EXE_NAME} PRIVATE cxx_std_17)

# Копирование зависимостей имеет смысл только при встраивании примера в чужой проект
# и если доступна утилита из utils.cmake.
if(NOT is_top_level)
    win_copy_deps_to_target_dir(${MYLIB_EXE_NAME} mylib::mylib)
endif()
