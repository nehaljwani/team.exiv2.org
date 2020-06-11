cmake_minimum_required(VERSION 3.8)
project(book VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD   11)
set(CMAKE_CXX_EXTENSIONS ON)

add_executable(visitor  visitor.cpp )
add_executable(parse    parse.cpp   )
add_executable(tvisitor tvisitor.cpp)
add_executable(args     args.cpp    )
add_executable(dmpf     dmpf.cpp    )

# That's all Folks!
##
