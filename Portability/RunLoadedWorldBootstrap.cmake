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
file(MAKE_DIRECTORY "${TEST_DIR}")
set(STORAGE_ROOT "${TEST_DIR}/storage-root")
file(REMOVE_RECURSE "${STORAGE_ROOT}")
file(MAKE_DIRECTORY "${STORAGE_ROOT}")
file(WRITE "${STORAGE_ROOT}/world.save" "loaded-smoke")

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
  list(APPEND run_args --command status --command stop)
  list(APPEND expected_markers
    "native bootstrap shell running"
    "stop requested")
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

file(REMOVE "${STORAGE_ROOT}/world.save")
