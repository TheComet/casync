cmake_minimum_required (VERSION 3.24)

project (casync
    LANGUAGES C ASM
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
else ()
    set (CASYNC_ASSEMBLER "gas")
endif ()

option (CASYNC_EXAMPLE "Build the example program" OFF)
set (CASYNC_ABI "${CASYNC_ABI}" CACHE STRING "Select the target ABI")
set (CASYNC_ARCH "${CASYNC_ARCH}" CACHE STRING "Select the target architecture")
set (CASYNC_ASSEMBLER "${CASYNC_ASSEMBLER}" CACHE STRING "Select the assembler")
set_property (CACHE CASYNC_ABI
    PROPERTY STRINGS "i386;sysv64;win64")
set_property (CACHE CASYNC_ARCH
    PROPERTY STRINGS "x86;x86_64")
set_property (CACHE CASYNC_ASSEMBLER
    PROPERTY STRINGS "gas;masm")

add_library (casync STATIC
    "src/casync.c"
    "src/sleep_posix.c"
    "src/arch/stack_${CASYNC_ARCH}_${CASYNC_ABI}.c"
    "src/arch/yield_${CASYNC_ASSEMBLER}_${CASYNC_ARCH}_${CASYNC_ABI}.s")
target_include_directories (casync PUBLIC
    "include")
target_compile_options (casync PRIVATE
    -Wall -Wextra)

if (CASYNC_EXAMPLE)
    add_executable (casync_example1 "src/example1.c")
    target_link_libraries (casync_example1 PUBLIC casync)
    target_compile_options (casync_example1 PUBLIC -Wall -Wextra)

    add_executable (casync_example2 "src/example2.c")
    target_link_libraries (casync_example2 PUBLIC casync)
    target_compile_options (casync_example2 PUBLIC -Wall -Wextra)

    add_executable (casync_example3 "src/example3.c")
    target_link_libraries (casync_example3 PUBLIC casync)
    target_compile_options (casync_example3 PUBLIC -Wall -Wextra)
endif ()
