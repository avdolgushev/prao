//
// Created by sorrow on 10.03.19.
//

#include <fstream>
#include "Compresser.h"
#include "Calibration/CalibrationDataStorage.h"
#include "Reader/FilesListItem.h"
#include "Metrics/MetricsType.h"
#include "Processing/MetricsCalculator.h"

Compresser::Compresser(char *fileListPath, char *calibrationListPath, OpenCLContext context) {
    this->context = context;
    this->fileListPath = fileListPath;
    this->calibrationListPath = calibrationListPath;
}


void Compresser::run() {
    /* get calibration files */
    CalibrationDataStorage *storage = readCalibrationDataStorage(calibrationListPath);
    ifstream in(fileListPath);
    while (!in.eof()) {
        FilesListItem item;
        in >> item;
        DataReader *reader = item.getDataReader(10);
        auto *data_reordered_buffer = new float[reader->getNeedBufferSize()];
        reader->setCalibrationData(storage);
        int i = 1;
        try {
            while (!reader->eof()) {
                int count = reader->readNextPoints(data_reordered_buffer);
                MetricsCalculator
                        calculator = MetricsCalculator(context, data_reordered_buffer, reader->getPointSize(), count,
                                                       0.02f,
                                                       0.7f);
                auto *metrics_buffer = calculator.calc();

                std::cout << i << ": smth calculated..." << std::endl;
                /* пока так. но вообще то это надо записывать. */
                delete[] metrics_buffer;
                ++i;
            }
        }
        catch (logic_error e) {

        }

    }
}

CalibrationDataStorage *Compresser::readCalibrationDataStorage(char *path_calibration) {
    float start = 0, diff = 0;
    auto *storage = new CalibrationDataStorage();

    start = clock();
    std::string path(path_calibration);
    storage->add_items_from_file(path);
    diff = (clock() - start) / CLOCKS_PER_SEC;
    cout << "reading calibration file took " << diff << " sec" << endl;
    return storage;
}