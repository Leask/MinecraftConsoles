set(MINECRAFT_PORTABILITY_SOURCES
  "${CMAKE_SOURCE_DIR}/include/lce_filesystem/lce_filesystem.cpp"
  "${CMAKE_SOURCE_DIR}/include/lce_filesystem/lce_filesystem.h"
  "${CMAKE_SOURCE_DIR}/include/lce_time/lce_time.cpp"
  "${CMAKE_SOURCE_DIR}/include/lce_time/lce_time.h"
)

add_library(Minecraft.Portability STATIC ${MINECRAFT_PORTABILITY_SOURCES})

target_include_directories(Minecraft.Portability
  PUBLIC
  "${CMAKE_SOURCE_DIR}/include"
)

target_compile_definitions(Minecraft.Portability PRIVATE
  ${MINECRAFT_SHARED_DEFINES}
)

configure_compiler_target(Minecraft.Portability)

add_executable(Minecraft.Portability.SmokeTest
  "${CMAKE_CURRENT_LIST_DIR}/FilesystemSmokeTest.cpp"
)

target_link_libraries(Minecraft.Portability.SmokeTest PRIVATE
  Minecraft.Portability
)

target_include_directories(Minecraft.Portability.SmokeTest PRIVATE
  "${CMAKE_SOURCE_DIR}/include"
)

configure_compiler_target(Minecraft.Portability.SmokeTest)
