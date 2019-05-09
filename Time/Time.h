//
// Created by work on 10.02.2019.
//

#ifndef PRAO_COMPRESSER_TIME_H
#define PRAO_COMPRESSER_TIME_H


#include <ctime>
#include <string>

using namespace std;

/// subs from tm 1900 years and makes mon in range [0-11]
int tm_SubDefault(tm &time);

/// adds to tm 1900 years and makes mon in range [1-12]
int tm_AddDefault(tm &time);


/// subs from tm 4 hours and makes it in default view
int tm_toUTC(tm &time, int seconds = 60 * 60 * 4);

double to_MJD(tm &time);

double to_starTime(double sunSeconds);

double to_SunTime(double starSeconds);


string myToString(int value, int countSymbols = 2);

string getCurrentDateTimeStr();

#endif //PRAO_COMPRESSER_TIME_H
