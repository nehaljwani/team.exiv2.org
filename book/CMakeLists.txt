cmake_minimum_required(VERSION 3.8)
project(book VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD   11)
set(CMAKE_CXX_EXTENSIONS ON)

add_executable(visitor     visitor.cpp    )
add_executable(parse       parse.cpp      )
add_executable(tvisitor    tvisitor.cpp   )
add_executable(args        args.cpp       )
add_executable(dmpf        dmpf.cpp       )

if(WIN32)
  find_library(WSOCK32_LIBRARY wsock32)
  find_library(WS2_32_ LIBRARY ws2_32)
  target_link_libraries(parse    wsock32 ws2_32)
  target_link_libraries(tvisitor wsock32 ws2_32)
else()
    set(SANITIZER_FLAGS "-fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all")
    # ASAN does not work on Windows
    if ( NOT CYGWIN AND NOT MINGW AND NOT MSYS )
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
   endif()
endif()

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
