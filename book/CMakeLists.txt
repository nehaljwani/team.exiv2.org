cmake_minimum_required(VERSION 3.8)
project(book VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD   11)
set(CMAKE_CXX_EXTENSIONS ON)

add_executable(visitor     visitor.cpp    )
add_executable(parse       parse.cpp      )
add_executable(tvisitor    tvisitor.cpp   )
add_executable(args        args.cpp       )
add_executable(dmpf        dmpf.cpp       )

# This is intentionally commented off
# See Chapter 10 Testing for discussion about building libtiff
if ( 0 ) 
	include_directories(/usr/local/include)
	link_directories(/usr/local/lib)
	add_executable(create_tiff create_tiff.cpp)
	target_link_libraries(create_tiff z tiff jpeg)
endif()

# That's all Folks!
##
