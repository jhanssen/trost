cmake_minimum_required(VERSION 3.21)
project(trost)

# if (NOT AMIGANDK)
#     set(AMIGANDK /opt/amiga)
# endif()

# set(CMAKE_C_COMPILER ${AMIGANDK}/bin/m68k-amigaos-gcc)
# set(CMAKE_CXX_COMPILER ${AMIGANDK}/bin/m68k-amigaos-g++)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror -fno-rtti -fno-exceptions")

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Werror")

add_subdirectory(src)
