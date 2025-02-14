cmake_minimum_required(VERSION 3.26)
project(OceanFFT)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCE_FILES main.cpp shader.frag shader.vert)

add_executable(OceanFFT ${SOURCE_FILES}
        Camera.cpp
        Camera.h
        OpenCLFFT.cpp
        OpenCLFFT.h
)

# Find OpenCL
if(APPLE)
    target_link_libraries(OceanFFT PRIVATE "-framework OpenCL")
else()
    find_package(OpenCL REQUIRED)
    target_link_libraries(OceanFFT PRIVATE ${OpenCL_LIBRARIES})
endif()

message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")


#set(CMAKE_BUILD_RPATH "${CMAKE_SOURCE_DIR}/clFFT/src/library")
#set(CMAKE_INSTALL_RPATH "${CMAKE_SOURCE_DIR}/clFFT/src/library")


add_subdirectory(clFFT/src)
include_directories(${CMAKE_SOURCE_DIR}/clFFT/src/include)
link_directories(${CMAKE_SOURCE_DIR}/clFFT/src/library)
include_directories(${CMAKE_BINARY_DIR}/clFFT/src/include)
target_link_libraries(OceanFFT PRIVATE clFFT)

# Find GLFW
find_package(glfw3 REQUIRED)
target_link_libraries(OceanFFT PRIVATE glfw)

# Find GLEW
find_package(GLEW REQUIRED)
target_link_libraries(OceanFFT PRIVATE GLEW::GLEW)

# Find GLM
find_package(glm REQUIRED)
target_include_directories(OceanFFT PRIVATE ${GLM_INCLUDE_DIRS})

# Link OpenGL
target_link_libraries(OceanFFT PRIVATE ${OPENGL_gl_LIBRARY})

#target_include_directories(Test PRIVATE ${CMAKE_SOURCE_DIR}/include)
#target_link_directories(Test PRIVATE ${CMAKE_SOURCE_DIR}/include)
