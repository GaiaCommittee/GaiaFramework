#==============================
# Requirements
#==============================

cmake_minimum_required(VERSION 3.10)

#==============================
# Project Settings
#==============================

if (NOT PROJECT_DECLARED)
    project("Gaia Framework" LANGUAGES CXX)
    set(PROJECT_DECLARED)
endif()

#==============================
# Unit Settings
#==============================

set(TARGET_NAME "GaiaFramework")
set(TARGET_VERSION 0.1.0)
set(INSTALL_NAMESPACE "Gaia")

#==============================
# Command Lines
#==============================

set(CMAKE_CXX_STANDARD 17)

#==============================
# Source
#==============================

# Macro which is used to find .cpp files recursively.
macro(find_cpp path list_name)
    file(GLOB_RECURSE _tmp_list RELATIVE ${path} ${path}/*.cpp)
    set(${list_name})
    foreach(f ${_tmp_list})
        if(NOT f MATCHES "cmake-*")
            list(APPEND ${list_name} ${f})
        endif()
    endforeach()
endmacro()

# Macro which is used to find .hpp files recursively.
macro(find_hpp path list_name)
    file(GLOB_RECURSE _tmp_list RELATIVE ${path} ${path}/*.hpp)
    set(${list_name})
    foreach(f ${_tmp_list})
        if(NOT f MATCHES "cmake-*")
            list(APPEND ${list_name} ${f})
        endif()
    endforeach()
endmacro()

# Macro for adding a custom module to a specific target.
macro(add_custom_module target_name visibility module_name)
    find_path(${module_name}_INCLUDE_DIRS "${module_name}")
    find_library(${module_name}_LIBS "${module_name}")
    target_include_directories(${target_name} ${visibility} ${${module_name}_INCLUDE_DIRS})
    target_link_libraries(${target_name} ${visibility} ${${module_name}_LIBS})
endmacro()

#------------------------------
# C++
#------------------------------

# C++ Source Files
find_cpp(${CMAKE_CURRENT_SOURCE_DIR} TARGET_SOURCE)
# C++ Header Files
find_hpp(${CMAKE_CURRENT_SOURCE_DIR} TARGET_HEADER)

#==============================
# Compile Targets
#==============================

add_library(${TARGET_NAME} SHARED ${TARGET_SOURCE} ${TARGET_HEADER} ${TARGET_CUDA_SOURCE} ${TARGET_CUDA_HEADER})

# Set version property.
set_target_properties(${TARGET_NAME} PROPERTIES VERSION ${TARGET_VERSION})

# Enable 'DEBUG' Macro in Debug Mode
if(CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_definitions(${TARGET_NAME} PRIVATE -DDEBUG)
endif()

#==============================
# Dependencies
#==============================

# Gaia Background
find_package(GaiaBackground)
target_include_directories(${TARGET_NAME} PUBLIC ${GaiaBackground_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} PUBLIC Gaia::GaiaBackground)

# Boost
find_package(Boost 1.65 REQUIRED COMPONENTS program_options)
target_include_directories(${TARGET_NAME} PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} PUBLIC ${Boost_LIBRARIES})

# TBB
find_path(TBB_INCLUDE "tbb/tbb.h")
find_library(TBB_LIB "libtbb.so")
target_include_directories(${TARGET_NAME} PRIVATE ${TBB_INCLUDE})
target_link_libraries(${TARGET_NAME} PRIVATE ${TBB_LIB})

# hiredis
find_path(HIREDIS_INCLUDE_DIRS hiredis)
find_library(HIREDIS_LIBRARIES "hiredis")
target_include_directories(${TARGET_NAME} PUBLIC ${HIREDIS_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} PUBLIC ${HIREDIS_LIBRARIES})

# redis-plus-plus
find_path(REDIS_INCLUDE_DIRS "sw")
find_library(REDIS_LIBRARIES "redis++")
target_include_directories(${TARGET_NAME} PUBLIC ${REDIS_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} PUBLIC ${REDIS_LIBRARIES})

# In Linux, 'Threads' need to explicitly linked.
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(Threads)
    target_link_libraries(${TARGET_NAME} PUBLIC ${CMAKE_THREAD_LIBS_INIT})
    target_link_libraries(${TARGET_NAME} PUBLIC dl)
endif()

#===============================
# Install Scripts
#===============================

configure_file(
        "${PROJECT_SOURCE_DIR}/${TARGET_NAME}Config.hpp.in"
        "${PROJECT_BINARY_DIR}/${TARGET_NAME}Config.hpp")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Install executable files and libraries to 'default_path/'.
install(TARGETS ${TARGET_NAME}
        EXPORT ${TARGET_NAME}-targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
# Install header files to 'default_path/TARGET_NAME/'
install(DIRECTORY "." DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${TARGET_NAME}/ FILES_MATCHING PATTERN "*.hpp")
install(EXPORT ${TARGET_NAME}-targets
        NAMESPACE ${INSTALL_NAMESPACE}::
        FILE ${TARGET_NAME}Config.cmake
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/${TARGET_NAME})