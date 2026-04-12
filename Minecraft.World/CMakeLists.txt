include("${CMAKE_CURRENT_LIST_DIR}/cmake/sources/Common.cmake")

include("${CMAKE_SOURCE_DIR}/cmake/CommonSources.cmake")

set(MINECRAFT_WORLD_SOURCES
  ${MINECRAFT_WORLD_COMMON}
  ${SOURCES_COMMON}
)

if(PLATFORM_NAME STREQUAL "NativeDesktop")
  list(REMOVE_ITEM MINECRAFT_WORLD_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/ConsoleSaveFileOriginal.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Connection.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/EffectCommand.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/EnchantItemCommand.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ExperienceCommand.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/GiveItemCommand.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Socket.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TimeCommand.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ToggleDownfallCommand.cpp"
  )
  if(MINECRAFT_BUILD_CLIENT)
    list(APPEND MINECRAFT_WORLD_SOURCES
      "${CMAKE_CURRENT_SOURCE_DIR}/NativeDesktop/NativeDesktopLegacyClientStubs.cpp"
    )
  endif()
endif()

add_library(Minecraft.World STATIC ${MINECRAFT_WORLD_SOURCES})

target_include_directories(Minecraft.World
  PRIVATE
  "${CMAKE_BINARY_DIR}/generated/"
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/include/"
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/x64headers"
)
target_compile_definitions(Minecraft.World PRIVATE
  ${MINECRAFT_SHARED_DEFINES}
  _LIB
)
target_precompile_headers(Minecraft.World PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:stdafx.h>")

configure_compiler_target(Minecraft.World)
