include("${CMAKE_CURRENT_LIST_DIR}/cmake/sources/Common.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sources/NativeDesktop.cmake")

include("${CMAKE_SOURCE_DIR}/cmake/CommonSources.cmake")

include("${CMAKE_SOURCE_DIR}/cmake/Utils.cmake")

set(MINECRAFT_CLIENT_SOURCES
  ${MINECRAFT_CLIENT_COMMON}
  ${MINECRAFT_CLIENT_NATIVE_DESKTOP}
  ${SOURCES_COMMON}
)

add_executable(Minecraft.Client ${MINECRAFT_CLIENT_SOURCES})

target_include_directories(Minecraft.Client PRIVATE
  "${CMAKE_BINARY_DIR}/generated/" # This is for the generated BuildVer.h
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/include/"
)
target_compile_definitions(Minecraft.Client PRIVATE
  ${MINECRAFT_SHARED_DEFINES}
  HAVE_UNISTD_H=1
  MINECRAFT_NATIVE_DESKTOP_CLIENT
)
target_precompile_headers(Minecraft.Client PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:stdafx.h>")
set_source_files_properties(compat_shims.cpp PROPERTIES
  SKIP_PRECOMPILE_HEADERS ON
)

configure_compiler_target(Minecraft.Client)

set_target_properties(Minecraft.Client PROPERTIES
  OUTPUT_NAME "Minecraft.Client"
  VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Minecraft.Client>"
)

target_link_libraries(Minecraft.Client PRIVATE
  Minecraft.World
)

# ---
# Asset / redist copy
# ---
include("${CMAKE_SOURCE_DIR}/cmake/CopyAssets.cmake")
set(ASSET_FOLDER_PAIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/music"                  "music"
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/Media"           "Common/Media"
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/res"             "Common/res"
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/Trial"           "Common/Trial"
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/Tutorial"        "Common/Tutorial"
)
setup_asset_folder_copy(Minecraft.Client "${ASSET_FOLDER_PAIRS}")

set(ASSET_FILE_PAIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/Media/MediaNativeDesktop.arc"
  "Common/Media"
)
setup_asset_file_copy(Minecraft.Client "${ASSET_FILE_PAIRS}")

# Copy redist files
add_copyredist_target(Minecraft.Client)

function(add_native_desktop_client_smoke_test
  test_name
  gameplay_frames
  save_restart)
  add_test(
    NAME ${test_name}
    COMMAND
      ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:Minecraft.Client>
      -DTEST_NAME=${test_name}
      -DGAMEPLAY_FRAMES=${gameplay_frames}
      -DSAVE_RESTART=${save_restart}
      -P
      "${CMAKE_CURRENT_SOURCE_DIR}/NativeDesktop/RunNativeDesktopClientSmoke.cmake"
  )
  set_tests_properties(${test_name} PROPERTIES
    LABELS "native;client;startup;gameplay"
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:Minecraft.Client>"
  )
endfunction()

if(BUILD_TESTING AND PLATFORM_NAME STREQUAL "NativeDesktop")
  add_native_desktop_client_smoke_test(
    minecraft_native_desktop_client_startup_smoke
    10
    0)
  add_native_desktop_client_smoke_test(
    minecraft_native_desktop_client_extended_gameplay_smoke
    60
    0)
  add_native_desktop_client_smoke_test(
    minecraft_native_desktop_client_save_restart_smoke
    12
    1)

  add_custom_target(Minecraft.NativeDesktop.Check
    COMMAND ${CMAKE_CTEST_COMMAND}
      --output-on-failure
      -C $<CONFIG>
      -R "^minecraft_native_desktop_client_.*smoke$"
    DEPENDS
      Minecraft.Client
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Running NativeDesktop client startup smoke tests"
    VERBATIM
  )
endif()
