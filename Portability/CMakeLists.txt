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

add_executable(Minecraft.Portability.SmokeTest
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Access/BanManager.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerOptions.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/FileUtils.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/StringUtils.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Console/ServerCliInput.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/ServerLogger.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/vendor/linenoise/linenoise.c"
  "${CMAKE_SOURCE_DIR}/Minecraft.World/PerformanceTimer.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.World/ThreadName.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/FilesystemSmokeTest.cpp"
)

target_link_libraries(Minecraft.Portability.SmokeTest PRIVATE
  Minecraft.Portability
)

target_include_directories(Minecraft.Portability.SmokeTest PRIVATE
  "${CMAKE_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/include"
)

configure_compiler_target(Minecraft.Portability.SmokeTest)
