cmake_minimum_required(VERSION 3.10)
project(prao-compresser)
set(CMAKE_CXX_STANDARD 17)
find_package(OpenCL REQUIRED)
add_executable(prao-compresser main.cpp
        Calibration/CalibrationDataInput.h Calibration/CalibrationDataInput.cpp
        Calibration/CalibrationData.h Calibration/CalibrationData.cpp
        Calibration/CalibrationDataStorage.h Calibration/CalibrationDataStorage.cpp Calibration/test.cpp Calibration/test.h
        Reader/DataHeader.h Reader/DataHeader.cpp Reader/testReader.h Reader/testReader.cpp Reader/DataReader.h
        Reader/DataReader.cpp Reader/FilesListItem.h Reader/FilesListItem.cpp
        Time/Time.h Time/Time.cpp
        Processing/OpenCLContext.cpp Processing/OpenCLContext.h
        Processing/MetricsCalculator.cpp Processing/MetricsCalculator.h Processing/Processor.cpp Processing/Processor.h
        Metrics/MetricsType.h Metrics/MetricsType.cpp Compresser.cpp Compresser.h Metrics/MetricsContainer.cpp Metrics/MetricsContainer.h Config/Config.cpp Config/Config.h Logger/Logger.cpp Logger/Logger.h)
target_link_libraries(prao-compresser OpenCL::OpenCL)