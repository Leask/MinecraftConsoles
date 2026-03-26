set(MINECRAFT_PORTABILITY_SOURCES
  "${CMAKE_SOURCE_DIR}/include/lce_net/lce_lan.cpp"
  "${CMAKE_SOURCE_DIR}/include/lce_net/lce_lan.h"
  "${CMAKE_SOURCE_DIR}/include/lce_filesystem/lce_filesystem.cpp"
  "${CMAKE_SOURCE_DIR}/include/lce_filesystem/lce_filesystem.h"
  "${CMAKE_SOURCE_DIR}/include/lce_net/lce_net.cpp"
  "${CMAKE_SOURCE_DIR}/include/lce_net/lce_net.h"
  "${CMAKE_SOURCE_DIR}/include/lce_stdin/lce_stdin.cpp"
  "${CMAKE_SOURCE_DIR}/include/lce_stdin/lce_stdin.h"
  "${CMAKE_SOURCE_DIR}/include/lce_time/lce_time.cpp"
  "${CMAKE_SOURCE_DIR}/include/lce_time/lce_time.h"
  "${CMAKE_SOURCE_DIR}/include/lce_win32/lce_win32.cpp"
  "${CMAKE_SOURCE_DIR}/include/lce_win32/lce_win32.h"
)

add_library(Minecraft.Portability STATIC ${MINECRAFT_PORTABILITY_SOURCES})

set(MINECRAFT_PORTABILITY_ZLIB_SOURCES
  "${CMAKE_SOURCE_DIR}/Minecraft.Client/Common/zlib/adler32.c"
  "${CMAKE_SOURCE_DIR}/Minecraft.Client/Common/zlib/compress.c"
  "${CMAKE_SOURCE_DIR}/Minecraft.Client/Common/zlib/crc32.c"
  "${CMAKE_SOURCE_DIR}/Minecraft.Client/Common/zlib/deflate.c"
  "${CMAKE_SOURCE_DIR}/Minecraft.Client/Common/zlib/infback.c"
  "${CMAKE_SOURCE_DIR}/Minecraft.Client/Common/zlib/inffast.c"
  "${CMAKE_SOURCE_DIR}/Minecraft.Client/Common/zlib/inflate.c"
  "${CMAKE_SOURCE_DIR}/Minecraft.Client/Common/zlib/inftrees.c"
  "${CMAKE_SOURCE_DIR}/Minecraft.Client/Common/zlib/trees.c"
  "${CMAKE_SOURCE_DIR}/Minecraft.Client/Common/zlib/uncompr.c"
  "${CMAKE_SOURCE_DIR}/Minecraft.Client/Common/zlib/zutil.c"
)

add_library(Minecraft.Portability.Zlib STATIC
  ${MINECRAFT_PORTABILITY_ZLIB_SOURCES}
)

find_package(Threads REQUIRED)

target_include_directories(Minecraft.Portability
  PUBLIC
  "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(Minecraft.Portability PUBLIC Threads::Threads)

target_compile_definitions(Minecraft.Portability PRIVATE
  ${MINECRAFT_SHARED_DEFINES}
)

configure_compiler_target(Minecraft.Portability)
configure_compiler_target(Minecraft.Portability.Zlib)

add_executable(Minecraft.Portability.SmokeTest
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Access/BanManager.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerOptions.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/FileUtils.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/StringUtils.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Console/ServerCliInput.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/ServerLogger.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/vendor/linenoise/linenoise.c"
  "${CMAKE_SOURCE_DIR}/Minecraft.World/FileHeader.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.World/PerformanceTimer.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.World/compression.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.World/system.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.World/ThreadName.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/FilesystemSmokeTest.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/WorldCompressionSmoke.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/WorldCompressionSmoke.h"
  "${CMAKE_CURRENT_LIST_DIR}/WorldFileHeaderSmoke.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/WorldFileHeaderSmoke.h"
  "${CMAKE_CURRENT_LIST_DIR}/WorldSystemSmoke.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/WorldSystemSmoke.h"
)

target_link_libraries(Minecraft.Portability.SmokeTest PRIVATE
  Minecraft.Portability
  Minecraft.Portability.Zlib
)

target_compile_definitions(Minecraft.Portability.SmokeTest PRIVATE
  ${MINECRAFT_SHARED_DEFINES}
)

target_include_directories(Minecraft.Portability.SmokeTest PRIVATE
  "${CMAKE_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/include"
)

configure_compiler_target(Minecraft.Portability.SmokeTest)

if(BUILD_TESTING)
  add_test(
    NAME minecraft_portability_smoke
    COMMAND $<TARGET_FILE:Minecraft.Portability.SmokeTest> "${CMAKE_SOURCE_DIR}"
  )
  set_tests_properties(minecraft_portability_smoke PROPERTIES
    LABELS "native;portability;smoke"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  )
endif()

add_custom_target(Minecraft.Portability.Check
  COMMAND ${CMAKE_CTEST_COMMAND}
    --output-on-failure
    -C $<CONFIG>
    -R "^minecraft_portability_smoke$"
  DEPENDS Minecraft.Portability.SmokeTest
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  COMMENT "Running native portability smoke test"
  VERBATIM
)
