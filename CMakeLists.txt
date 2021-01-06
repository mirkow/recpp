cmake_minimum_required(VERSION 3.10)

file(READ .version recpp_VERSION)
string(STRIP "${recpp_VERSION}" recpp_VERSION)

project(
  recpp
  LANGUAGES CXX
  VERSION ${recpp_VERSION})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release") # default

  set(CMAKE_BUILD_TYPE
      "${CMAKE_BUILD_TYPE}"
      CACHE
        STRING
        "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
                                               "MinSizeRel" "RelWithDebInfo")
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(HEADERS src/recpp/utils.h src/recpp/FunctionMover.h)

set(SOURCES src/recpp/utils.cpp src/recpp/FunctionMover.cpp)

find_package(Clang REQUIRED)

add_library(recpp SHARED ${SOURCES} ${HEADERS})
target_link_libraries(recpp PUBLIC clangTooling stdc++)
target_link_options(recpp PUBLIC -Wall -frtti -fPIC)
target_include_directories(
  recpp PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/>
               $<INSTALL_INTERFACE:include/>)
target_include_directories(recpp PUBLIC ${CLANG_INCLUDE_DIRS})

set_target_properties(recpp PROPERTIES PUBLIC_HEADER "${HEADERS}")

set_target_properties(recpp PROPERTIES VERSION ${PROJECT_VERSION}
                                       SOVERSION ${PROJECT_VERSION_MAJOR})

# ===============================
# Installation
# ===============================

# Offer the user the choice of overriding the installation directories
set(INSTALL_LIB_DIR
    lib
    CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR
    bin
    CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR
    include
    CACHE PATH "Installation directory for header files")
if(WIN32 AND NOT CYGWIN)
  set(DEF_INSTALL_CMAKE_DIR CMake)
else()
  set(DEF_INSTALL_CMAKE_DIR share/recpp/cmake)
endif()
set(INSTALL_CMAKE_DIR
    ${DEF_INSTALL_CMAKE_DIR}
    CACHE PATH "Installation directory for CMake files")

install(
  TARGETS recpp
  EXPORT recppTargets
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
  PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}/recpp" COMPONENT dev)

# Add all targets to the build-tree export set
export(TARGETS recpp FILE "${PROJECT_BINARY_DIR}/recppTargets.cmake")

# Export the package for use from the build-tree (this registers the build-tree
# with a global CMake-registry)
export(PACKAGE recpp)

# Make relative paths absolute (needed later on)
foreach(p LIB BIN INCLUDE CMAKE)
  set(var INSTALL_${p}_DIR)
  if(NOT IS_ABSOLUTE "${${var}}")
    set(ABS_INSTALL_${p}_DIR "${CMAKE_INSTALL_PREFIX}/${${var}}")
  else()
    set(ABS_INSTALL_${p}_DIR "${${var}}")
  endif()
endforeach()
foreach(p LIB BIN INCLUDE)
  file(RELATIVE_PATH REL_${p}_DIR "${ABS_INSTALL_CMAKE_DIR}"
       "${ABS_INSTALL_${p}_DIR}")
endforeach()

# Create the recppConfig.cmake and recppConfigVersion files

# ... for the build tree
set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")
# TODO: Replace with target_link_directories, when using cmake 3.13
set(CONF_LINK_DIR_CMD "link_directories(${CONF_LIB_DIRS})")
configure_file(cmake/recppConfig.cmake.in
               "${PROJECT_BINARY_DIR}/recppConfig.cmake" @ONLY)
# ... for the install tree
set(CONF_INCLUDE_DIRS "\${recpp_CMAKE_DIR}/${REL_INCLUDE_DIR}")
set(CONF_LIB_DIRS "\${recpp_CMAKE_DIR}/${REL_LIB_DIR}")
set(CONF_LINK_DIR_CMD "link_directories(${CONF_LIB_DIRS})")

configure_file(
  cmake/recppConfig.cmake.in
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/recppConfig.cmake" @ONLY)
# ... for both
configure_file(cmake/recppConfigVersion.cmake.in
               "${PROJECT_BINARY_DIR}/recppConfigVersion.cmake" @ONLY)

# Install the recppConfig.cmake and recppConfigVersion.cmake
install(
  FILES "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/recppConfig.cmake"
        "${PROJECT_BINARY_DIR}/recppConfigVersion.cmake"
  DESTINATION "${INSTALL_CMAKE_DIR}"
  COMPONENT dev)

# Install the export set for use with the install-tree
install(
  EXPORT recppTargets
  DESTINATION "${INSTALL_CMAKE_DIR}"
  COMPONENT dev)

# ===============================
# cpack
# ===============================

set(CPACK_PACKAGE_NAME "librecpp-dev")
set(CPACK_GENERATOR "DEB;TGZ")
set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_SOURCE_DIR}/cmake/setupCPack.cmake)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Mirko Wächter")
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  set(CPACK_DEBIAN_ARCHITECTURE "amd64")
else()
  set(CPACK_DEBIAN_ARCHITECTURE "i386")
endif()

include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_DESCRIPTION
    "Library for convenient logging of messages to the console or files.")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Refactoring tools for C++")
set(CPACK_PACKAGE_CONTACT "Mirko Wächter")

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
include(CPack)

# ===============================
# clang format/tidy
# ===============================

option(BUILD_TESTS "Enables building unit tests." OFF)
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

# ===============================
# Examples
# ===============================
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic") # needed for
                                                                  # backtrace in
                                                                  # executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
add_custom_target(static_exes)

macro(add_recpp_exe app_name app_src)
  add_executable(${app_name} ${app_src})
  set_target_properties(${app_name} PROPERTIES LINK_FLAGS_RELEASE -s)
  target_link_libraries(${app_name} recpp stdc++)

  add_custom_target(
    ${app_name}_static_exe
    mkblob ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${app_name} -o ${app_name}_static
    -static
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    DEPENDS ${app_name})
  add_dependencies(static_exes ${app_name}_static_exe)
endmacro()

add_recpp_exe(FunctionMover src/FunctionMover/main.cpp)
