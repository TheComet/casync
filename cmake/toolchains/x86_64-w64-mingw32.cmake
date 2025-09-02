set (CMAKE_SYSTEM_NAME Windows)

# which compilers to use for C and C++
set (CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set (CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set (CMAKE_ASM_COMPILER x86_64-w64-mingw32-gcc)

# programs in the host environment
set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)


