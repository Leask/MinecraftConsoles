cmake_minimum_required(VERSION 3.24)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
  message(FATAL_ERROR
    "Windows builds are no longer supported on main. "
    "Use the last-windows-compatible branch for the archived Windows build."
  )
endif()

if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" AND
    NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR
    "This branch only targets macOS and Linux native builds."
  )
endif()

project(MinecraftConsoles LANGUAGES C CXX)

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
  ON
)
option(MINECRAFT_BUILD_CLIENT
  "Build the experimental native desktop client target."
  OFF
)

if(NOT DEFINED PLATFORM_NAME)
  set(PLATFORM_NAME "NativeDesktop" CACHE STRING "Target platform name")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Use a 64-bit generator/toolchain (x64).")
endif()

set(CMAKE_CONFIGURATION_TYPES
  "Debug"
  "Release"
  CACHE STRING "" FORCE
)
function(configure_compiler_target target)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    target_compile_options(${target} PRIVATE
      $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANGUAGE:C,CXX>>:-O0 -Wall>
      $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:C,CXX>>:-O2 -w>
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

# Add platform-specific defines
list(APPEND MINECRAFT_SHARED_DEFINES ${PLATFORM_DEFINES})

message(STATUS "Platform name: ${PLATFORM_NAME}")
message(STATUS "Build portability target: ${MINECRAFT_BUILD_PORTABILITY}")
message(STATUS "Build Minecraft.World: ${MINECRAFT_BUILD_WORLD}")
message(STATUS "Build Minecraft.Client: ${MINECRAFT_BUILD_CLIENT}")

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

# ---
# Build versioning
# ---
set(BUILDVER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateBuildVer.cmake")
set(BUILDVER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/Common/BuildVer.h")
get_filename_component(BUILDVER_OUTPUT_DIR "${BUILDVER_OUTPUT}" DIRECTORY)

add_custom_command(
  OUTPUT "${BUILDVER_OUTPUT}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILDVER_OUTPUT_DIR}"
  COMMAND ${CMAKE_COMMAND}
    "-DOUTPUT_FILE=${BUILDVER_OUTPUT}"
    -P "${BUILDVER_SCRIPT}"
  DEPENDS "${BUILDVER_SCRIPT}"
  COMMENT "Generating BuildVer.h..."
  VERBATIM
)

add_custom_target(GenerateBuildVer DEPENDS "${BUILDVER_OUTPUT}")

if(TARGET Minecraft.World)
  add_dependencies(Minecraft.World GenerateBuildVer)
endif()
if(TARGET Minecraft.Client)
  add_dependencies(Minecraft.Client GenerateBuildVer)
endif()
if(TARGET Minecraft.Server.NativeBootstrap AND TARGET Minecraft.World)
  target_link_libraries(Minecraft.Server.NativeBootstrap PRIVATE Minecraft.World)
endif()

if(TARGET Minecraft.Portability.Check AND TARGET Minecraft.World)
  add_dependencies(Minecraft.Portability.Check Minecraft.World)
endif()

# ---
# Project organisation
# ---
if(TARGET Minecraft.Client)
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Minecraft.Client)
elseif(TARGET Minecraft.Portability.SmokeTest)
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Minecraft.Portability.SmokeTest)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(TARGET GenerateBuildVer PROPERTY FOLDER "Build")
