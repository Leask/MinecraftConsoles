set(BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/NativeDesktop/")

set(_MINECRAFT_CLIENT_NATIVE_DESKTOP_NATIVE
  "${BASE_DIR}/NativeDesktopClientRuntime.cpp"
  "${BASE_DIR}/NativeDesktopClientApp.h"
  "${BASE_DIR}/NativeDesktopClientSaveCatalog.h"
  "${BASE_DIR}/NativeDesktopClientSaveControl.cpp"
  "${BASE_DIR}/NativeDesktopClientSaveControl.h"
  "${BASE_DIR}/NativeDesktopClientStorageControl.cpp"
  "${BASE_DIR}/NativeDesktopClientStorageControl.h"
  "${BASE_DIR}/NativeDesktopClientStubs.h"
  "${BASE_DIR}/NativeDesktop_UIController.h"
)
source_group("NativeDesktop" FILES ${_MINECRAFT_CLIENT_NATIVE_DESKTOP_NATIVE})

set(_MINECRAFT_CLIENT_NATIVE_DESKTOP_AUDIO
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/Audio/SoundEngine.cpp"
)
source_group("Common/Audio" FILES ${_MINECRAFT_CLIENT_NATIVE_DESKTOP_AUDIO})

file(GLOB _MINECRAFT_CLIENT_NATIVE_DESKTOP_UI CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/UI/UI*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/UI/UI*.h"
)
list(APPEND _MINECRAFT_CLIENT_NATIVE_DESKTOP_UI
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/UI/IUIScene_StartGame.cpp"
)
list(REMOVE_ITEM _MINECRAFT_CLIENT_NATIVE_DESKTOP_UI
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/UI/UIControl_Touch.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/UI/UIControl_Touch.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/UI/UIScene_CommandBlockMenu.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/UI/UIScene_CommandBlockMenu.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/UI/UIScene_InGameSaveManagementMenu.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/UI/UIScene_InGameSaveManagementMenu.h"
)
source_group("Common/UI" FILES ${_MINECRAFT_CLIENT_NATIVE_DESKTOP_UI})

set(_MINECRAFT_CLIENT_NATIVE_DESKTOP_NETWORK
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/Network/PlatformNetworkManagerStub.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/Network/PlatformNetworkManagerStub.h"
  "${BASE_DIR}/Network/NativeDesktopNetLayer.cpp"
  "${BASE_DIR}/Network/NativeDesktopNetLayer.h"
  "${BASE_DIR}/Network/NetworkPlayerNative.cpp"
  "${BASE_DIR}/Network/NetworkPlayerNative.h"
)
source_group("Common/Network" FILES
  ${_MINECRAFT_CLIENT_NATIVE_DESKTOP_NETWORK}
)

file(GLOB _MINECRAFT_CLIENT_NATIVE_DESKTOP_ZLIB CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/zlib/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/zlib/*.h"
)
source_group("Common/zlib" FILES ${_MINECRAFT_CLIENT_NATIVE_DESKTOP_ZLIB})

set(_MINECRAFT_CLIENT_NATIVE_DESKTOP_LEGACY_SHARED
  "${BASE_DIR}/Legacy/ShutdownManager.cpp"
  "${BASE_DIR}/Legacy/ShutdownManager.h"
  "${BASE_DIR}/Legacy/winerror.h"
)
source_group("LegacyShared" FILES
  ${_MINECRAFT_CLIENT_NATIVE_DESKTOP_LEGACY_SHARED}
)

set(MINECRAFT_CLIENT_NATIVE_DESKTOP
  ${_MINECRAFT_CLIENT_NATIVE_DESKTOP_AUDIO}
  ${_MINECRAFT_CLIENT_NATIVE_DESKTOP_UI}
  ${_MINECRAFT_CLIENT_NATIVE_DESKTOP_NETWORK}
  ${_MINECRAFT_CLIENT_NATIVE_DESKTOP_ZLIB}
  ${_MINECRAFT_CLIENT_NATIVE_DESKTOP_LEGACY_SHARED}
  ${_MINECRAFT_CLIENT_NATIVE_DESKTOP_NATIVE}
)
