//
// Created by sorrow on 10.03.19.
//

#include "Compresser.h"


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

    int remains = 0, offset = 0, count;
    double curr_starTime_seconds;
    MetricsContainer container;
    storageEntry *metrics_storage;
    while(reader != nullptr) {
        in >> *item_next;

        if (item_next->good()) {
            readerNext = item_next->getDataReader(starSeconds);
            readerNext->setCalibrationData(storage);
            reader->set_MJD_next(readerNext->get_MJD_begin());
        } else
            readerNext = nullptr;


        int start, time_processing_curr = 0;
        if (remains){
            count = offset + reader->readNextPoints(data_reordered_buffer, remains, offset);
            MetricsCalculator calculator = MetricsCalculator(context, data_reordered_buffer, reader->getPointSize(), count, localWorkSize, leftPercentile, rightPercentile);
            metrics_storage->addNewMetrics(curr_starTime_seconds, calculator.calc());
            remains = 0;
        }

        metrics_storage = container.addNewFilesListItem(item);

        while (!reader->eof()) {
            curr_starTime_seconds = reader->getCurrStarTimeSecondsAligned();
            count = reader->readNextPoints(data_reordered_buffer);
            MetricsCalculator calculator = MetricsCalculator(context, data_reordered_buffer, reader->getPointSize(), count, localWorkSize, leftPercentile, rightPercentile);
            start = clock();
            metrics_storage->addNewMetrics(curr_starTime_seconds, calculator.calc());
            time_processing_curr += clock() - start;
        }

        curr_starTime_seconds = reader->getCurrStarTimeSecondsAligned();
        offset = reader->readRemainder(data_reordered_buffer, &remains);
        cout << "processing time: " << time_processing_curr / (float) CLOCKS_PER_SEC << endl;

        container.flush();


        reader = readerNext;
        item = item_next;
        item_next = new FilesListItem();
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


