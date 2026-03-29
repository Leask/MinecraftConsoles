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
    "native world bootstrap=loaded level-id=world"
    "native stub runtime prepared loaded save payload"
    "Failed to start native dedicated server (code -2).")
  string(FIND "${combined_output}" "${expected_marker}" marker_index)
  if(marker_index LESS 0)
    message(FATAL_ERROR
      "Corrupt loaded bootstrap output did not contain expected marker: "
      "${expected_marker}\noutput:\n${combined_output}\n")
  endif()
endforeach()

file(REMOVE "${STORAGE_ROOT}/world.save")
