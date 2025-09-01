# the name of the target operating system
set (CMAKE_SYSTEM_NAME Linux)

set (CMAKE_CXX_FLAGS_INIT -m32)
set (CMAKE_C_FLAGS_INIT -m32)
set (CMAKE_ASM_FLAGS_INIT -m32)

# which compilers to use for C and C++
set (CMAKE_CXX_COMPILER g++)
set (CMAKE_C_COMPILER gcc)
set (CMAKE_ASM_COMPILER gcc)

#set (CMAKE_FIND_ROOT_PATH   /usr/i486-linux-gnu )

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

