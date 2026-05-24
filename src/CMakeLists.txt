set(SCENECHANGE_MSVC_WARNINGS /wd4224 /wd4244 /wd4996)
set(SCENECHANGE_GNU_WARNINGS -Wall -Wshadow)

add_library(scenechange MODULE scenechange.c)
target_link_libraries(scenechange PRIVATE PkgConfig::VAPOURSYNTH)
if(MSVC)
  target_compile_options(scenechange PRIVATE ${SCENECHANGE_MSVC_WARNINGS})
else()
  target_compile_options(scenechange PRIVATE ${SCENECHANGE_GNU_WARNINGS})
endif()

add_library(temporalsoften2 MODULE temporalsoften.c)
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
  target_compile_options(temporalsoften2 PRIVATE ${SCENECHANGE_GNU_WARNINGS} -msse2)
endif()

install(TARGETS scenechange temporalsoften2
        LIBRARY DESTINATION "${VAPOURSYNTH_PLUGINS_DIR}"
        RUNTIME DESTINATION "${VAPOURSYNTH_PLUGINS_DIR}")
