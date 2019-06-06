//
// Created by work on 09.03.2019.
//

#ifndef PRAO_COMPRESSER_METRICSTYPE_H
#define PRAO_COMPRESSER_METRICSTYPE_H

#include <iostream>
#include <cmath>
#include <algorithm>

using namespace std;

struct metrics {
    static const int metric_count = 7;
    float min; 					// minimum of all
    float max; 					// maximum of all
    float max_ind; 				// index of maximum of all
    float average; 				// average of all
    float median; 				// median of all
    float variance; 			// variance of all
    float variance_bounded; 	// variance of bounded
    //float left_bound; 			// the left bound value based on which the bounded var is calculated
    //float right_bound; 			// the right bound value based on which the bounded var is calculated


    /// \brief calculates the need metrics over CPU and compares with proveded (accuracy is 0.1)
    /// \param input array of arrays of data
    /// \param calculated array of calculated metrics of arrays of date
    /// \param count_arrays count of arrays
    /// \param array_size size of each array
    /// \param left_percentile ratio that describes how much will be taken of the lowest
    /// \param right_percentile ratio that describes how much will be taken of the biggest
    /// \param comparing_accuracy when comparing two float metrics calculated by different ways
    static void check_natively_over_CPU(float * input, metrics * calculated, int count_arrays, int array_size, float left_percentile, float right_percentile, float comparing_accuracy = 0.1f);
};


#endif //PRAO_COMPRESSER_METRICSTYPE_H
