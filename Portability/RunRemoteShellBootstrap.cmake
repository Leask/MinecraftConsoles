if(NOT DEFINED EXECUTABLE OR EXECUTABLE STREQUAL "")
  message(FATAL_ERROR "EXECUTABLE is required")
endif()

function(assert_save_uint_equals save_text key expected_value context)
  string(REGEX MATCH "(^|\n)${key}=([0-9]+)(\n|$)" save_match
    "${save_text}")
  if(NOT save_match)
    message(FATAL_ERROR
      "${context} did not contain parsable ${key}\n"
      "save file:\n${save_text}\n")
  endif()
  set(actual_value "${CMAKE_MATCH_2}")
  if(NOT actual_value STREQUAL "${expected_value}")
    message(FATAL_ERROR
      "${context} expected ${key}=${expected_value}, got "
      "${actual_value}\n"
      "save file:\n${save_text}\n")
  endif()
endfunction()

function(assert_save_uint_at_least save_text key minimum_value context)
  string(REGEX MATCH "(^|\n)${key}=([0-9]+)(\n|$)" save_match
    "${save_text}")
  if(NOT save_match)
    message(FATAL_ERROR
      "${context} did not contain parsable ${key}\n"
      "save file:\n${save_text}\n")
  endif()
  set(actual_value "${CMAKE_MATCH_2}")
  if(actual_value LESS minimum_value)
    message(FATAL_ERROR
      "${context} expected ${key} >= ${minimum_value}, got "
      "${actual_value}\n"
      "save file:\n${save_text}\n")
  endif()
endfunction()

get_filename_component(EXECUTABLE_DIR "${EXECUTABLE}" DIRECTORY)

if(NOT DEFINED TEST_DIR OR TEST_DIR STREQUAL "")
  set(TEST_DIR "${EXECUTABLE_DIR}/remote-shell-smoke")
endif()

file(REMOVE_RECURSE "${TEST_DIR}")
get_filename_component(TEST_PARENT "${TEST_DIR}" DIRECTORY)
file(MAKE_DIRECTORY "${TEST_PARENT}")
file(MAKE_DIRECTORY "${TEST_DIR}")
set(STORAGE_ROOT "${TEST_DIR}/storage-root")
file(MAKE_DIRECTORY "${STORAGE_ROOT}")

execute_process(
  COMMAND
    "${EXECUTABLE}"
    --storage-root "${STORAGE_ROOT}"
    --shell-self-command status
    --shell-self-command save
    --shell-self-command stop
    --require-accepted-connections 1
    --require-remote-commands 3
    --shutdown-after-ms 250
    -port 0
    -bind 127.0.0.1
    -name NativeRemoteShell
  WORKING_DIRECTORY "${TEST_DIR}"
  RESULT_VARIABLE remote_shell_result
  OUTPUT_VARIABLE remote_shell_output
  ERROR_VARIABLE remote_shell_error
)

if(NOT remote_shell_result EQUAL 0)
  message(FATAL_ERROR
    "Remote shell smoke failed with exit ${remote_shell_result}\n"
    "stdout:\n${remote_shell_output}\n"
    "stderr:\n${remote_shell_error}\n")
endif()

set(combined_output "${remote_shell_output}\n${remote_shell_error}")
foreach(expected_marker IN ITEMS
    "remote shell command #1: status"
    "remote shell command #2: save"
    "remote shell command #3: stop"
    "status runtime=created-new"
    "status payload settings=0x"
    "startup-payload=none"
    "validated=true"
    "checksum=0x"
    "state-checksum=0x"
    "thread=invoked"
    "phase="
    "status session active=true"
    "hosted-thread=active"
    "status worker pending="
    "state="
    "pending-commands="
    "last-queued="
    "last-processed="
    "status run initial-save="
    "status save path="
    "ticks="
    "action="
    "payload=none"
    "manual save requested via worker command #"
    "stop requested via worker command #"
    "persisted native save stub #")
  string(FIND "${combined_output}" "${expected_marker}" marker_index)
  if(marker_index LESS 0)
    message(FATAL_ERROR
      "Remote shell output did not contain expected marker: "
      "${expected_marker}\noutput:\n${combined_output}\n")
  endif()
endforeach()

file(READ "${STORAGE_ROOT}/world.save" saved_world_text)
foreach(expected_save_marker IN ITEMS
    "native-headless-save"
    "startup-mode=created-new"
    "session-phase="
    "payload-name="
    "payload-bytes=0"
    "payload-checksum=0"
    "save-generation="
    "state-checksum="
    "host-settings="
    "dedicated-no-local-player=true"
    "world-size-chunks="
    "hell-scale="
    "startup-payload-present=false"
    "startup-payload-validated=true"
    "startup-thread-iterations="
    "startup-thread-duration-ms="
    "hosted-thread-active=false"
    "hosted-thread-ticks="
    "accepted-connections=1"
    "session-active=false"
    "session-completed=true"
    "world-action=idle"
    "app-shutdown=true"
    "gameplay-halted="
    "requested-app-shutdown=true"
    "shutdown-halted=true"
    "worker-pending-ticks="
    "worker-pending-autosave-commands=0"
    "worker-pending-save-commands=0"
    "worker-pending-stop-commands=0"
    "worker-pending-halt-commands=0"
    "worker-ticks="
    "worker-completions="
    "worker-autosave-commands="
    "worker-save-commands="
    "worker-stop-commands="
    "worker-halt-commands="
    "worker-last-queued-command="
    "worker-active-command=0"
    "worker-active-kind=0"
    "worker-active-ticks=0"
    "worker-last-processed-command="
    "worker-last-processed-kind="
    "tick-count="
    "gameplay-iterations="
    "saved-at-filetime="
    "remote-commands=3"
    "autosave-completions=")
  string(FIND "${saved_world_text}" "${expected_save_marker}" save_index)
  if(save_index LESS 0)
    message(FATAL_ERROR
      "Remote shell save file did not contain expected marker: "
      "${expected_save_marker}\n"
      "save file:\n${saved_world_text}\n")
  endif()
endforeach()

string(REGEX MATCH "autosave-completions=([0-9]+)" remote_autosave_match
  "${saved_world_text}")
if(NOT remote_autosave_match)
  message(FATAL_ERROR
    "Remote shell save file did not contain parsable autosave count\n"
    "save file:\n${saved_world_text}\n")
endif()
set(remote_autosave_completions "${CMAKE_MATCH_1}")
if(remote_autosave_completions LESS 2)
  message(FATAL_ERROR
    "Remote shell expected at least 2 autosave completions, got "
    "${remote_autosave_completions}\n"
    "save file:\n${saved_world_text}\n")
endif()

assert_save_uint_equals(
  "${saved_world_text}"
  "accepted-connections"
  "1"
  "Remote shell save file")
assert_save_uint_equals(
  "${saved_world_text}"
  "remote-commands"
  "3"
  "Remote shell save file")
assert_save_uint_at_least(
  "${saved_world_text}"
  "worker-autosave-commands"
  "2"
  "Remote shell save file")
assert_save_uint_at_least(
  "${saved_world_text}"
  "worker-save-commands"
  "1"
  "Remote shell save file")
assert_save_uint_at_least(
  "${saved_world_text}"
  "worker-stop-commands"
  "1"
  "Remote shell save file")
assert_save_uint_at_least(
  "${saved_world_text}"
  "worker-last-queued-command"
  "4"
  "Remote shell save file")
assert_save_uint_at_least(
  "${saved_world_text}"
  "worker-last-processed-command"
  "3"
  "Remote shell save file")

file(REMOVE "${STORAGE_ROOT}/world.save")
