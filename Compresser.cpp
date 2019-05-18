#include <utility>

//
// Created by sorrow on 10.03.19.
//

#include "Compresser.h"



void Compresser::run() {
    /* get calibration files */
    CalibrationDataStorage *storage = readCalibrationDataStorage(calibrationListPath);
    ifstream in(fileListPath);
    while (!in.eof()) {
        FilesListItem item;
        in >> item;
        DataReader *reader = item.getDataReader(starSeconds);
        auto *data_reordered_buffer = new float[reader->getNeedBufferSize()];
        reader->setCalibrationData(storage);

        MetricsContainer container(reader);

        int i = 1;
        clock_t tStart1, sum1 = 0;
        clock_t tStart2, sum2 = 0;
        try {
            while (!reader->eof()) {
                tStart2 = clock();
                int count = reader->readNextPoints(data_reordered_buffer);
                sum2 += clock() - tStart2;
                MetricsCalculator
                        calculator = MetricsCalculator(context, data_reordered_buffer, reader->getPointSize(), count,
                                                       localWorkSize,
                                                       leftPercentile,
                                                       rightPercentile);
                tStart1 = clock();
                auto *metrics_buffer = calculator.calc();
                container.addNewMetrics(metrics_buffer);
                sum1 += clock() - tStart1;

                if (i % 30 == 0)
                    std::cout << i << " arrays calculated..." << std::endl;
                ++i;
            }
            std::cout << "calculating work time: " << (float) (sum1) / (float) CLOCKS_PER_SEC << "s"
                      << std::endl;
            std::cout << "reading and calibration work time: " << (float) (sum2) / (float) CLOCKS_PER_SEC << "s"
                      << std::endl;
            std::cout << "reading work time: " << (float) (reader->time_reading) / (float) CLOCKS_PER_SEC << "s"
                      << std::endl;
            std::cout << "calibrating work time: " << (float) (reader->time_calibrating) / (float) CLOCKS_PER_SEC << "s"
                      << std::endl;
            std::cout << "copying work time: " << (float) (reader->time_copying) / (float) CLOCKS_PER_SEC << "s"
                      << std::endl;

            container.saveToFile(outputPath + '\\' + item.filename + ".processed");
        }
        catch (logic_error e) {
            std::cout << e.what() << std::endl;
        }


    }
}

CalibrationDataStorage *Compresser::readCalibrationDataStorage(std::string path_calibration) {
    LOGGER(">> Creating storage of calibration signals from file %s", path_calibration.c_str());
    float start = 0, diff = 0;
    auto *storage = new CalibrationDataStorage();

    start = clock();
    storage->add_items_from_file(path_calibration);
    diff = (clock() - start) / CLOCKS_PER_SEC;
    cout << "reading calibration file took " << diff << " sec" << endl;
    LOGGER("<< Created storage of calibration signals from file %s took %f seconds", path_calibration.c_str(), diff);
    return storage;
}


Compresser::Compresser(char *configFile, const OpenCLContext context) {
    this->context = context;

    Config cfg = Config(configFile);
    this->outputPath = cfg.getOutputPath();
    this->leftPercentile = cfg.getLeftPercentile();
    this->rightPercentile = cfg.getRightPercentile();
    this->starSeconds = cfg.getStarSeconds();
    this->localWorkSize = cfg.getLocalWorkSize();
    this->fileListPath = cfg.getFileListPath();
    this->calibrationListPath = cfg.getCalibrationListPath();
    int algorithm = cfg.getAlgorithm();
    this->context.initMetricsKernels(algorithm);
}


