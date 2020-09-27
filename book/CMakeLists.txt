cmake_minimum_required(VERSION 3.8)
project(book VERSION 0.0.1 LANGUAGES CXX)
include(CheckCXXCompilerFlag)

set(CMAKE_CXX_STANDARD   11)
set(CMAKE_CXX_EXTENSIONS ON)

if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    message(FATAL_ERROR "32 bit build is not supported")  
    error()
endif()

option( EXIV2_TEAM_USE_SANITIZERS     "Enable ASAN and UBSAN when available"                  OFF )

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
endif()

# ASAN 
if ( EXIV2_TEAM_USE_SANITIZERS ) 
  #I do not support this on Windows
  if ( NOT CYGWIN AND NOT MINGW AND NOT MSYS AND NOT MSVC )
    check_cxx_compiler_flag(                                -fno-omit-frame-pointer       HAS_NO_EMIT)
    if(HAS_NO_EMIT)
      add_compile_options(                                  -fno-omit-frame-pointer )
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fno-omit-frame-pointer")
    endif()
    check_cxx_compiler_flag(                                -fsanitize=address,undefined  HAS_FSAU)
    if(HAS_FSAU)
      add_compile_options(                                  -fsanitize=address,undefined )
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
    endif()
    check_cxx_compiler_flag(                                -fno-sanitize-recover=all     HAS_FSRA)
    if(HAS_FSRA)
      add_compile_options(                                  -fno-sanitize-recover=all )
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fno-sanitize-recover=all")
    endif()
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
