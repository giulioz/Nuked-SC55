cmake_minimum_required (VERSION 3.2...3.5)
project (NukedSC55 VERSION 0.2.0 LANGUAGES C CXX)

include(FindPkgConfig)
include(GNUInstallDirs)
include(CheckCXXCompilerFlag)
include(CheckFunctionExists)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Prefer C++11 standard
set(CMAKE_CXX_STANDARD 11)

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

set(CMAKE_CXX_FLAGS "-O3 -g")

#===========================================================================================

if(NOT WIN32)
    check_function_exists(fstatat FILES_HAS_REALPATH)
    if(FILES_HAS_REALPATH)
        add_definitions(-DFILES_HAS_REALPATH)
    endif()
endif()

include(3rdparty/Utf8Main/utf8main.cmake)

set(SC55_SRC
    src/mcu.cpp src/mcu.h # main() is here!
    src/lcd.cpp src/lcd.h src/lcd_font.h
    src/mcu_interrupt.cpp src/mcu_interrupt.h
    src/mcu_opcodes.cpp src/mcu_opcodes.h
    src/mcu_timer.cpp src/mcu_timer.h
    src/pcm.cpp src/pcm.h
    src/submcu.cpp src/submcu.h

    src/utils/files.cpp src/utils/files.h
    3rdparty/libresample/src/filterkit.c
    3rdparty/libresample/src/resample.c
    3rdparty/libresample/src/resamplesubs.c
)

add_library(nuked-sc55 STATIC ${SC55_SRC} ${UTF8MAIN_SRCS})

target_include_directories(nuked-sc55 PRIVATE 3rdparty/libresample/include)

set(SC55_INSTALL_FILES)

# Copy ROM files into runtime directory
macro(copy_rom_target TargetName TargetFile)
    add_custom_target(sc55_copy_${TargetName} ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${TargetFile}")
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${TargetFile}"
        COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_CURRENT_SOURCE_DIR}/data/${TargetFile}"
                "${CMAKE_CURRENT_BINARY_DIR}/${TargetFile}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/data/${TargetFile}"
    )
    list(APPEND SC55_INSTALL_FILES ${CMAKE_CURRENT_BINARY_DIR}/${TargetFile})
endmacro()

copy_rom_target(back_data back.data)

install(TARGETS nuked-sc55
        EXPORT NukedSC55StaticTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")

install(FILES ${SC55_INSTALL_FILES}
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/nuked-sc55")
