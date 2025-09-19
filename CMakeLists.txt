cmake_minimum_required (VERSION 3.12)

project(glome VERSION 0.0.1)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(CheckLibraryExists)
check_library_exists(m sqrtf "" HAVE_LIB_M)
if (HAVE_LIB_M)
    set(EXTRA_LIBS ${EXTRA_LIBS} m)
endif (HAVE_LIB_M)

# Find GLFW3 library
find_package(glfw3)

# Find OpenGL library
include(FindOpenGL)

macro(add_compiler_flag)
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ARGN}")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARGN}")
endmacro(add_compiler_flag)

macro(add_linker_flag)
  set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${ARGN}")
endmacro(add_linker_flag)

# user configurable options
option(EXTERNAL_GLFW "Use external GLFW project" ON)
option(EXTERNAL_GLAD "Use external GLAD project" ON)
option(ENABLE_ASAN "Enable ASAN" OFF)
option(ENABLE_MSAN "Enable MSAN" OFF)
option(ENABLE_UBSAN "Enable UBSAN" OFF)

if(ENABLE_ASAN)
  add_compiler_flag(-fsanitize=address)
  add_linker_flag(-fsanitize=address)
endif()

if(ENABLE_MSAN)
  add_compiler_flag(-fsanitize=memory)
  add_linker_flag(-fsanitize=memory)
endif()

if(ENABLE_UBSAN)
  add_compiler_flag(-fsanitize=undefined)
  add_linker_flag(-fsanitize=undefined)
endif()

message(STATUS "Build external GLFW = ${EXTERNAL_GLFW}")
message(STATUS "Build external GLAD = ${EXTERNAL_GLAD}")

# Support for external glfw
if (EXTERNAL_GLFW)
    include(ExternalProject)
    ExternalProject_Add(
        GLFW
        GIT_REPOSITORY "https://github.com/glfw/glfw.git"
        GIT_TAG "3.3-stable"
        SOURCE_DIR "${CMAKE_BINARY_DIR}/third_party/glfw3"
        STEP_TARGETS build
        EXCLUDE_FROM_ALL TRUE
        CMAKE_ARGS -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF
                   -DGLFW_BUILD_DOCS=OFF -DGLFW_INSTALL=OFF
    )
    set(GLFW_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/third_party/glfw3/include")
    set(GLFW_LIBRARY_DIR "${CMAKE_BINARY_DIR}/GLFW-prefix/src/GLFW-build/src/")
    set(GLFW_LIBRARIES glfw3 pthread dl)
    include_directories(${GLFW_INCLUDE_DIRS})
    link_directories(${GLFW_LIBRARY_DIR})
    list(APPEND GLFW_LIBS_ALL ${GLFW_LIBRARIES} ${EXTRA_LIBS})
    if(UNIX AND NOT APPLE)
        find_package(X11 REQUIRED)
        list(APPEND GLFW_LIBS_ALL X11::X11)
        include_directories(${X11_INCLUDE_DIR})
    endif()
else ()
    add_definitions(${GLFW3_CFLAGS})
    list(APPEND GLFW_LIBS_ALL glfw ${EXTRA_LIBS})
endif ()

# Support for external glad
if (EXTERNAL_GLAD)
    include(ExternalProject)
    ExternalProject_Add(
        GLAD
        GIT_REPOSITORY "https://github.com/Dav1dde/glad.git"
        SOURCE_DIR "${CMAKE_BINARY_DIR}/third_party/glad"
        STEP_TARGETS build
        EXCLUDE_FROM_ALL TRUE
        CMAKE_ARGS -DGLAD_EXPORT=OFF
    )
    set(GLAD_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/GLAD-prefix/src/GLAD-build/include/")
    set(GLAD_LIBRARY_DIR "${CMAKE_BINARY_DIR}/GLAD-prefix/src/GLAD-build/")
    set(GLAD_LIBRARIES glad)
    add_definitions(-DHAVE_GLAD)
    include_directories(${GLAD_INCLUDE_DIRS})
    link_directories(${GLAD_LIBRARY_DIR})
    list(APPEND GLFW_LIBS_ALL ${GLAD_LIBRARIES})
else ()
    list(APPEND GLFW_LIBS_ALL ${OPENGL_opengl_LIBRARY})
endif ()

# Find FreeType2 library
find_package(Freetype REQUIRED)
list(APPEND GLFW_LIBS_ALL Freetype::Freetype)
include_directories(${FREETYPE_INCLUDE_DIRS})

# macro to add opengl example
macro(gl_cprogram prog)
    target_link_libraries(${prog} ${GLFW_LIBS_ALL})
    if (EXTERNAL_GLAD)
        target_compile_definitions(${prog} PRIVATE -DHAVE_GLAD)
    endif ()
    if (EXTERNAL_GLFW)
        add_dependencies(${prog} GLFW-build)
    endif ()
    if (EXTERNAL_GLAD)
        add_dependencies(${prog} GLAD-build)
    endif ()
endmacro()

#
# glome
#

include_directories(include)
include_directories(${FT2_INCLUDE_DIRS})

include_directories(third_party/nanovg/src)
add_library(nanovg STATIC third_party/nanovg/src/nanovg.c)
list(APPEND GLFW_LIBS_ALL nanovg)

include_directories(third_party/stb)

foreach(prog IN ITEMS glome)
  add_executable(${prog} src/${prog}.c)
  gl_cprogram(${prog})
endforeach()
