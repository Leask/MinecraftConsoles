if(NOT DEFINED EXECUTABLE OR EXECUTABLE STREQUAL "")
  message(FATAL_ERROR "EXECUTABLE is required")
endif()

get_filename_component(EXECUTABLE_DIR "${EXECUTABLE}" DIRECTORY)

if(NOT DEFINED TEST_DIR OR TEST_DIR STREQUAL "")
  set(TEST_DIR "${EXECUTABLE_DIR}/save-reload-smoke")
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
    --command save
    --shutdown-after-ms 250
    -port 0
    -bind 127.0.0.1
    -name NativeSaveReload
  WORKING_DIRECTORY "${TEST_DIR}"
  RESULT_VARIABLE create_result
  OUTPUT_VARIABLE create_output
  ERROR_VARIABLE create_error
)

if(NOT create_result EQUAL 0)
  message(FATAL_ERROR
    "Create-save run failed with exit ${create_result}\n"
    "stdout:\n${create_output}\n"
    "stderr:\n${create_error}\n")
endif()

set(create_combined_output "${create_output}\n${create_error}")
foreach(expected_marker IN ITEMS
    "native world bootstrap=created-new level-id=world"
    "manual save requested"
    "persisted native save stub #2")
  string(FIND "${create_combined_output}" "${expected_marker}" marker_index)
  if(marker_index LESS 0)
    message(FATAL_ERROR
      "Create-save output did not contain expected marker: "
      "${expected_marker}\noutput:\n${create_combined_output}\n")
  endif()
endforeach()

file(READ "${STORAGE_ROOT}/world.save" saved_world_text)
foreach(expected_save_marker IN ITEMS
    "native-headless-save"
    "startup-mode=created-new"
    "session-phase="
    "payload-bytes=0"
    "payload-checksum=0"
    "host-settings="
    "dedicated-no-local-player=true"
    "world-size-chunks="
    "hell-scale="
    "startup-payload-present=false"
    "startup-payload-validated=true"
    "startup-thread-iterations="
    "startup-thread-duration-ms="
    "session-active=false"
    "session-completed=true"
    "world-action=idle"
    "tick-count="
    "gameplay-iterations="
    "autosave-completions=2")
  string(FIND "${saved_world_text}" "${expected_save_marker}" save_index)
  if(save_index LESS 0)
    message(FATAL_ERROR
      "Create-save file did not contain expected marker: "
      "${expected_save_marker}\n"
      "save file:\n${saved_world_text}\n")
  endif()
endforeach()

file(RENAME
  "${STORAGE_ROOT}/world.save"
  "${STORAGE_ROOT}/renamed-slot.save")

execute_process(
  COMMAND
    "${EXECUTABLE}"
    --storage-root "${STORAGE_ROOT}"
    --bootstrap-only
    -port 0
    -bind 127.0.0.1
    -name NativeSaveReload
  WORKING_DIRECTORY "${TEST_DIR}"
  RESULT_VARIABLE reload_result
  OUTPUT_VARIABLE reload_output
  ERROR_VARIABLE reload_error
)

if(NOT reload_result EQUAL 0)
  message(FATAL_ERROR
    "Reload run failed with exit ${reload_result}\n"
    "stdout:\n${reload_output}\n"
    "stderr:\n${reload_error}\n")
endif()

set(reload_combined_output "${reload_output}\n${reload_error}")
foreach(expected_reload_marker IN ITEMS
    "matched save filename: world"
    "native world bootstrap=loaded level-id=world"
    "native loaded save metadata path="
    "startup=created-new phase="
    "settings=0x"
    "payload-checksum=0x"
    "startup-payload=none"
    "validated=true"
    "world-size="
    "hell-scale="
    "completed=true"
    "gameplay-iterations="
    "native bootstrap completed in bootstrap-only mode")
  string(FIND
    "${reload_combined_output}"
    "${expected_reload_marker}"
    reload_marker_index)
  if(reload_marker_index LESS 0)
    message(FATAL_ERROR
      "Reload output did not contain expected marker: "
      "${expected_reload_marker}\noutput:\n${reload_combined_output}\n")
  endif()
endforeach()

execute_process(
  COMMAND
    "${EXECUTABLE}"
    --storage-root "${STORAGE_ROOT}"
    --command save
    --shutdown-after-ms 250
    -port 0
    -bind 127.0.0.1
    -name NativeSaveReload
  WORKING_DIRECTORY "${TEST_DIR}"
  RESULT_VARIABLE reload_shell_result
  OUTPUT_VARIABLE reload_shell_output
  ERROR_VARIABLE reload_shell_error
)

if(NOT reload_shell_result EQUAL 0)
  message(FATAL_ERROR
    "Reload-shell run failed with exit ${reload_shell_result}\n"
    "stdout:\n${reload_shell_output}\n"
    "stderr:\n${reload_shell_error}\n")
endif()

set(reload_shell_combined_output "${reload_shell_output}\n${reload_shell_error}")
foreach(expected_reload_shell_marker IN ITEMS
    "native world bootstrap=loaded level-id=world"
    "manual save requested"
    "renamed-slot.save")
  string(FIND
    "${reload_shell_combined_output}"
    "${expected_reload_shell_marker}"
    reload_shell_marker_index)
  if(reload_shell_marker_index LESS 0)
    message(FATAL_ERROR
      "Reload-shell output did not contain expected marker: "
      "${expected_reload_shell_marker}\n"
      "output:\n${reload_shell_combined_output}\n")
  endif()
endforeach()

if(EXISTS "${STORAGE_ROOT}/world.save")
  message(FATAL_ERROR
    "Reload-shell unexpectedly created a new world.save instead of "
    "persisting back to the loaded slot")
endif()

file(READ "${STORAGE_ROOT}/renamed-slot.save" reloaded_saved_world_text)
foreach(expected_reloaded_save_marker IN ITEMS
    "native-headless-save"
    "startup-mode=loaded"
    "session-phase="
    "payload-checksum="
    "host-settings="
    "dedicated-no-local-player=true"
    "world-size-chunks="
    "hell-scale="
    "startup-payload-present=true"
    "startup-payload-validated=true"
    "startup-thread-iterations="
    "startup-thread-duration-ms="
    "session-completed=true")
  string(FIND
    "${reloaded_saved_world_text}"
    "${expected_reloaded_save_marker}"
    reloaded_save_index)
  if(reloaded_save_index LESS 0)
    message(FATAL_ERROR
      "Reload-shell save file did not contain expected marker: "
      "${expected_reloaded_save_marker}\n"
      "save file:\n${reloaded_saved_world_text}\n")
  endif()
endforeach()

file(REMOVE "${STORAGE_ROOT}/renamed-slot.save")
