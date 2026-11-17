cmake_minimum_required(VERSION 3.30)
project(super3_android LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)  # same as upstream (Makefiles/Rules.inc)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../../" ABSOLUTE)

# --- Pre-generated Musashi opcode tables ---
# The code generator (m68kmake) is a host tool; to avoid requiring a full
# Windows toolchain during Gradle builds we check in its outputs under
# android/app/src/main/cpp/generated.
set(M68K_GENERATED_DIR "${CMAKE_CURRENT_LIST_DIR}/generated")
set(M68K_GENERATED_SOURCES
  "${M68K_GENERATED_DIR}/m68kops.c"
  "${M68K_GENERATED_DIR}/m68kopac.c"
  "${M68K_GENERATED_DIR}/m68kopdm.c"
  "${M68K_GENERATED_DIR}/m68kopnz.c"
)
set(M68K_GENERATED_HEADER "${M68K_GENERATED_DIR}/m68kops.h")

foreach(gen_file IN LISTS M68K_GENERATED_SOURCES M68K_GENERATED_HEADER)
  if(NOT EXISTS "${gen_file}")
    message(FATAL_ERROR "Missing ${gen_file}. Run m68kmake <outdir> Src/CPU/68K/Musashi/m68k_in.c to regenerate.")
  endif()
endforeach()

# --- Main Android Library ---
#
# Src/ is an unmodified copy of upstream Supermodel (see Scripts/sync-upstream.sh).
# Android-specific code lives in this directory:
#   override/   headers that replace upstream desktop-OpenGL headers (searched first)
#   generated/  files made by Scripts/sync-upstream.sh (patched upstream copies, tables)

file(GLOB_RECURSE SUPER3_SOURCES
  "${REPO_ROOT}/Src/*.cpp"
  "${REPO_ROOT}/Src/*.c"
)

# Upstream code not built for Android: desktop OpenGL renderers and front end,
# other arcade boards (Model 1/2, Pro-1000), code generators and tests.
list(FILTER SUPER3_SOURCES EXCLUDE REGEX
  "/Src/(Graphics|Debugger|Model1|Model2|Pro1000)/|/Src/OSD/(SDL|Windows|OSX|Unix)/|/Src/OSD/(ConsoleInput|Test_[^/]*)\\.cpp$|/Src/Pkgs/(GL|imgui)/|/Src/Pkgs/glew\\.c$|/Src/CPU/(V60|SHARC|I960|MB86233|MB86235)/|/Src/CPU/68K/Musashi/(m68k_in|m68kmake)\\.c$|/Src/CPU/68K/Turbo68K/|/Src/CPU/PowerPC/(ppc603|ppc_ops)\\.c$|/Src/CPU/PowerPC/PPCDisasm\\.cpp$|/Src/Model3/(53C810Disasm|Model3GraphicsState)\\.cpp$|/Src/Sound/SCSPLFO\\.cpp$|/Src/Sound/MultiPCM\\.cpp$|/Src/Util/Test_[^/]*$")

file(GLOB UPSTREAM_NEW3D_SOURCES "${REPO_ROOT}/Src/Graphics/New3D/*.cpp")
# New3D.cpp and TextureBank.cpp are built from patched copies made by
# Scripts/sync-upstream.sh (steps 6 and 7)
list(FILTER UPSTREAM_NEW3D_SOURCES EXCLUDE REGEX "/(New3D|TextureBank)\\.cpp$")
list(APPEND UPSTREAM_NEW3D_SOURCES "${M68K_GENERATED_DIR}/New3D_android.cpp" "${M68K_GENERATED_DIR}/TextureBank_android.cpp")

# --- SDL2 (for future Android OSD wiring) ---
# Prefer a local checkout if provided via -DSDL2_LOCAL_DIR, otherwise fetch the
# latest release tarball.
option(SUPER3_FETCH_SDL2 "Download and build SDL2 for Android" ON)
set(SDL2_VERSION "2.32.10")
option(SUPER3_FETCH_SDL2_NET "Download and build SDL2_net for Android" ON)
set(SDL2_NET_VERSION "2.2.0")

if(DEFINED SDL2_LOCAL_DIR)
  message(STATUS "Using local SDL2 at ${SDL2_LOCAL_DIR}")
  add_subdirectory("${SDL2_LOCAL_DIR}" "${CMAKE_BINARY_DIR}/sdl2-local")
elseif(SUPER3_FETCH_SDL2)
  FetchContent_Declare(
    SDL2
    URL "https://github.com/libsdl-org/SDL/archive/refs/tags/release-${SDL2_VERSION}.zip"
  )
  FetchContent_MakeAvailable(SDL2)
