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
    int count_read_points;
    metrics *metrics_;
    //metrics_with_time(double MJD_time, double starTime, metrics *metrics) : MJD_time_(MJD_time), starTime_(starTime), metrics_(metrics) {}
};

struct storageEntry{
    FilesListItem * filesListItem = nullptr;
    vector<metrics_with_time> storage = vector<metrics_with_time>();

    void addNewMetrics(double MJD_time, double starTime, int count_read_points, metrics *metrics_) {
        storage.emplace_back(metrics_with_time { MJD_time, starTime, count_read_points, metrics_ });
    }
};

class MetricsContainer {
    int dim_size = -1;

    vector<storageEntry> storage = vector<storageEntry>();

    float * prepare_buffer_time_ray_band_metric(storageEntry *entry, vector<metrics *> &found_metrics,
                                                int *out_array_size, metrics_with_time &found_start);
    float * prepare_metric_unk(storageEntry *entry, vector<metrics *> &found_metrics, int *out_array_size,
                               metrics_with_time &found_start);
    void write_header(string file_path, storageEntry * entry, vector<metrics *> &found_metrics, metrics_with_time &found_start);
    void saveFound(storageEntry * entry, vector<metrics *> &found_metrics, metrics_with_time &found_start);
    void delete_by_iters(vector<pair<vector<metrics_with_time>*, vector<metrics_with_time>::iterator> > &iterators_to_erase);
    void fix_max_ind(vector<metrics *> &found_metrics, vector<int> &found_metrics_counts);
public:
    ~MetricsContainer();
    storageEntry * addNewFilesListItem(FilesListItem *filesListItem);
    void flush(bool save_last_not_full = false);
};


#endif //PRAO_COMPRESSER_METRICSCONTAINER_H
