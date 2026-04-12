# macOS/Linux recursive copy with exclusion support
#
# Required:
#   COPY_SOURCE   – source directory
#   COPY_DEST     – destination directory
#
# Optional:
#   EXCLUDE_FILES – pipe-separated file patterns to exclude
#   EXCLUDE_FOLDERS – pipe-separated folder patterns to exclude

if(NOT COPY_SOURCE OR NOT COPY_DEST)
  message(FATAL_ERROR "COPY_SOURCE and COPY_DEST must be set.")
endif()

# Replace "|" with ";" to convert the exclusion patterns back into a list
if(EXCLUDE_FILES)
  string(REPLACE "|" ";" EXCLUDE_FILES "${EXCLUDE_FILES}")
endif()

if(EXCLUDE_FOLDERS)
  string(REPLACE "|" ";" EXCLUDE_FOLDERS "${EXCLUDE_FOLDERS}")
endif()

message(STATUS "Copying from ${COPY_SOURCE} to ${COPY_DEST}")
file(MAKE_DIRECTORY "${COPY_DEST}")

if(CMAKE_HOST_UNIX)
  set(rsync_args -av)

  foreach(pattern IN LISTS EXCLUDE_FILES)
    list(APPEND rsync_args "--exclude=${pattern}")
  endforeach()

  foreach(pattern IN LISTS EXCLUDE_FOLDERS)
    list(APPEND rsync_args "--exclude=${pattern}")
  endforeach()

  # Trailing slashes ensure rsync copies contents, not the directory itself
  execute_process(
    COMMAND rsync ${rsync_args} "${COPY_SOURCE}/" "${COPY_DEST}/"
    RESULT_VARIABLE rs
  )

  if(rs GREATER 0) # Any non-zero exit code indicates an error
    message(FATAL_ERROR "rsync failed (exit code ${rs})")
  endif()
else()
  message(FATAL_ERROR "Asset copying is supported only on macOS/Linux hosts.")
endif()
