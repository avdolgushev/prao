//
// Created by sorrow on 10.03.19.
//

#ifndef PRAO_COMPRESSER_COMPRESSER_H
#define PRAO_COMPRESSER_COMPRESSER_H


#include <string>
#include <vector>
#include <fstream>
#include <cmath>
#include <utility>


#include "Processing/OpenCLContext.h"
#include "Processing/MetricsCalculator.h"
#include "Calibration/CalibrationDataStorage.h"
#include "Metrics/MetricsContainer.h"
#include "Reader/FilesListItem.h"
#include "Config/Config.h"

#include "Logger/Logger.h"

class Compresser {
private:
    /* это приходит в конфиге */
    std::string fileListPath;
    std::string calibrationListPath;
    size_t localWorkSize{};
    double starSeconds{};
    float leftPercentile{};
    float rightPercentile{};
    std::string outputPath;

private:

    /* это приходит в конструкторе */
    /**
     * Хостовая часть
     */
    OpenCLContext context{};


public:
    Compresser() = default;
    Compresser(char* configFile, OpenCLContext context);


    /**
     * Основной метод.
     * Запускает калибровку, считывание, сжатие, запись данных
     * По всем файлам из fileListPath
     */
    void run();

    /**
     * Служебный метод
     * Получает CalibrationDataStorage
     * @param path_calibration - файл с путями до файлов калибровки
     * @return CalibrationDataStorage
     */
    CalibrationDataStorage *readCalibrationDataStorage(std::string path_calibration);
};


#endif //PRAO_COMPRESSER_COMPRESSER_H
