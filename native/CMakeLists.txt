set(SCENECHANGE_MSVC_WARNINGS /wd4224 /wd4244 /wd4996)
set(SCENECHANGE_GNU_WARNINGS -Wall -Wshadow)

add_library(scenechange MODULE scenechange.c)
target_link_libraries(scenechange PRIVATE PkgConfig::VAPOURSYNTH)
if(MSVC)
  target_compile_options(scenechange PRIVATE ${SCENECHANGE_MSVC_WARNINGS})
else()
  target_compile_options(scenechange PRIVATE ${SCENECHANGE_GNU_WARNINGS})
endif()

option(SCENECHANGE_FORCE_GENERIC
       "Force the generic scalar backend even on architectures that have a SIMD one."
       OFF)

set(TEMPORALSOFTEN_SRCS temporalsoften.c)
set(TEMPORALSOFTEN_SIMD_FLAGS "")
if(SCENECHANGE_FORCE_GENERIC)
  message(STATUS "SCENECHANGE_FORCE_GENERIC=ON; using the generic scalar backend.")
  list(APPEND TEMPORALSOFTEN_SRCS temporalsoften_generic.c)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|i[3-6]86|x86)$")
  list(APPEND TEMPORALSOFTEN_SRCS temporalsoften_x86.c)
  if(MSVC)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
      set(TEMPORALSOFTEN_SIMD_FLAGS /arch:SSE2)
    endif()
  else()
    set(TEMPORALSOFTEN_SIMD_FLAGS -msse2 -mfpmath=sse)
  endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
  list(APPEND TEMPORALSOFTEN_SRCS temporalsoften_aarch64.c)
else()
  message(STATUS
    "No SIMD backend for '${CMAKE_SYSTEM_PROCESSOR}'; using the generic scalar fallback.")
  list(APPEND TEMPORALSOFTEN_SRCS temporalsoften_generic.c)
endif()

add_library(temporalsoften2 MODULE ${TEMPORALSOFTEN_SRCS})
target_link_libraries(temporalsoften2 PRIVATE PkgConfig::VAPOURSYNTH)
if(NOT MSVC)
  find_library(MATH_LIBRARY m)
  if(MATH_LIBRARY)
    target_link_libraries(temporalsoften2 PRIVATE ${MATH_LIBRARY})
  endif()
endif()
if(MSVC)
  target_compile_options(temporalsoften2 PRIVATE ${SCENECHANGE_MSVC_WARNINGS})
else()
  target_compile_options(temporalsoften2 PRIVATE ${SCENECHANGE_GNU_WARNINGS})
endif()
target_compile_options(temporalsoften2 PRIVATE ${TEMPORALSOFTEN_SIMD_FLAGS})

# Per-target metadata: Windows resource files for DLL VERSIONINFO, macOS
# embedded Info.plist via -sectcreate. Both pull project version and
# common attribution from the CMake project() definition.
if(WIN32)
  enable_language(RC)
  set(SCENECHANGE_RC_COMPANY "Tatsh")
  set(SCENECHANGE_RC_COPYRIGHT "Copyright (C) 2012 Oka Motofumi; LGPL-2.1-or-later")

  set(SCENECHANGE_RC_DESCRIPTION "Scene change detection plugin for VapourSynth")
  set(SCENECHANGE_RC_INTERNAL_NAME "scenechange")
  set(SCENECHANGE_RC_ORIGINAL_FILENAME "scenechange.dll")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.rc.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/scenechange.rc" @ONLY)
  target_sources(scenechange PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/scenechange.rc")

  set(SCENECHANGE_RC_DESCRIPTION "VapourSynth TemporalSoften filter")
  set(SCENECHANGE_RC_INTERNAL_NAME "temporalsoften2")
  set(SCENECHANGE_RC_ORIGINAL_FILENAME "temporalsoften2.dll")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.rc.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/temporalsoften2.rc" @ONLY)
  target_sources(temporalsoften2 PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/temporalsoften2.rc")
elseif(APPLE)
  set(SCENECHANGE_PLIST_COPYRIGHT "Copyright (C) 2012 Oka Motofumi; LGPL-2.1-or-later")

  set(SCENECHANGE_PLIST_NAME "scenechange")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/scenechange.plist" @ONLY)
  target_link_options(scenechange PRIVATE
    "LINKER:-sectcreate,__TEXT,__info_plist,${CMAKE_CURRENT_BINARY_DIR}/scenechange.plist")

  set(SCENECHANGE_PLIST_NAME "temporalsoften2")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/temporalsoften2.plist" @ONLY)
  target_link_options(temporalsoften2 PRIVATE
    "LINKER:-sectcreate,__TEXT,__info_plist,${CMAKE_CURRENT_BINARY_DIR}/temporalsoften2.plist")
endif()

if(BUILD_PYTHON_PACKAGE)
  # Python wheel build: bundle the plugins as package data so they land in
  # site-packages/scenechange/ and can be located through importlib.resources
  # by scenechange.TemporalSoften.load_plugins().
  install(TARGETS scenechange temporalsoften2
          LIBRARY DESTINATION scenechange
          RUNTIME DESTINATION scenechange)
else()
  if(MSVC)
    # MSVC builds ship as a flat ZIP that users extract into their VapourSynth
    # plugin directory. MSYS2/MinGW Windows builds install into an FHS-like
    # prefix instead, so they follow the same layout as Linux and macOS.
    set(SCENECHANGE_PLUGIN_DEST ".")
    set(SCENECHANGE_DOC_DEST ".")
  else()
    set(SCENECHANGE_PLUGIN_DEST "${VAPOURSYNTH_PLUGINS_DIR}")
    set(SCENECHANGE_DOC_DEST "${CMAKE_INSTALL_DOCDIR}")
  endif()

  install(TARGETS scenechange temporalsoften2
          LIBRARY DESTINATION "${SCENECHANGE_PLUGIN_DEST}"
          RUNTIME DESTINATION "${SCENECHANGE_PLUGIN_DEST}")

  install(FILES "${CMAKE_SOURCE_DIR}/LICENSE.txt" "${CMAKE_SOURCE_DIR}/README.md"
          DESTINATION "${SCENECHANGE_DOC_DEST}")
endif()
