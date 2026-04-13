# macOS/Linux single file copy using native tools
#
# Required:
#   COPY_SOURCE – pipe-separated list of source file paths
#   COPY_DEST  – destination directory
cmake_minimum_required(VERSION 3.24)

if(NOT COPY_SOURCE OR NOT COPY_DEST)
  message(FATAL_ERROR "COPY_SOURCE and COPY_DEST must be set.")
endif()

string(REPLACE "|" ";" COPY_SOURCE "${COPY_SOURCE}")

if(CMAKE_HOST_UNIX)
  file(MAKE_DIRECTORY "${COPY_DEST}")

  execute_process(
    COMMAND rsync -av ${COPY_SOURCE} "${COPY_DEST}/"
    RESULT_VARIABLE rs
  )

  if(rs GREATER 0) # Any non-zero exit code indicates an error
    message(FATAL_ERROR "rsync failed (exit code ${rs})")
  endif()
else()
  message(FATAL_ERROR "Asset copying is supported only on macOS/Linux hosts.")
endif()
