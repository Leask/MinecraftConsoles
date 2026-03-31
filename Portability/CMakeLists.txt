set(MINECRAFT_PORTABILITY_SOURCES
  "${CMAKE_SOURCE_DIR}/include/lce_net/lce_lan.cpp"
  "${CMAKE_SOURCE_DIR}/include/lce_net/lce_lan.h"
  "${CMAKE_SOURCE_DIR}/include/lce_filesystem/lce_filesystem.cpp"
  "${CMAKE_SOURCE_DIR}/include/lce_filesystem/lce_filesystem.h"
  "${CMAKE_SOURCE_DIR}/include/lce_net/lce_net.cpp"
  "${CMAKE_SOURCE_DIR}/include/lce_net/lce_net.h"
  "${CMAKE_SOURCE_DIR}/include/lce_process/lce_process.cpp"
  "${CMAKE_SOURCE_DIR}/include/lce_process/lce_process.h"
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
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Access/Access.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Access/BanManager.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Access/WhitelistManager.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerAutosaveTracker.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/AccessStorageUtils.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerBootstrap.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerGameplayLoop.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerHostedGameRuntime.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/NativeDedicatedServerHostedGameCore.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/NativeDedicatedServerHostedGameThread.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/NativeDedicatedServerHostedGameWorker.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/NativeDedicatedServerHostedGameSession.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/NativeDedicatedServerHostedGameRuntime.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/NativeDedicatedServerWorldLoadStorage.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerLifecycle.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/NativeDedicatedServerLoadedSaveState.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/NativeDedicatedServerSaveStub.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerOptions.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerPlatformState.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/NativeDedicatedServerPlatformRuntime.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerRuntime.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerSignalState.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerShutdownPlan.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerSessionConfig.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerSocketBootstrap.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerWorldBootstrap.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerWorldLoadPipeline.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerWorldSaveSelection.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/FileUtils.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/ServerStoragePaths.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/StringUtils.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/ServerProperties.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/WorldManager.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Console/ServerCliInput.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/ServerLogger.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/vendor/linenoise/linenoise.c"
  "${CMAKE_SOURCE_DIR}/Minecraft.World/FileHeader.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.World/PerformanceTimer.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.World/compression.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.World/system.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.World/ThreadName.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/FilesystemSmokeTest.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ServerPropertiesSmoke.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ServerPropertiesSmoke.h"
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

add_executable(Minecraft.Server.NativeBootstrap
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Access/Access.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Access/BanManager.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Access/WhitelistManager.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerAutosaveTracker.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerBootstrap.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerGameplayLoop.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerHeadlessRuntime.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerHeadlessShell.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerHostedGameRuntime.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerLifecycle.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/NativeDedicatedServerHostedGameCore.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/NativeDedicatedServerHostedGameThread.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/NativeDedicatedServerHostedGameWorker.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/NativeDedicatedServerLoadedSaveState.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/NativeDedicatedServerSaveStub.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerOptions.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerPlatformState.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/NativeDedicatedServerHostedGameSession.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/NativeDedicatedServerHostedGameRuntime.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/NativeDedicatedServerSignalHandlers.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/NativeDedicatedServerPlatformRuntime.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/NativeDedicatedServerWorldLoadStorage.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerRuntime.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerSignalState.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerShutdownPlan.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerSessionConfig.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerSocketBootstrap.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerWorldBootstrap.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerWorldLoadPipeline.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/DedicatedServerWorldSaveSelection.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/FileUtils.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/ServerStoragePaths.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/StringUtils.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/ServerLogger.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/ServerProperties.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/WorldManager.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/vendor/linenoise/linenoise.c"
  "${CMAKE_CURRENT_LIST_DIR}/NativeServerBootstrapMain.cpp"
)

target_link_libraries(Minecraft.Server.NativeBootstrap PRIVATE
  Minecraft.Portability
)

target_compile_definitions(Minecraft.Server.NativeBootstrap PRIVATE
  ${MINECRAFT_SHARED_DEFINES}
)

target_include_directories(Minecraft.Server.NativeBootstrap PRIVATE
  "${CMAKE_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/include"
)

configure_compiler_target(Minecraft.Server.NativeBootstrap)

if(BUILD_TESTING)
  add_test(
    NAME minecraft_portability_smoke
    COMMAND $<TARGET_FILE:Minecraft.Portability.SmokeTest> "${CMAKE_SOURCE_DIR}"
  )
  add_test(
    NAME minecraft_native_server_bootstrap
    COMMAND
      $<TARGET_FILE:Minecraft.Server.NativeBootstrap>
      --bootstrap-only
      -port
      0
      -bind
      127.0.0.1
      -name
      NativeBootstrap
  )
  add_test(
    NAME minecraft_native_server_loaded_bootstrap
    COMMAND
      ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:Minecraft.Server.NativeBootstrap>
      -DRUN_MODE=bootstrap
      -DTEST_DIR=$<TARGET_FILE_DIR:Minecraft.Server.NativeBootstrap>/loaded-bootstrap-smoke
      -P
      "${CMAKE_SOURCE_DIR}/Portability/RunLoadedWorldBootstrap.cmake"
  )
  add_test(
    NAME minecraft_native_server_corrupt_loaded_bootstrap
    COMMAND
      ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:Minecraft.Server.NativeBootstrap>
      -DTEST_DIR=$<TARGET_FILE_DIR:Minecraft.Server.NativeBootstrap>/corrupt-loaded-bootstrap-smoke
      -P
      "${CMAKE_SOURCE_DIR}/Portability/RunCorruptLoadedWorldBootstrap.cmake"
  )
  add_test(
    NAME minecraft_native_server_loaded_shell_smoke
    COMMAND
      ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:Minecraft.Server.NativeBootstrap>
      -DRUN_MODE=shell
      -DTEST_DIR=$<TARGET_FILE_DIR:Minecraft.Server.NativeBootstrap>/loaded-shell-smoke
      -P
      "${CMAKE_SOURCE_DIR}/Portability/RunLoadedWorldBootstrap.cmake"
  )
  add_test(
    NAME minecraft_native_server_save_reload_smoke
    COMMAND
      ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:Minecraft.Server.NativeBootstrap>
      -DTEST_DIR=$<TARGET_FILE_DIR:Minecraft.Server.NativeBootstrap>/save-reload-smoke
      -P
      "${CMAKE_SOURCE_DIR}/Portability/RunSaveReloadBootstrap.cmake"
  )
  add_test(
    NAME minecraft_native_server_listener_smoke
    COMMAND
      $<TARGET_FILE:Minecraft.Server.NativeBootstrap>
      --self-connect
      -port
      0
      -bind
      127.0.0.1
      -name
      NativeSelfConnect
  )
  add_test(
    NAME minecraft_native_server_shell_smoke
    COMMAND
      $<TARGET_FILE:Minecraft.Server.NativeBootstrap>
      --shutdown-after-ms
      150
      -port
      0
      -bind
      127.0.0.1
      -name
      NativeShell
  )
  add_test(
    NAME minecraft_native_server_scripted_shell_smoke
    COMMAND
      $<TARGET_FILE:Minecraft.Server.NativeBootstrap>
      --command
      save
      --command
      status
      --command
      help
      --command
      stop
      -port
      0
      -bind
      127.0.0.1
      -name
      NativeScriptedShell
  )
  add_test(
    NAME minecraft_native_server_live_accept_smoke
    COMMAND
      $<TARGET_FILE:Minecraft.Server.NativeBootstrap>
      --shell-self-connect
      --require-accepted-connections
      1
      --shutdown-after-ms
      150
      -port
      0
      -bind
      127.0.0.1
      -name
      NativeLiveAccept
  )
  add_test(
    NAME minecraft_native_server_remote_command_smoke
    COMMAND
      ${CMAKE_COMMAND}
      -DEXECUTABLE=$<TARGET_FILE:Minecraft.Server.NativeBootstrap>
      -DTEST_DIR=$<TARGET_FILE_DIR:Minecraft.Server.NativeBootstrap>/remote-shell-smoke
      -P
      "${CMAKE_SOURCE_DIR}/Portability/RunRemoteShellBootstrap.cmake"
  )
  set_tests_properties(minecraft_portability_smoke PROPERTIES
    LABELS "native;portability;smoke"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  )
  set_tests_properties(minecraft_native_server_bootstrap PROPERTIES
    LABELS "native;server;bootstrap"
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:Minecraft.Server.NativeBootstrap>"
  )
  set_tests_properties(minecraft_native_server_loaded_bootstrap PROPERTIES
    LABELS "native;server;bootstrap;world-load"
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:Minecraft.Server.NativeBootstrap>"
  )
  set_tests_properties(minecraft_native_server_corrupt_loaded_bootstrap
    PROPERTIES
    LABELS "native;server;bootstrap;world-load"
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:Minecraft.Server.NativeBootstrap>"
  )
  set_tests_properties(minecraft_native_server_loaded_shell_smoke PROPERTIES
    LABELS "native;server;bootstrap;shell;world-load"
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:Minecraft.Server.NativeBootstrap>"
  )
  set_tests_properties(minecraft_native_server_save_reload_smoke PROPERTIES
    LABELS "native;server;bootstrap;shell;world-load;save"
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:Minecraft.Server.NativeBootstrap>"
  )
  set_tests_properties(minecraft_native_server_listener_smoke PROPERTIES
    LABELS "native;server;bootstrap;socket"
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:Minecraft.Server.NativeBootstrap>"
  )
  set_tests_properties(minecraft_native_server_shell_smoke PROPERTIES
    LABELS "native;server;bootstrap;shell"
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:Minecraft.Server.NativeBootstrap>"
  )
  set_tests_properties(minecraft_native_server_scripted_shell_smoke PROPERTIES
    LABELS "native;server;bootstrap;shell"
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:Minecraft.Server.NativeBootstrap>"
  )
  set_tests_properties(minecraft_native_server_live_accept_smoke PROPERTIES
    LABELS "native;server;bootstrap;shell;socket"
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:Minecraft.Server.NativeBootstrap>"
  )
  set_tests_properties(minecraft_native_server_remote_command_smoke PROPERTIES
    LABELS "native;server;bootstrap;shell;socket"
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:Minecraft.Server.NativeBootstrap>"
  )
endif()

add_custom_target(Minecraft.Portability.Check
  COMMAND ${CMAKE_CTEST_COMMAND}
    --output-on-failure
    -C $<CONFIG>
    -R "^minecraft_(portability_smoke|native_server_bootstrap|native_server_loaded_bootstrap|native_server_corrupt_loaded_bootstrap|native_server_loaded_shell_smoke|native_server_save_reload_smoke|native_server_listener_smoke|native_server_shell_smoke|native_server_scripted_shell_smoke|native_server_live_accept_smoke|native_server_remote_command_smoke)$"
  DEPENDS
    Minecraft.Portability.SmokeTest
    Minecraft.Server.NativeBootstrap
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  COMMENT "Running native portability smoke test"
  VERBATIM
)
