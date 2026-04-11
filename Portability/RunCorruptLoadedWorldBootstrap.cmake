if(NOT DEFINED EXECUTABLE OR EXECUTABLE STREQUAL "")
  message(FATAL_ERROR "EXECUTABLE is required")
endif()

get_filename_component(EXECUTABLE_DIR "${EXECUTABLE}" DIRECTORY)

if(NOT DEFINED TEST_DIR OR TEST_DIR STREQUAL "")
  set(TEST_DIR "${EXECUTABLE_DIR}/corrupt-loaded-bootstrap-smoke")
endif()

file(REMOVE_RECURSE "${TEST_DIR}")
get_filename_component(TEST_PARENT "${TEST_DIR}" DIRECTORY)
file(MAKE_DIRECTORY "${TEST_PARENT}")
file(MAKE_DIRECTORY "${TEST_DIR}")
set(STORAGE_ROOT "${TEST_DIR}/storage-root")
file(MAKE_DIRECTORY "${STORAGE_ROOT}")
file(WRITE "${STORAGE_ROOT}/world.save" "corrupt-loaded-world")

execute_process(
  COMMAND
    "${EXECUTABLE}"
    --storage-root "${STORAGE_ROOT}"
    -port 0
    -bind 127.0.0.1
    -name NativeCorruptLoaded
  WORKING_DIRECTORY "${TEST_DIR}"
  RESULT_VARIABLE corrupt_result
  OUTPUT_VARIABLE corrupt_output
  ERROR_VARIABLE corrupt_error
)

if(NOT corrupt_result EQUAL 4)
  message(FATAL_ERROR
    "Corrupt loaded bootstrap expected exit 4 but got ${corrupt_result}\n"
    "stdout:\n${corrupt_output}\n"
    "stderr:\n${corrupt_error}\n")
endif()

set(combined_output "${corrupt_output}\n${corrupt_error}")
foreach(expected_marker IN ITEMS
    "failed to load matched world save"
    "native world bootstrap failed")
  string(FIND "${combined_output}" "${expected_marker}" marker_index)
  if(marker_index LESS 0)
    message(FATAL_ERROR
      "Corrupt loaded bootstrap output did not contain expected marker: "
      "${expected_marker}\noutput:\n${combined_output}\n")
  endif()
endforeach()

foreach(forbidden_marker IN ITEMS
    "native bootstrap bound dedicated listener"
    "native bootstrap shell running"
    "persisted native save stub"
    "native bootstrap completed in bootstrap-only mode")
  string(FIND "${combined_output}" "${forbidden_marker}" marker_index)
  if(NOT marker_index LESS 0)
    message(FATAL_ERROR
      "Corrupt loaded bootstrap unexpectedly reached post-load marker: "
      "${forbidden_marker}\noutput:\n${combined_output}\n")
  endif()
endforeach()

file(READ "${STORAGE_ROOT}/world.save" corrupt_save_text)
if(NOT corrupt_save_text STREQUAL "corrupt-loaded-world")
  message(FATAL_ERROR
    "Corrupt loaded bootstrap rewrote the corrupt save before failing\n"
    "save file:\n${corrupt_save_text}\n")
endif()

file(REMOVE "${STORAGE_ROOT}/world.save")
