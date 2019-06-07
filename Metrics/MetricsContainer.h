//
// Created by work on 10.03.2019.
//

#ifndef PRAO_COMPRESSER_METRICSCONTAINER_H
#define PRAO_COMPRESSER_METRICSCONTAINER_H

#include <string>
#include <vector>
#include <fstream>
#include <iomanip>

#include "MetricsType.h"
#include "../Config/Config.h"
#include "../Reader/DataReader.h"
#include "../Reader/FilesListItem.h"

#define EPS 1e-9

struct metrics_with_time {
    double MJD_time_;
    double starTime_;
    metrics *metrics_;
    //metrics_with_time(double MJD_time, double starTime, metrics *metrics) : MJD_time_(MJD_time), starTime_(starTime), metrics_(metrics) {}
};

struct storageEntry{
    FilesListItem * filesListItem = nullptr;
    vector<metrics_with_time> storage = vector<metrics_with_time>();

    void addNewMetrics(double MJD_time, double starTime, metrics *metrics_) {
        storage.emplace_back(metrics_with_time { MJD_time, starTime, metrics_ });
    }
};

class MetricsContainer {
    vector<storageEntry> storage = vector<storageEntry>();

    float * prepare_buffer(storageEntry * entry, vector<metrics *> &found_metrics, int *out_array_size);
    void write_header(string file_path, storageEntry * entry, vector<metrics *> &found_metrics, double star_start, double MJD_start);

public:
    ~MetricsContainer();
    storageEntry * addNewFilesListItem(FilesListItem *filesListItem);
    void flush();
    void saveFound(storageEntry * entry, vector<metrics *> &found_metrics, double star_start, double MJD_start);
};


#endif //PRAO_COMPRESSER_METRICSCONTAINER_H
