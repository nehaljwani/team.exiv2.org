cmake_minimum_required(VERSION 3.8)
project(book VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD   98)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(exiv2 REQUIRED CONFIG NAMES exiv2)    # search ${CMAKE_INSTALL_PREFIX}/lib/cmake/exiv2/
add_executable(io      io.cpp     ) # compile this
target_link_libraries(io exiv2lib ) # link exiv2lib

add_executable(visitor  visitor.cpp )
add_executable(tvisitor tvisitor.cpp)
add_executable(args     args.cpp    )
add_executable(dmpf     dmpf.cpp    )

# That's all Folks!
##
