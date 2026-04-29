if(NOT DEFINED EXECUTABLE)
  message(FATAL_ERROR "EXECUTABLE is required")
endif()

if(NOT EXISTS "${EXECUTABLE}")
  message(FATAL_ERROR "NativeDesktop client executable does not exist: ${EXECUTABLE}")
endif()

if(NOT DEFINED BOOTSTRAP_FRAMES)
  set(BOOTSTRAP_FRAMES 3)
endif()
if(NOT DEFINED GAMEPLAY_FRAMES)
  set(GAMEPLAY_FRAMES 10)
endif()
if(NOT DEFINED WAIT_MS)
  set(WAIT_MS 90000)
endif()

if(BOOTSTRAP_FRAMES LESS 1)
  message(FATAL_ERROR "BOOTSTRAP_FRAMES must be greater than zero")
endif()
if(GAMEPLAY_FRAMES LESS 1)
  message(FATAL_ERROR "GAMEPLAY_FRAMES must be greater than zero")
endif()
if(WAIT_MS LESS 1000)
  message(FATAL_ERROR "WAIT_MS must be at least 1000")
endif()

get_filename_component(EXECUTABLE_DIR "${EXECUTABLE}" DIRECTORY)

set(INPUT_SCRIPT "bootstrap:0:focus:true")
string(APPEND INPUT_SCRIPT "|bootstrap:0:mouse-move:80,60")
string(APPEND INPUT_SCRIPT "|gameplay:0:mouse-move:320,240")
string(APPEND INPUT_SCRIPT "|gameplay:0:mouse-down:middle")
string(APPEND INPUT_SCRIPT "|gameplay:1:mouse-up:middle")
string(APPEND INPUT_SCRIPT "|gameplay:2:mouse-delta:4,-2")
string(APPEND INPUT_SCRIPT "|gameplay:3:wheel:120")
string(APPEND INPUT_SCRIPT "|gameplay:4:char:x")
string(APPEND INPUT_SCRIPT "|gameplay:5:key-down:w")
string(APPEND INPUT_SCRIPT "|gameplay:6:key-up:w")

function(native_desktop_output_tail output_var input)
  string(LENGTH "${input}" input_length)
  set(max_output_length 12000)
  if(input_length GREATER max_output_length)
    math(EXPR tail_offset "${input_length} - ${max_output_length}")
    string(SUBSTRING "${input}" ${tail_offset} ${max_output_length} tail)
    set(
      "${output_var}"
      "... output truncated to last ${max_output_length} characters ...\n${tail}"
      PARENT_SCOPE)
  else()
    set("${output_var}" "${input}" PARENT_SCOPE)
  endif()
endfunction()

execute_process(
  COMMAND
    ${CMAKE_COMMAND} -E env
    "MINECRAFT_NATIVE_DESKTOP_BOOTSTRAP_FRAMES=${BOOTSTRAP_FRAMES}"
    "MINECRAFT_NATIVE_DESKTOP_GAMEPLAY_FRAMES=${GAMEPLAY_FRAMES}"
    "MINECRAFT_NATIVE_DESKTOP_WAIT_MS=${WAIT_MS}"
    "MINECRAFT_NATIVE_DESKTOP_INPUT_SCRIPT=${INPUT_SCRIPT}"
    "${EXECUTABLE}"
  WORKING_DIRECTORY "${EXECUTABLE_DIR}"
  TIMEOUT 120
  RESULT_VARIABLE smoke_result
  OUTPUT_VARIABLE smoke_output
  ERROR_VARIABLE smoke_error
)

set(combined_output "${smoke_output}\n${smoke_error}")

if(NOT smoke_result EQUAL 0)
  native_desktop_output_tail(smoke_output_tail "${smoke_output}")
  native_desktop_output_tail(smoke_error_tail "${smoke_error}")
  message(FATAL_ERROR
    "NativeDesktop client startup smoke failed with exit ${smoke_result}\n"
    "stdout:\n${smoke_output_tail}\n"
    "stderr:\n${smoke_error_tail}\n")
endif()

math(EXPR expected_gameplay_frame "${GAMEPLAY_FRAMES} - 1")

