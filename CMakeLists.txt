cmake_minimum_required(VERSION 3.10)
project(prao-compresser)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
find_package(OpenCL REQUIRED)
add_executable(prao-compresser main.cpp
        Calibration/CalibrationDataInput.h Calibration/CalibrationDataInput.cpp
        Calibration/CalibrationData.h Calibration/CalibrationData.cpp
        Calibration/CalibrationDataStorage.h Calibration/CalibrationDataStorage.cpp
        Reader/DataHeader.h Reader/DataHeader.cpp Reader/DataReader.h
        Reader/DataReader.cpp Reader/FilesListItem.h Reader/FilesListItem.cpp
        Time/Time.h Time/Time.cpp
        Processing/OpenCLContext.cpp Processing/OpenCLContext.h
        Processing/MetricsCalculator.cpp Processing/MetricsCalculator.h Processing/Processor.cpp Processing/Processor.h
        Metrics/MetricsType.h Metrics/MetricsType.cpp Processing/Compresser.cpp Processing/Compresser.h Metrics/MetricsContainer.cpp Metrics/MetricsContainer.h Config/Config.cpp Config/Config.h Logger/Logger.cpp Logger/Logger.h)
target_link_libraries(prao-compresser OpenCL::OpenCL)


SET (ResourcesIn ${CMAKE_CURRENT_SOURCE_DIR}/resources)
SET (ResourcesOut ${CMAKE_BINARY_DIR}/resources)

add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        ${ResourcesOut}/Kernels/)

# coping kernels to output
add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${ResourcesIn}/Kernels/heapSort.cl
        ${ResourcesIn}/Kernels/nth_element.cl
        ${ResourcesOut}/Kernels/)

# coping examples of input files to output
add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${ResourcesIn}/examples/
        ${ResourcesOut}/Examples/)

# coping graph builder to output
add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/plot/
        ${CMAKE_BINARY_DIR}/plot/)