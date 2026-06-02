cmake_minimum_required(VERSION 3.28)
project(scenechange VERSION 0.3.0 LANGUAGES C)

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

option(SCENECHANGE_FETCH_VAPOURSYNTH_HEADERS
       "Download VapourSynth4.h at configure time when no headers are found (used for wheel builds)."
       ON)
set(SCENECHANGE_VAPOURSYNTH_HEADERS_REF "R72"
    CACHE STRING "Git ref of the VapourSynth repository to fetch VapourSynth4.h from.")

if(NOT VAPOURSYNTH_FOUND)
  find_path(VAPOURSYNTH_INCLUDE_DIR
            NAMES VapourSynth4.h
            PATH_SUFFIXES vapoursynth)
  if(NOT VAPOURSYNTH_INCLUDE_DIR AND SCENECHANGE_FETCH_VAPOURSYNTH_HEADERS)
    # VapourSynth4.h only depends on stdint.h and stddef.h, so fetching this
    # single header is enough to compile the plugins. The VapourSynth runtime is
    # provided by the host process at load time, so no library is linked. This
    # path is used by the Python wheel builds, where no system VapourSynth is
    # installed.
    set(_vs_headers_dir "${CMAKE_BINARY_DIR}/vapoursynth-headers")
    message(STATUS
            "VapourSynth headers not found; downloading VapourSynth4.h "
            "(${SCENECHANGE_VAPOURSYNTH_HEADERS_REF}).")
    file(DOWNLOAD
         "https://raw.githubusercontent.com/vapoursynth/vapoursynth/${SCENECHANGE_VAPOURSYNTH_HEADERS_REF}/include/VapourSynth4.h"
         "${_vs_headers_dir}/VapourSynth4.h"
         STATUS _vs_download_status
         TLS_VERIFY ON)
    list(GET _vs_download_status 0 _vs_download_code)
    if(NOT _vs_download_code EQUAL 0)
      list(GET _vs_download_status 1 _vs_download_message)
      message(FATAL_ERROR "Failed to download VapourSynth4.h: ${_vs_download_message}.")
    endif()
    set(VAPOURSYNTH_INCLUDE_DIR "${_vs_headers_dir}" CACHE PATH "VapourSynth include directory." FORCE)
  endif()
  if(NOT VAPOURSYNTH_INCLUDE_DIR)
    message(FATAL_ERROR
      "VapourSynth headers not found. Install via your package manager "
      "(apt install vapoursynth, brew install vapoursynth), set "
      "VAPOURSYNTH_INCLUDE_DIR to a directory containing VapourSynth4.h, or enable "
      "the SCENECHANGE_FETCH_VAPOURSYNTH_HEADERS option.")
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
option(BUILD_PYTHON_PACKAGE
       "Install the plugins into the scenechange Python package (for wheel builds)." OFF)

# scikit-build-core defines SKBUILD when building a wheel; in that case the
# plugins are bundled into the Python package rather than installed to the
# VapourSynth plugins directory.
if(SKBUILD)
  set(BUILD_PYTHON_PACKAGE ON)
endif()

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
  doxygen_add_docs(docs README.md native ALL)
  if(INSTALL_DOCS)
    install(
      DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs/html
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION})
  endif()
endif()

add_subdirectory(native)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

if(NOT BUILD_PYTHON_PACKAGE)
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
endif()
