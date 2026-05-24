set(SCENECHANGE_MSVC_WARNINGS /wd4224 /wd4244 /wd4996)
set(SCENECHANGE_GNU_WARNINGS -Wall -Wshadow)

add_library(scenechange MODULE scenechange.c)
target_link_libraries(scenechange PRIVATE PkgConfig::VAPOURSYNTH)
if(MSVC)
  target_compile_options(scenechange PRIVATE ${SCENECHANGE_MSVC_WARNINGS})
else()
  target_compile_options(scenechange PRIVATE ${SCENECHANGE_GNU_WARNINGS})
endif()

set(TEMPORALSOFTEN_SRCS temporalsoften.c)
set(TEMPORALSOFTEN_SIMD_FLAGS "")
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|i[3-6]86|x86)$")
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
              "${CMAKE_SOURCE_DIR}/temporalsoften2.py"
        DESTINATION "${SCENECHANGE_DOC_DEST}")
