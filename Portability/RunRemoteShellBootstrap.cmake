if(NOT DEFINED EXECUTABLE OR EXECUTABLE STREQUAL "")
  message(FATAL_ERROR "EXECUTABLE is required")
endif()

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
    "thread=invoked"
    "phase="
    "status session active=true"
    "status run initial-save="
    "status save path="
    "ticks="
    "action="
    "payload=none"
    "manual save requested"
    "persisted native save stub #1")
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
    "session-active=false"
    "session-completed=true"
    "world-action=idle"
    "tick-count="
    "gameplay-iterations="
    "saved-at-filetime="
    "remote-commands=3"
    "autosave-completions=1")
  string(FIND "${saved_world_text}" "${expected_save_marker}" save_index)
  if(save_index LESS 0)
    message(FATAL_ERROR
      "Remote shell save file did not contain expected marker: "
      "${expected_save_marker}\n"
      "save file:\n${saved_world_text}\n")
  endif()
endforeach()

file(REMOVE "${STORAGE_ROOT}/world.save")
