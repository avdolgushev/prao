#include <utility>

//
// Created by sorrow on 10.03.19.
//

#include "Compresser.h"

#include <iomanip>

void Compresser::run() {
    /* get calibration files */
    CalibrationDataStorage *storage = readCalibrationDataStorage(calibrationListPath);
    ifstream in(fileListPath);

    FilesListItem *item = new FilesListItem(), *item_next = new FilesListItem();
    DataReader *reader, *readerNext;

    in >> *item;
    reader = item->getDataReader(starSeconds);
    reader->setCalibrationData(storage);
    reader->AlignByStarTimeChunk();
    auto *data_reordered_buffer = new float[reader->getNeedBufferSize()];

    int remains = 0, offset = 0;
    double curr_starTime_seconds;
    MetricsContainer container;
    storageEntry *metrics_storage;
    while(reader != nullptr) {
        //std::cout << std::setprecision(10) << reader->getCurrStarTimeSecondsAligned() << std::endl;
        in >> *item_next;

        if (item_next->good()) {
            readerNext = item_next->getDataReader(starSeconds);
            readerNext->setCalibrationData(storage);
            reader->set_MJD_next(readerNext->get_MJD_begin());
        } else
            readerNext = nullptr;


        int count;
        if (remains){
            std::cout << std::setprecision(10) << curr_starTime_seconds << std::endl;
            count = offset + reader->readNextPoints(data_reordered_buffer, remains, offset);
            MetricsCalculator calculator = MetricsCalculator(context, data_reordered_buffer, reader->getPointSize(), count, localWorkSize, leftPercentile, rightPercentile);
            auto *metrics_buffer = calculator.calc();
            metrics_storage->addNewMetrics(curr_starTime_seconds, metrics_buffer);
            remains = 0;
        }
        metrics_storage = container.addNewFilesListItem(item);

        while (!reader->eof()) {
            curr_starTime_seconds = reader->getCurrStarTimeSecondsAligned();
            std::cout << std::setprecision(10) << curr_starTime_seconds << std::endl;
            count = reader->readNextPoints(data_reordered_buffer);
            MetricsCalculator calculator = MetricsCalculator(context, data_reordered_buffer, reader->getPointSize(), count, localWorkSize, leftPercentile, rightPercentile);
            auto *metrics_buffer = calculator.calc();
            metrics_storage->addNewMetrics(curr_starTime_seconds, metrics_buffer);
        }

        curr_starTime_seconds = reader->getCurrStarTimeSecondsAligned();
        offset = reader->readRemainder(data_reordered_buffer, &remains);

        container.flush();

        //std::cout << std::setprecision(10) << reader->getCurrStarTimeSecondsAligned() << std::endl;
        delete reader;
        reader = readerNext;
        item = item_next;
    }
}
/*
void Compresser::run1() {
    CalibrationDataStorage *storage = readCalibrationDataStorage(calibrationListPath);
    ifstream in(fileListPath);

    FilesListItem item;
    DataReader *reader, *readerNext;

    in >> item;
    reader = item.getDataReader(starSeconds);
    reader->setCalibrationData(storage);
    reader->AlignByStarTimeChunk();
    auto *data_reordered_buffer = new float[reader->getNeedBufferSize()];

    int remains = 0, offset = 0;
    double curr_starTime_seconds;
    while(reader != nullptr) {
        std::cout << std::setprecision(10) << reader->getCurrStarTimeSeconds() << std::endl;
        in >> item;

        if (item.good()) {
            readerNext = item.getDataReader(starSeconds);
            readerNext->setCalibrationData(storage);
            reader->set_MJD_next(readerNext->get_MJD_begin());
        } else
            readerNext = nullptr;

        MetricsContainer container(reader);

        int count;
        if (remains){
            std::cout << std::setprecision(10) << curr_starTime_seconds << std::endl;
            count = offset + reader->readNextPoints(data_reordered_buffer, remains, offset);
            MetricsCalculator calculator = MetricsCalculator(context, data_reordered_buffer, reader->getPointSize(), count, localWorkSize, leftPercentile, rightPercentile);
            auto *metrics_buffer = calculator.calc();
            container.addNewMetrics(metrics_buffer);
            remains = 0, offset = 0;
        }

        int i = 1;
        while (!reader->eof()) {
            curr_starTime_seconds = reader->getCurrStarTimeSeconds();
            std::cout << std::setprecision(10) << curr_starTime_seconds << std::endl;
            count = reader->readNextPoints(data_reordered_buffer);
            MetricsCalculator calculator = MetricsCalculator(context, data_reordered_buffer, reader->getPointSize(), count, localWorkSize, leftPercentile, rightPercentile);
            auto *metrics_buffer = calculator.calc();
            container.addNewMetrics(metrics_buffer);

//            if (i % 30 == 0)
//                std::cout << i << " arrays calculated..." << std::endl;
//            ++i;
        }

        curr_starTime_seconds = reader->getCurrStarTimeSeconds();
        offset = reader->readRemainder(data_reordered_buffer, &remains);
        // TODO
        container.saveToFile(outputPath + '\\' + item.filename + ".processed");
        std::cout << std::setprecision(10) << reader->getCurrStarTimeSeconds() << std::endl;
        delete reader;
        reader = readerNext;
    }
}

void Compresser::run3() {
    CalibrationDataStorage *storage = readCalibrationDataStorage(calibrationListPath);
    ifstream in(fileListPath);
    bool first = true;

    while (!in.eof()) {
        FilesListItem item;
        in >> item;
        DataReader *reader = item.getDataReader(starSeconds);
        reader->setCalibrationData(storage);
        if (first){
            reader->AlignByStarTimeChunk();
            first = false;
        }
        auto *data_reordered_buffer = new float[reader->getNeedBufferSize()];

        MetricsContainer container(reader);

        int i = 1;
        clock_t tStart1, sum1 = 0;
        clock_t tStart2, sum2 = 0;
        try {
            while (!reader->eof()) {
                tStart2 = clock();
                double curr_starTime_seconds = reader->getCurrStarTimeSeconds();
                int count = reader->readNextPoints(data_reordered_buffer);
                sum2 += clock() - tStart2;
                MetricsCalculator calculator = MetricsCalculator(context, data_reordered_buffer, reader->getPointSize(), count,
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


void Compresser::run2() {
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
*/
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