set(expected_markers
  "NativeDesktop bootstrap: Minecraft::main ready"
  "NativeDesktop bootstrap: ui ready"
  "NativeDesktop gameplay: network thread started"
  "NativeDesktop network: StartNetworkGame result=1"
  "NativeDesktop gameplay: ready after"
  "NativeDesktop gameplay: frame ${expected_gameplay_frame} end"
  "NativeDesktop gameplay: complete"
  "NativeDesktop bootstrap: loop complete"
)

foreach(expected_marker IN LISTS expected_markers)
  string(FIND "${combined_output}" "${expected_marker}" marker_index)
  if(marker_index LESS 0)
    native_desktop_output_tail(combined_output_tail "${combined_output}")
    message(FATAL_ERROR
      "NativeDesktop client startup smoke output did not contain marker: "
      "${expected_marker}\n"
      "output:\n${combined_output_tail}\n")
  endif()
endforeach()

set(expected_summary_markers
  "NativeDesktop runtime summary:"
  "phase=complete"
  "failure=none"
  "exitCode=0"
  "startup.mediaArchive=1"
  "startup.stringTable=1"
  "startup.clipboard=1"
  "startup.xuid=1"
  "startup.profile=1"
  "startup.networkManager=1"
  "startup.qnet=1"
  "startup.localPlayer=1"
  "startup.minecraftRuntime=1"
  "startup.ui=1"
  "startup.bundledDLC=1"
  "startupComplete=1"
  "bootstrapFrames=${BOOTSTRAP_FRAMES}/${BOOTSTRAP_FRAMES}"
  "gameplayThreadStarted=1"
  "gameplayReady=1"
  "gameplayFrames=${GAMEPLAY_FRAMES}/${GAMEPLAY_FRAMES}"
  "shutdownRequested=1"
  "leaveGameComplete=1"
  "networkThreadStopped=1"
  "shutdownComplete=1"
  "loopComplete=1"
  "runtimeHealthy=1"
  "runtime.gameStarted=1"
  "runtime.levelReady=1"
  "runtime.playerReady=1"
  "runtime.screenCleared=1"
  "runtime.menuObserved=1"
  "qnet.hostObserved=1"
  "qnet.gameplayObserved=1"
  "qnet.playersMax=1"
  "input.scriptLoaded=1"
  "input.scriptEvents=10"
  "input.invalidEvents=0"
  "input.appliedEvents=10"
  "input.lastAppliedPhase=gameplay"
  "input.lastAppliedFrame=6"
  "input.any=1"
  "input.kbmActive=1"
  "input.mouse=320,240"
)

foreach(expected_summary_marker IN LISTS expected_summary_markers)
  string(FIND
    "${combined_output}"
    "${expected_summary_marker}"
    summary_marker_index)
  if(summary_marker_index LESS 0)
    native_desktop_output_tail(combined_output_tail "${combined_output}")
    message(FATAL_ERROR
      "NativeDesktop client startup smoke output did not contain summary "
      "marker: ${expected_summary_marker}\n"
      "output:\n${combined_output_tail}\n")
  endif()
endforeach()

string(REGEX MATCH
  "startup[.]uiSkinLibraries=[1-9][0-9]*"
  ui_skin_libraries_marker
  "${combined_output}")
if(ui_skin_libraries_marker STREQUAL "")
  native_desktop_output_tail(combined_output_tail "${combined_output}")
  message(FATAL_ERROR
    "NativeDesktop client startup smoke output did not contain a positive "
    "startup.uiSkinLibraries summary value\n"
    "output:\n${combined_output_tail}\n")
endif()

string(REGEX MATCH
  "startup[.]texturePacks=([2-9]|[1-9][0-9]+)"
  texture_pack_count_marker
  "${combined_output}")
if(texture_pack_count_marker STREQUAL "")
  native_desktop_output_tail(combined_output_tail "${combined_output}")
  message(FATAL_ERROR
    "NativeDesktop client startup smoke output did not contain a bundled "
    "texture pack summary value greater than one\n"
    "output:\n${combined_output_tail}\n")
endif()

string(REGEX MATCH
  "gameplayReadyAfterMs=[0-9]+"
  gameplay_ready_after_marker
  "${combined_output}")
if(gameplay_ready_after_marker STREQUAL "")
  native_desktop_output_tail(combined_output_tail "${combined_output}")
  message(FATAL_ERROR
    "NativeDesktop client startup smoke output did not contain a valid "
    "gameplayReadyAfterMs summary value\n"
    "output:\n${combined_output_tail}\n")
endif()

message(STATUS "NativeDesktop client startup smoke passed")
