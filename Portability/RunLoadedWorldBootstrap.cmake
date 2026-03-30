if(NOT DEFINED EXECUTABLE OR EXECUTABLE STREQUAL "")
  message(FATAL_ERROR "EXECUTABLE is required")
endif()

get_filename_component(EXECUTABLE_DIR "${EXECUTABLE}" DIRECTORY)

if(NOT DEFINED TEST_DIR OR TEST_DIR STREQUAL "")
  set(TEST_DIR "${EXECUTABLE_DIR}")
endif()

if(NOT DEFINED RUN_MODE OR RUN_MODE STREQUAL "")
  set(RUN_MODE "bootstrap")
endif()

file(REMOVE_RECURSE "${TEST_DIR}")
get_filename_component(TEST_PARENT "${TEST_DIR}" DIRECTORY)
file(MAKE_DIRECTORY "${TEST_PARENT}")
file(MAKE_DIRECTORY "${TEST_DIR}")
set(STORAGE_ROOT "${TEST_DIR}/storage-root")
file(REMOVE_RECURSE "${STORAGE_ROOT}")
file(MAKE_DIRECTORY "${STORAGE_ROOT}")
file(WRITE
  "${STORAGE_ROOT}/world.save"
  "native-headless-save\n"
  "world=world\n"
  "level-id=world\n"
  "startup-mode=loaded\n"
  "session-phase=stopped\n"
  "host=LoadedSmoke\n"
  "bind=127.0.0.1\n"
  "payload-name=world\n"
  "seed=1234\n"
  "payload-bytes=0\n"
  "payload-checksum=0\n"
  "save-generation=0\n"
  "state-checksum=0\n"
  "host-settings=0\n"
  "dedicated-no-local-player=true\n"
  "world-size-chunks=54\n"
  "hell-scale=3\n"
  "session-active=false\n"
  "world-action=idle\n"
  "app-shutdown=false\n"
  "gameplay-halted=false\n"
  "initial-save-requested=false\n"
  "initial-save-completed=false\n"
  "initial-save-timed-out=false\n"
  "session-completed=true\n"
  "requested-app-shutdown=false\n"
  "shutdown-halted=false\n"
  "configured-port=0\n"
  "listener-port=0\n"
  "public-slots=16\n"
  "startup-payload-present=true\n"
  "startup-payload-validated=true\n"
  "startup-thread-iterations=4\n"
  "startup-thread-duration-ms=20\n"
  "accepted-connections=0\n"
  "remote-commands=0\n"
  "autosave-requests=0\n"
  "autosave-completions=0\n"
  "tick-count=0\n"
  "uptime-ms=0\n"
  "gameplay-iterations=0\n"
  "saved-at-filetime=0\n")

set(run_args
  --storage-root "${STORAGE_ROOT}"
  -port 0
  -bind 127.0.0.1
  -name NativeLoadedBootstrap)
set(expected_markers
  "native world bootstrap=loaded level-id=world")

if(RUN_MODE STREQUAL "bootstrap")
  list(PREPEND run_args --bootstrap-only)
elseif(RUN_MODE STREQUAL "shell")
  list(APPEND run_args --command save --command status --shutdown-after-ms 250)
  list(APPEND expected_markers
    "native bootstrap shell running"
    "manual save requested"
    "world-action=busy"
    "status runtime=loaded"
    "status payload settings=0x"
    "startup-payload=present"
    "validated=true"
    "checksum=0x"
    "state-checksum=0x"
    "thread=invoked"
    "phase="
    "status session active=true"
    "hosted-thread=active"
    "status run initial-save="
    "status save path="
    "ticks="
    "action="
    "payload=world"
    "native bootstrap auto-shutdown after 250ms")
else()
  message(FATAL_ERROR "Unsupported RUN_MODE: ${RUN_MODE}")
endif()

execute_process(
  COMMAND
    "${EXECUTABLE}"
    ${run_args}
  WORKING_DIRECTORY "${TEST_DIR}"
  RESULT_VARIABLE bootstrap_result
  OUTPUT_VARIABLE bootstrap_output
  ERROR_VARIABLE bootstrap_error
)

if(NOT bootstrap_result EQUAL 0)
  message(FATAL_ERROR
    "Loaded bootstrap failed with exit ${bootstrap_result}\n"
    "stdout:\n${bootstrap_output}\n"
    "stderr:\n${bootstrap_error}\n")
endif()

set(combined_output "${bootstrap_output}\n${bootstrap_error}")
foreach(expected_marker IN LISTS expected_markers)
  string(FIND
    "${combined_output}"
    "${expected_marker}"
    loaded_bootstrap_index)

  if(loaded_bootstrap_index LESS 0)
    file(REMOVE "${STORAGE_ROOT}/world.save")
    message(FATAL_ERROR
      "Loaded bootstrap output did not contain expected marker: "
      "${expected_marker}\noutput:\n${combined_output}\n")
  endif()
endforeach()

if(RUN_MODE STREQUAL "shell")
  file(READ "${STORAGE_ROOT}/world.save" saved_world_text)
foreach(expected_save_marker IN ITEMS
    "native-headless-save"
    "startup-mode=loaded"
    "session-phase="
    "payload-name=world"
    "payload-bytes="
    "payload-checksum="
    "save-generation="
    "state-checksum="
    "host-settings="
    "dedicated-no-local-player=true"
    "world-size-chunks="
    "hell-scale="
    "startup-payload-present=true"
    "startup-payload-validated=true"
    "startup-thread-iterations="
    "startup-thread-duration-ms="
    "hosted-thread-active=false"
    "hosted-thread-ticks="
    "session-active=false"
    "session-completed=true"
    "gameplay-iterations="
    "world-action=idle"
    "tick-count="
    "saved-at-filetime="
    "autosave-completions="
    "remote-commands=0")
    string(FIND
      "${saved_world_text}"
      "${expected_save_marker}"
      saved_world_index)
    if(saved_world_index LESS 0)
      message(FATAL_ERROR
        "Saved world file did not contain expected marker: "
        "${expected_save_marker}\n"
        "save file:\n${saved_world_text}\n")
    endif()
  endforeach()
endif()

file(REMOVE "${STORAGE_ROOT}/world.save")
