cmake_minimum_required(VERSION 3.24)

if(WIN32)
  project(MinecraftConsoles LANGUAGES C CXX RC ASM_MASM)
else()
  project(MinecraftConsoles LANGUAGES C CXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

option(MINECRAFT_BUILD_PORTABILITY
  "Build shared native portability modules."
  ON
)
option(MINECRAFT_BUILD_WORLD
  "Build the Minecraft.World static library."
  ${WIN32}
)
option(MINECRAFT_BUILD_CLIENT
  "Build the desktop client target."
  ${WIN32}
)
option(MINECRAFT_BUILD_SERVER
  "Build the dedicated server target."
  ${WIN32}
)

if(NOT DEFINED PLATFORM_NAME)
  if(WIN32)
    set(PLATFORM_NAME "Windows64" CACHE STRING "Target platform name")
  else()
    set(PLATFORM_NAME "NativeDesktop" CACHE STRING "Target platform name")
  endif()
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Use a 64-bit generator/toolchain (x64).")
endif()

set(CMAKE_CONFIGURATION_TYPES
  "Debug"
  "Release"
  CACHE STRING "" FORCE
)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

function(configure_compiler_target target)
  # MSVC and compatible compilers (like Clang-cl)
  if (MSVC)
    target_compile_options(${target} PRIVATE
      $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANGUAGE:C,CXX>>:/W3>
      $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:C,CXX>>:/W0>
      $<$<COMPILE_LANGUAGE:C,CXX>:/MP>
      $<$<COMPILE_LANGUAGE:C,CXX>:/FS>
      $<$<COMPILE_LANGUAGE:C,CXX>:/GS>
      $<$<COMPILE_LANGUAGE:CXX>:/EHsc>
      $<$<COMPILE_LANGUAGE:CXX>:/GR>
      $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANGUAGE:C,CXX>>:/Od>
      $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:C,CXX>>:/O2 /Oi /GT /GF>
    )
  endif()

  # MSVC
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${target} PRIVATE
      $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:C,CXX>>:/GL>
    )
    target_link_options(${target} PRIVATE
      $<$<CONFIG:Release>:/LTCG:incremental>
    )
  endif()

  # Clang
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${target} PRIVATE
      $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANGUAGE:C,CXX>>:-O0 -Wall>
      $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:C,CXX>>:-O2 -w -flto>
    )
    target_link_options(${target} PRIVATE
      $<$<CONFIG:Release>:-flto>
    )
  endif()
endfunction()


# ---
# Configuration
# ---
set(MINECRAFT_SHARED_DEFINES
  _LARGE_WORLDS
  _DEBUG_MENUS_ENABLED
  $<$<CONFIG:Debug>:_DEBUG>
)

if(WIN32)
  list(APPEND MINECRAFT_SHARED_DEFINES
    _CRT_NON_CONFORMING_SWPRINTFS
    _CRT_SECURE_NO_WARNINGS
  )
endif()

# Add platform-specific defines
list(APPEND MINECRAFT_SHARED_DEFINES ${PLATFORM_DEFINES})

message(STATUS "Platform name: ${PLATFORM_NAME}")
message(STATUS "Build portability target: ${MINECRAFT_BUILD_PORTABILITY}")
message(STATUS "Build Minecraft.World: ${MINECRAFT_BUILD_WORLD}")
message(STATUS "Build Minecraft.Client: ${MINECRAFT_BUILD_CLIENT}")
message(STATUS "Build Minecraft.Server: ${MINECRAFT_BUILD_SERVER}")

# ---
# Sources
# ---
if(MINECRAFT_BUILD_PORTABILITY)
  add_subdirectory(Portability)
endif()

if(MINECRAFT_BUILD_WORLD)
  add_subdirectory(Minecraft.World)
endif()

if(MINECRAFT_BUILD_CLIENT)
  add_subdirectory(Minecraft.Client)
endif()

if(MINECRAFT_BUILD_SERVER AND PLATFORM_NAME STREQUAL "Windows64")
  add_subdirectory(Minecraft.Server)
endif()

# ---
# Build versioning
# ---
set(BUILDVER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateBuildVer.cmake")
set(BUILDVER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/Common/BuildVer.h")

add_custom_target(GenerateBuildVer
  COMMAND ${CMAKE_COMMAND}
    "-DOUTPUT_FILE=${BUILDVER_OUTPUT}"
    -P "${BUILDVER_SCRIPT}"
  COMMENT "Generating BuildVer.h..."
  VERBATIM
)

if(TARGET Minecraft.World)
  add_dependencies(Minecraft.World GenerateBuildVer)
endif()
if(TARGET Minecraft.Client)
  add_dependencies(Minecraft.Client GenerateBuildVer)
endif()
if(TARGET Minecraft.Server)
  add_dependencies(Minecraft.Server GenerateBuildVer)
endif()

# ---
# Project organisation
# ---
# Set the startup project for Visual Studio
if(TARGET Minecraft.Client)
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Minecraft.Client)
elseif(TARGET Minecraft.Portability.SmokeTest)
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Minecraft.Portability.SmokeTest)
endif()

# Setup folders for Visual Studio, just hides the build targets under a sub folder
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(TARGET GenerateBuildVer PROPERTY FOLDER "Build")
