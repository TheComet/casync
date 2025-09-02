cmake_minimum_required (VERSION 3.24)

project (casync
    LANGUAGES C
    VERSION 0.0.1)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set (CASYNC_ARCH "x86_64")
    if (WIN32)
        set (CASYNC_ABI "win64")
    else ()
        set (CASYNC_ABI "sysv64")
    endif ()
elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
    set (CASYNC_ARCH "x86")
    set (CASYNC_ABI "i386")
endif ()

if (WIN32)
    set (CASYNC_ASSEMBLER "masm")
    set (CASYNC_PLATFORM "win32")
    set (CASYNC_ASM_EXT "asm")
else ()
    set (CASYNC_ASSEMBLER "gas")
    set (CASYNC_PLATFORM "posix")
    set (CASYNC_ASM_EXT "s")
endif ()

option (CASYNC_EXAMPLE "Build the example program" ON)
set (CASYNC_ABI "${CASYNC_ABI}" CACHE STRING "Select the target ABI")
set (CASYNC_ARCH "${CASYNC_ARCH}" CACHE STRING "Select the target architecture")
set (CASYNC_ASSEMBLER "${CASYNC_ASSEMBLER}" CACHE STRING "Select the assembler")

set_property (CACHE CASYNC_ABI
    PROPERTY STRINGS "i386;sysv64;win64")
set_property (CACHE CASYNC_ARCH
    PROPERTY STRINGS "x86;x86_64")
set_property (CACHE CASYNC_ASSEMBLER
    PROPERTY STRINGS "gas;masm")
    
if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    enable_language (ASM)
endif ()
if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    enable_language (ASM_MASM)
endif ()

add_library (casync STATIC
    "src/casync.c"
    "util/sleep_${CASYNC_PLATFORM}.c"
    "src/arch/stack_${CASYNC_ARCH}_${CASYNC_ABI}.c"
    "src/arch/yield_${CASYNC_ASSEMBLER}_${CASYNC_ARCH}_${CASYNC_ABI}.${CASYNC_ASM_EXT}")
target_include_directories (casync PUBLIC
    "include")
target_compile_options (casync PUBLIC
    $<$<C_COMPILER_ID:GNU>:-Wall -Wextra>)

target_compile_options (casync PUBLIC -fsanitize=address)
target_link_libraries (casync PUBLIC asan)

if (CASYNC_EXAMPLE)
    add_executable (casync_example1 "example/example1.c")
    target_link_libraries (casync_example1 PUBLIC casync)

    add_executable (casync_example2 "example/example2.c")
    target_link_libraries (casync_example2 PUBLIC casync)

    add_executable (casync_example3 "example/example3.c")
    target_link_libraries (casync_example3 PUBLIC casync)
    if (WIN32)
        target_link_libraries (casync_example3 PUBLIC ws2_32)
    endif ()
endif ()
