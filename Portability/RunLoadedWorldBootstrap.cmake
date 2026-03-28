if(NOT DEFINED EXECUTABLE OR EXECUTABLE STREQUAL "")
  message(FATAL_ERROR "EXECUTABLE is required")
endif()

get_filename_component(EXECUTABLE_DIR "${EXECUTABLE}" DIRECTORY)

if(NOT DEFINED TEST_DIR OR TEST_DIR STREQUAL "")
  set(TEST_DIR "${EXECUTABLE_DIR}")
endif()

file(REMOVE_RECURSE "${TEST_DIR}")
file(MAKE_DIRECTORY "${TEST_DIR}")
file(REMOVE_RECURSE "${EXECUTABLE_DIR}/NativeDesktop/GameHDD")
file(MAKE_DIRECTORY "${EXECUTABLE_DIR}/NativeDesktop/GameHDD")
file(WRITE "${EXECUTABLE_DIR}/NativeDesktop/GameHDD/world.save" "loaded-smoke")

execute_process(
  COMMAND
    "${EXECUTABLE}"
    --bootstrap-only
    -port
    0
    -bind
    127.0.0.1
    -name
    NativeLoadedBootstrap
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
string(FIND
  "${combined_output}"
  "native world bootstrap=loaded level-id=world"
  loaded_bootstrap_index)

if(loaded_bootstrap_index LESS 0)
  file(REMOVE "${EXECUTABLE_DIR}/NativeDesktop/GameHDD/world.save")
  message(FATAL_ERROR
    "Loaded bootstrap output did not contain expected marker.\n"
    "output:\n${combined_output}\n")
endif()

file(REMOVE "${EXECUTABLE_DIR}/NativeDesktop/GameHDD/world.save")
