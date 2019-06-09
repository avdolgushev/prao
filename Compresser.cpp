//
// Created by sorrow on 10.03.19.
//

#include "Compresser.h"


void Compresser::run() {
    /* get calibration files */
    CalibrationDataStorage *storage = readCalibrationDataStorage(Configuration.calibrationListPath);
    ifstream in(Configuration.fileListPath);

    FilesListItem *item = new FilesListItem(), *item_next = new FilesListItem();
    DataReader *reader, *readerNext;

    in >> *item;
    reader = item->getDataReader(Configuration.starSecondsZip);
    reader->setCalibrationData(storage);
    reader->AlignByStarTimeChunk();
    auto *data_reordered_buffer = new float[reader->getNeedBufferSize()];

    int remains = 0, offset = 0, count;

    double curr_starTime_seconds, curr_MJD;
    int count_read_points;

    MetricsContainer container;
    storageEntry *metrics_storage;
    while(reader != nullptr) {
        in >> *item_next;

        if (item_next->good()) {
            readerNext = item_next->getDataReader(Configuration.starSecondsZip);
            readerNext->setCalibrationData(storage);
            reader->set_MJD_next(readerNext->get_MJD_begin());
        } else {
            readerNext = nullptr;
            delete item_next;
        }


        int start, time_processing_curr = 0;
        if (remains){
            count = offset + reader->readNextPoints(data_reordered_buffer, remains, offset);
            MetricsCalculator calculator = MetricsCalculator(context, data_reordered_buffer, reader->getPointSize(), count, Configuration.localWorkSize, Configuration.leftPercentile, Configuration.rightPercentile);
            metrics_storage->addNewMetrics(curr_MJD, curr_starTime_seconds, count_read_points, calculator.calc());
            remains = 0;
        }

        metrics_storage = container.addNewFilesListItem(item);

        while (!reader->eof()) {
            curr_starTime_seconds = reader->getCurrStarTimeSecondsAligned();
            curr_MJD = reader->get_MJD_current();
            count_read_points = reader->get_count_read_points();
            count = reader->readNextPoints(data_reordered_buffer);
            MetricsCalculator calculator = MetricsCalculator(context, data_reordered_buffer, reader->getPointSize(), count, Configuration.localWorkSize, Configuration.leftPercentile, Configuration.rightPercentile);
            start = clock();
            metrics_storage->addNewMetrics(curr_MJD, curr_starTime_seconds, count_read_points, calculator.calc());
            time_processing_curr += clock() - start;
        }

        curr_starTime_seconds = reader->getCurrStarTimeSecondsAligned();
        curr_MJD = reader->get_MJD_current();
        count_read_points = reader->get_count_read_points();
        offset = reader->readRemainder(data_reordered_buffer, &remains);
        cout << "processing time: " << time_processing_curr / (float) CLOCKS_PER_SEC << endl;

        container.flush();


        reader = readerNext;
        item = item_next;
        item_next = new FilesListItem();
    }

    delete storage;
    in.close();
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


Compresser::Compresser(const OpenCLContext context) {
    this->context = context;
    this->context.initMetricsKernels(Configuration.algorithm);
}


