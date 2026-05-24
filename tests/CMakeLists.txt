include(FetchContent)

# Build cmocka as a static library inside this project. Avoid building its
# shared library, examples, or self-tests because we only need the link target.
set(WITH_STATIC_LIB ON CACHE BOOL "" FORCE)
set(WITH_SHARED_LIB OFF CACHE BOOL "" FORCE)
set(WITH_CMOCKERY_SUPPORT OFF CACHE BOOL "" FORCE)
set(WITH_EXAMPLES OFF CACHE BOOL "" FORCE)
set(UNIT_TESTING OFF CACHE BOOL "" FORCE)
set(PICKY_DEVELOPER OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  cmocka
  GIT_REPOSITORY https://gitlab.com/cmocka/cmocka.git
  GIT_TAG cmocka-1.1.7
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(cmocka)

# cmocka has named its static target inconsistently across releases; pick
# whichever exists in the version we fetched.
if(TARGET cmocka_static)
  set(_cmocka_target cmocka_static)
elseif(TARGET cmocka-static)
  set(_cmocka_target cmocka-static)
elseif(TARGET cmocka)
  set(_cmocka_target cmocka)
else()
  message(FATAL_ERROR "cmocka target not found after FetchContent_MakeAvailable.")
endif()

# Compile the same backend that the main plugin compiles, so tests exercise
# the kernel implementation the host platform actually ships. Honour the
# SCENECHANGE_FORCE_GENERIC override so the test workflow can exercise the
# generic backend on a SIMD-capable host.
if(SCENECHANGE_FORCE_GENERIC)
  set(_kernel_backend "${CMAKE_SOURCE_DIR}/src/temporalsoften_generic.c")
  set(_kernel_flags "")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|i[3-6]86|x86)$")
  set(_kernel_backend "${CMAKE_SOURCE_DIR}/src/temporalsoften_x86.c")
  if(MSVC)
    set(_kernel_flags "")
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
      set(_kernel_flags /arch:SSE2)
    endif()
  else()
    set(_kernel_flags -msse2 -mfpmath=sse)
  endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
  set(_kernel_backend "${CMAKE_SOURCE_DIR}/src/temporalsoften_aarch64.c")
  set(_kernel_flags "")
else()
  set(_kernel_backend "${CMAKE_SOURCE_DIR}/src/temporalsoften_generic.c")
  set(_kernel_flags "")
endif()

add_executable(test_kernel test_kernel.c "${_kernel_backend}")
target_include_directories(test_kernel PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(test_kernel PRIVATE "${_cmocka_target}" PkgConfig::VAPOURSYNTH)
if(_kernel_flags)
  target_compile_options(test_kernel PRIVATE ${_kernel_flags})
endif()
if(NOT MSVC)
  find_library(SCENECHANGE_TEST_MATH_LIBRARY m)
  if(SCENECHANGE_TEST_MATH_LIBRARY)
    target_link_libraries(test_kernel PRIVATE "${SCENECHANGE_TEST_MATH_LIBRARY}")
  endif()
endif()
add_test(NAME test_kernel COMMAND test_kernel)
