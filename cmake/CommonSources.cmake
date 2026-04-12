set(_INCLUDE_LCE_FILESYSTEM
  "${CMAKE_SOURCE_DIR}/include/lce_filesystem/lce_filesystem.cpp"
  "${CMAKE_SOURCE_DIR}/include/lce_filesystem/lce_filesystem.h"
)
source_group("include/lce_filesystem" FILES ${_INCLUDE_LCE_FILESYSTEM})

set(_INCLUDE_LCE_TIME
  "${CMAKE_SOURCE_DIR}/include/lce_time/lce_time.cpp"
  "${CMAKE_SOURCE_DIR}/include/lce_time/lce_time.h"
)
source_group("include/lce_time" FILES ${_INCLUDE_LCE_TIME})

set(_INCLUDE_LCE_STDIN
  "${CMAKE_SOURCE_DIR}/include/lce_stdin/lce_stdin.cpp"
  "${CMAKE_SOURCE_DIR}/include/lce_stdin/lce_stdin.h"
)
source_group("include/lce_stdin" FILES ${_INCLUDE_LCE_STDIN})

set(_INCLUDE_LCE_NET
  "${CMAKE_SOURCE_DIR}/include/lce_net/lce_lan.cpp"
  "${CMAKE_SOURCE_DIR}/include/lce_net/lce_lan.h"
  "${CMAKE_SOURCE_DIR}/include/lce_net/lce_net.cpp"
  "${CMAKE_SOURCE_DIR}/include/lce_net/lce_net.h"
)
source_group("include/lce_net" FILES ${_INCLUDE_LCE_NET})

set(_INCLUDE_LCE_PROCESS
  "${CMAKE_SOURCE_DIR}/include/lce_process/lce_process.cpp"
  "${CMAKE_SOURCE_DIR}/include/lce_process/lce_process.h"
)
source_group("include/lce_process" FILES ${_INCLUDE_LCE_PROCESS})

set(_INCLUDE_LCE_ABI
  "${CMAKE_SOURCE_DIR}/include/lce_abi/lce_abi.cpp"
  "${CMAKE_SOURCE_DIR}/include/lce_abi/lce_abi.h"
)
source_group("include/lce_abi" FILES ${_INCLUDE_LCE_ABI})

set(_INCLUDE_BUILDVER
  "${CMAKE_SOURCE_DIR}/include/Common/BuildVer.h"
)
source_group("Common" FILES ${_INCLUDE_BUILDVER})

set(SOURCES_COMMON
	${_INCLUDE_LCE_FILESYSTEM}
	${_INCLUDE_LCE_TIME}
	${_INCLUDE_LCE_STDIN}
	${_INCLUDE_LCE_NET}
	${_INCLUDE_LCE_PROCESS}
	${_INCLUDE_LCE_ABI}
	${_INCLUDE_BUILDVER}
)
