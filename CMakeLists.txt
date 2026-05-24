cmake_minimum_required(VERSION 3.28)
project(scenechange VERSION 0.2.0 LANGUAGES C)

include(FeatureSummary)
include(GNUInstallDirs)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig)
if(PkgConfig_FOUND)
  pkg_check_modules(VAPOURSYNTH IMPORTED_TARGET vapoursynth)
endif()

if(NOT VAPOURSYNTH_FOUND)
  find_path(VAPOURSYNTH_INCLUDE_DIR
            NAMES VapourSynth.h
            PATH_SUFFIXES vapoursynth)
  if(NOT VAPOURSYNTH_INCLUDE_DIR)
    message(FATAL_ERROR
      "VapourSynth headers not found. Install via your package manager "
      "(apt install vapoursynth, brew install vapoursynth) or set "
      "VAPOURSYNTH_INCLUDE_DIR to a directory containing VapourSynth.h.")
  endif()
  add_library(PkgConfig::VAPOURSYNTH INTERFACE IMPORTED)
  target_include_directories(PkgConfig::VAPOURSYNTH INTERFACE ${VAPOURSYNTH_INCLUDE_DIR})
endif()

# Relative to CMAKE_INSTALL_PREFIX so CPack can stage it under the package root.
set(VAPOURSYNTH_PLUGINS_DIR "${CMAKE_INSTALL_LIBDIR}/vapoursynth"
    CACHE PATH "Install directory for VapourSynth plugins (relative to CMAKE_INSTALL_PREFIX).")

option(BUILD_DOCS "Build documentation." OFF)
option(INSTALL_DOCS "Install documentation." OFF)
option(BUILD_TESTS "Build the unit-test suite under tests/." OFF)
option(SCENECHANGE_COVERAGE "Enable code-coverage instrumentation (GCC/Clang only)." OFF)

if(SCENECHANGE_COVERAGE AND NOT MSVC)
  add_compile_options(--coverage -O0 -g)
  add_link_options(--coverage)
endif()

if(BUILD_DOCS)
  set(DOXYGEN_GENERATE_TAGFILE "${CMAKE_BINARY_DIR}/html/${CMAKE_PROJECT_NAME}.tags")
  set(DOXYGEN_INTERACTIVE_SVG YES)
  set(DOXYGEN_MARKDOWN_ID_STYLE GITHUB)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md")
  find_package(Doxygen REQUIRED COMPONENTS dot)
  doxygen_add_docs(docs README.md src ALL)
  if(INSTALL_DOCS)
    install(
      DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs/html
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION})
  endif()
endif()

add_subdirectory(src)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "Tatsh")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Scene change detection plugin for VapourSynth.")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/Tatsh/scenechange")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CMAKE_PROJECT_NAME}")

set(CPACK_GENERATOR "ZIP")

if(NOT CPACK_SYSTEM_NAME)
  if(WIN32)
    set(_cpack_os "windows")
  elseif(APPLE)
    set(_cpack_os "macos")
  else()
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" _cpack_os)
  endif()
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|aarch64|arm64)$")
    set(_cpack_arch "arm64")
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
    set(_cpack_arch "x86_64")
  else()
    set(_cpack_arch "${CMAKE_SYSTEM_PROCESSOR}")
  endif()
  set(CPACK_SYSTEM_NAME "${_cpack_os}-${_cpack_arch}")
endif()
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${CMAKE_PROJECT_VERSION}-${CPACK_SYSTEM_NAME}")

include(CPack)
