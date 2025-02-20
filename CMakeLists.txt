cmake_minimum_required(VERSION 3.26)
project(OceanFFT)

set(CMAKE_CXX_STANDARD 17)

# Set macOS deployment target to 11.0 to ensure compatibility with MPSImageFFT
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")

# File globbing for source files
file(GLOB SOURCE_FILES main.cpp shader.frag shader.vert)

# Add executable and source files
add_executable(OceanFFT ${SOURCE_FILES}
        Camera.cpp
        Camera.h
        OpenCLFFT.cpp
        OpenCLFFT.h
        IFFT.cpp
        IFFT.h
)

# Find OpenCL
if(APPLE)
    # Link OpenCL and Metal framework for macOS (Apple platform)
    target_link_libraries(OceanFFT PRIVATE "-framework OpenCL")
    target_link_libraries(OceanFFT PRIVATE "-framework Metal")
    target_link_libraries(OceanFFT PRIVATE "-framework MetalPerformanceShaders")
    # Link the Accelerate framework
    target_link_libraries(OceanFFT PRIVATE "-framework Accelerate")
else()
    find_package(OpenCL REQUIRED)
    target_link_libraries(OceanFFT PRIVATE ${OpenCL_LIBRARIES})
endif()

# Add clFFT subdirectory and libraries
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
find_package(OpenGL REQUIRED)
target_link_libraries(OceanFFT PRIVATE OpenGL::GL)

# Additional necessary macOS system libraries or dependencies can be added here if needed.



target_include_directories(OceanFFT PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_directories(OceanFFT PRIVATE ${CMAKE_SOURCE_DIR}/include)
