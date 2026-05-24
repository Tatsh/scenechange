cmake_minimum_required(VERSION 3.28)
project(scenechange VERSION 0.2.0 LANGUAGES C)

include(FeatureSummary)
include(GNUInstallDirs)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(VAPOURSYNTH REQUIRED IMPORTED_TARGET vapoursynth)

pkg_get_variable(VAPOURSYNTH_PLUGINS_DIR_DEFAULT vapoursynth pluginsdir)
if(NOT VAPOURSYNTH_PLUGINS_DIR_DEFAULT)
  set(VAPOURSYNTH_PLUGINS_DIR_DEFAULT "${CMAKE_INSTALL_FULL_LIBDIR}/vapoursynth")
endif()
set(VAPOURSYNTH_PLUGINS_DIR "${VAPOURSYNTH_PLUGINS_DIR_DEFAULT}"
    CACHE PATH "Install directory for VapourSynth plugins.")

option(BUILD_DOCS "Build documentation." OFF)
option(INSTALL_DOCS "Install documentation." OFF)

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

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

include(CPack)
