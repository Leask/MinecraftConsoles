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
    "persisted native save stub #")
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
    "save-generation="
    "state-checksum="
    "host-settings="
    "dedicated-no-local-player=true"
    "world-size-chunks="
    "hell-scale="
    "online-game=true"
    "private-game=false"
    "fake-local-player=true"
    "public-slots=16"
    "private-slots=0"
    "startup-payload-present=false"
    "startup-payload-validated=true"
    "startup-thread-iterations="
    "startup-thread-duration-ms="
    "hosted-thread-active=false"
    "hosted-thread-ticks="
    "session-active=false"
    "session-completed=true"
    "world-action=idle"
    "worker-pending-ticks="
    "worker-ticks="
    "worker-completions="
    "tick-count="
    "gameplay-iterations="
    "autosave-completions=")
  string(FIND "${saved_world_text}" "${expected_save_marker}" save_index)
  if(save_index LESS 0)
    message(FATAL_ERROR
      "Create-save file did not contain expected marker: "
      "${expected_save_marker}\n"
      "save file:\n${saved_world_text}\n")
  endif()
endforeach()

string(REGEX MATCH "autosave-completions=([0-9]+)" create_autosave_match
  "${saved_world_text}")
if(NOT create_autosave_match)
  message(FATAL_ERROR
    "Create-save file did not contain parsable autosave count\n"
    "save file:\n${saved_world_text}\n")
endif()
set(create_autosave_completions "${CMAKE_MATCH_1}")
if(create_autosave_completions LESS 2)
  message(FATAL_ERROR
    "Create-save file expected at least 2 autosave completions, got "
    "${create_autosave_completions}\n"
    "save file:\n${saved_world_text}\n")
endif()

string(REGEX MATCH "worker-ticks=([0-9]+)" create_worker_ticks_match
  "${saved_world_text}")
if(NOT create_worker_ticks_match)
  message(FATAL_ERROR
    "Create-save file did not contain parsable worker tick count\n"
    "save file:\n${saved_world_text}\n")
endif()
set(create_worker_ticks "${CMAKE_MATCH_1}")

string(REGEX MATCH "hosted-thread-ticks=([0-9]+)"
  create_hosted_thread_ticks_match
  "${saved_world_text}")
if(NOT create_hosted_thread_ticks_match)
  message(FATAL_ERROR
    "Create-save file did not contain parsable hosted thread tick count\n"
    "save file:\n${saved_world_text}\n")
endif()
set(create_hosted_thread_ticks "${CMAKE_MATCH_1}")

string(REGEX MATCH "worker-completions=([0-9]+)"
  create_worker_completions_match
  "${saved_world_text}")
if(NOT create_worker_completions_match)
  message(FATAL_ERROR
    "Create-save file did not contain parsable worker completion count\n"
    "save file:\n${saved_world_text}\n")
endif()
set(create_worker_completions "${CMAKE_MATCH_1}")

string(REGEX MATCH "gameplay-iterations=([0-9]+)"
  create_gameplay_iterations_match
  "${saved_world_text}")
if(NOT create_gameplay_iterations_match)
  message(FATAL_ERROR
    "Create-save file did not contain parsable gameplay iteration count\n"
    "save file:\n${saved_world_text}\n")
endif()
set(create_gameplay_iterations "${CMAKE_MATCH_1}")

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
    "online=true"
    "private=false"
    "fake-local=true"
    "public-slots=16"
    "private-slots=0"
    "payload-checksum=0x"
    "save-generation="
    "state-checksum=0x"
    "startup-payload=none"
    "validated=true"
    "world-size="
    "hell-scale="
    "completed=true"
    "gameplay-iterations="
    "hosted-thread=stopped"
    "hosted-thread-ticks="
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
    "save-generation="
    "state-checksum="
    "host-settings="
    "dedicated-no-local-player=true"
    "world-size-chunks="
    "hell-scale="
    "online-game=true"
    "private-game=false"
    "fake-local-player=true"
    "public-slots=16"
    "private-slots=0"
    "startup-payload-present=true"
    "startup-payload-validated=true"
    "startup-thread-iterations="
    "startup-thread-duration-ms="
    "hosted-thread-active=false"
    "hosted-thread-ticks="
    "worker-pending-ticks="
    "worker-ticks="
    "worker-completions="
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

string(REGEX MATCH "worker-ticks=([0-9]+)" reload_worker_ticks_match
  "${reloaded_saved_world_text}")
if(NOT reload_worker_ticks_match)
  message(FATAL_ERROR
    "Reloaded save did not contain parsable worker tick count\n"
    "save file:\n${reloaded_saved_world_text}\n")
endif()
set(reload_worker_ticks "${CMAKE_MATCH_1}")
if(reload_worker_ticks LESS create_worker_ticks)
  message(FATAL_ERROR
    "Reloaded save regressed worker tick count: "
    "${reload_worker_ticks} < ${create_worker_ticks}\n"
    "save file:\n${reloaded_saved_world_text}\n")
endif()

string(REGEX MATCH "hosted-thread-ticks=([0-9]+)"
  reload_hosted_thread_ticks_match
  "${reloaded_saved_world_text}")
if(NOT reload_hosted_thread_ticks_match)
  message(FATAL_ERROR
    "Reloaded save did not contain parsable hosted thread tick count\n"
    "save file:\n${reloaded_saved_world_text}\n")
endif()
set(reload_hosted_thread_ticks "${CMAKE_MATCH_1}")
if(reload_hosted_thread_ticks LESS create_hosted_thread_ticks)
  message(FATAL_ERROR
    "Reloaded save regressed hosted thread tick count: "
    "${reload_hosted_thread_ticks} < ${create_hosted_thread_ticks}\n"
    "save file:\n${reloaded_saved_world_text}\n")
endif()

string(REGEX MATCH "worker-completions=([0-9]+)"
  reload_worker_completions_match
  "${reloaded_saved_world_text}")
if(NOT reload_worker_completions_match)
  message(FATAL_ERROR
    "Reloaded save did not contain parsable worker completion count\n"
    "save file:\n${reloaded_saved_world_text}\n")
endif()
set(reload_worker_completions "${CMAKE_MATCH_1}")
if(reload_worker_completions LESS create_worker_completions)
  message(FATAL_ERROR
    "Reloaded save regressed worker completion count: "
    "${reload_worker_completions} < ${create_worker_completions}\n"
    "save file:\n${reloaded_saved_world_text}\n")
endif()

string(REGEX MATCH "gameplay-iterations=([0-9]+)"
  reload_gameplay_iterations_match
  "${reloaded_saved_world_text}")
if(NOT reload_gameplay_iterations_match)
  message(FATAL_ERROR
    "Reloaded save did not contain parsable gameplay iteration count\n"
    "save file:\n${reloaded_saved_world_text}\n")
endif()
set(reload_gameplay_iterations "${CMAKE_MATCH_1}")
if(reload_gameplay_iterations LESS create_gameplay_iterations)
  message(FATAL_ERROR
    "Reloaded save regressed gameplay iteration count: "
    "${reload_gameplay_iterations} < ${create_gameplay_iterations}\n"
    "save file:\n${reloaded_saved_world_text}\n")
endif()

file(REMOVE "${STORAGE_ROOT}/renamed-slot.save")
