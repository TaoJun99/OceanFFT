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
find_package(OpenCL REQUIRED)
target_link_libraries(OceanFFT PRIVATE OpenCL)

include_directories(${CMAKE_SOURCE_DIR}/clFFT/include)
link_directories(${CMAKE_SOURCE_DIR}/clFFT/lib64)

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