endif()

if(DEFINED SDL2_NET_LOCAL_DIR)
  message(STATUS "Using local SDL2_net at ${SDL2_NET_LOCAL_DIR}")
  add_subdirectory("${SDL2_NET_LOCAL_DIR}" "${CMAKE_BINARY_DIR}/sdl2-net-local")
elseif(SUPER3_FETCH_SDL2_NET)
  FetchContent_Declare(
    SDL2_NET
    URL "https://github.com/libsdl-org/SDL_net/archive/refs/tags/release-${SDL2_NET_VERSION}.zip"
  )
  FetchContent_MakeAvailable(SDL2_NET)
endif()

set(SDL2_NET_TARGET "")
if(TARGET SDL2_net::SDL2_net)
  set(SDL2_NET_TARGET SDL2_net::SDL2_net)
elseif(TARGET SDL2_net::SDL2_net-static)
  set(SDL2_NET_TARGET SDL2_net::SDL2_net-static)
elseif(TARGET SDL2_net)
  set(SDL2_NET_TARGET SDL2_net)
endif()
if(SDL2_NET_TARGET STREQUAL "")
  message(FATAL_ERROR "SDL2_net target not found. Enable SUPER3_FETCH_SDL2_NET or provide SDL2_NET_LOCAL_DIR.")
endif()

add_library(super3 SHARED
  native-lib.cpp
  android_osd.cpp
  android_input_system.cpp
  gles_presenter.cpp
  render2d_android.cpp
  gles_compat.cpp
  # Upstream's New3D renderer, built for OpenGL ES 3.2 through override/GL/glew.h
  "${REPO_ROOT}/Src/Graphics/Shader.cpp"
  ${UPSTREAM_NEW3D_SOURCES}
  "${M68K_GENERATED_DIR}/Audio_android.cpp"
  "${REPO_ROOT}/Src/OSD/Unix/FileSystemPath.cpp"
  "${M68K_GENERATED_DIR}/UpstreamDefaultConfig.cpp"
  ${SUPER3_SOURCES}
  ${M68K_GENERATED_SOURCES}
)

target_compile_definitions(super3 PRIVATE ANDROID __ANDROID__ NET_BOARD)

# Musashi defines a "uint" macro that clashes with bionic's sys/types.h typedef
# when m68kcpu.c includes stdio.h; seeing the typedef first avoids editing Src/.
set_source_files_properties("${REPO_ROOT}/Src/CPU/68K/Musashi/m68kcpu.c" PROPERTIES COMPILE_OPTIONS "-include;sys/types.h")

target_include_directories(super3 PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/override"
  "${REPO_ROOT}/Src"
  "${REPO_ROOT}/Src/CPU/68K/Musashi"
  "${REPO_ROOT}/Src/OSD"
  "${REPO_ROOT}/Src/OSD/SDL" # Types.h, SDLIncludes.h
  "${M68K_GENERATED_DIR}"
)

find_library(log-lib log)
find_library(android-lib android)
find_library(z-lib z)
find_library(egl-lib EGL)
find_library(glesv3-lib GLESv3)

target_link_libraries(super3 PRIVATE
  ${log-lib}
  ${android-lib}
  ${z-lib}
  ${egl-lib}
  ${glesv3-lib}
  SDL2::SDL2 # present when SUPER3_FETCH_SDL2 is ON
  ${SDL2_NET_TARGET}
)

# Logging. Release builds write nothing to the Android log: every liblog call in
# SUPER3, SDL2 and SDL2_net is redirected to the no-op functions in nolog.c.
# The personal build passes -DSUPER3_LOGGING=ON (fps and frame-time lines, shader errors).
option(SUPER3_LOGGING "Write to the Android log (logcat)" OFF)
if(NOT SUPER3_LOGGING)
  set(SUPER3_NOLOG_WRAP
    -Wl,--wrap=__android_log_write
    -Wl,--wrap=__android_log_print
    -Wl,--wrap=__android_log_vprint
    -Wl,--wrap=__android_log_buf_write
    -Wl,--wrap=__android_log_buf_print
    -Wl,--wrap=__android_log_write_log_message
  )
  foreach(lib super3 SDL2::SDL2 ${SDL2_NET_TARGET})
    set(real "${lib}")
    get_target_property(aliased "${lib}" ALIASED_TARGET)
    if(aliased)
      set(real "${aliased}")
    endif()
    get_target_property(type "${real}" TYPE)
    if(type STREQUAL "SHARED_LIBRARY")
      target_sources("${real}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/nolog.c")
      target_link_options("${real}" PRIVATE ${SUPER3_NOLOG_WRAP})
    endif()
  endforeach()
endif()
