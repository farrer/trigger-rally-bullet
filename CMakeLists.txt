cmake_minimum_required(VERSION 2.8)

set(PACKAGE trigger-rally)

# Set Version number
set(VERSION_MAJOR 0)
set(VERSION_MINOR 7)
set(VERSION_PATCH 0)
if(${VERSION_PATCH})
   set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
else(${VERSION_PATCH})
   set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR})
endif(${VERSION_PATCH})
set(PACKAGE_VERSION VERSION)

# Set directories
set(DATADIR ${CMAKE_INSTALL_PREFIX}/share)
set(BIN_DIR "${CMAKE_SOURCE_DIR}/bin")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})

project(trigger-rally)

# Some compiler options
add_definitions(-Wall -std=c++11 -DNDEBUG)

# First, try to find required libraries
FIND_PACKAGE(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

FIND_PACKAGE(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

FIND_PACKAGE(SDL2_image REQUIRED)
include_directories(${SDL2_IMAGE_INCLUDE_DIR})

FIND_PACKAGE(OpenAL REQUIRED)
include_directories(${OPENAL_INCLUDE_DIR})

# include the ./src as default too
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/include)
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR}/include)

# Include the sources
include(sources.cmake)
configure_file("./src/include/config.h.in" "./include/config.h")

# Make Libraries
add_library(triggerlib ${TRIGGER_LIB_SOURCES} ${TRIGGER_HEADERS} )
add_library(penginelib ${PENGINE_LIB_SOURCES} ${TRIGGER_HEADERS} )
add_library(psimlib ${PSIM_LIB_SOURCES} ${TRIGGER_HEADERS} )
add_library(tinyxml2lib ${TINYXML2_LIB_SOURCES} ${TRIGGER_HEADERS} )

# TODO add script for search alut, GLEW GLU and other missing ones.
set(LIBRARIES 
   ${SDL2_IMAGE_LIBRARY} ${SDL2_LIBRARY} 
   m ${OPENGL_LIBRARY} GLU GLEW physfs
   ${OPENAL_LIBRARY} alut pthread
)

# Make Binaries
add_executable(trigger-rally WIN32 ${TRIGGER_SOURCES} ${TRIGGER_HEADERS} )
target_link_libraries(trigger-rally triggerlib penginelib psimlib tinyxml2lib ${LIBRARIES})

# Install files
install(TARGETS trigger-rally DESTINATION bin)
# TODO: check if installing data correctly.
install(DIRECTORY ./data DESTINATION share/${PACKAGE})

