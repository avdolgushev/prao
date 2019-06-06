//
// Created by work on 09.03.2019.
//

#include "MetricsType.h"



int partition(float *arr_, int L, int R){
    swap(arr_[(L + R) / 2], arr_[L]);
    float piv = arr_[L];
    while (true) {
        while (arr_[R] < piv) // && L < R
            --R;
        if (L < R)
            arr_[L++] = arr_[R];
        while (arr_[L] > piv && L < R)
            L++;
        if (L < R)
            arr_[R--] = arr_[L];
        else
            break;

    }
    arr_[L] = piv;
    return L;
}


float findOrderStatistic(float * array, int n, int k) {
    int left = 0, right = n - 1;
    while (true) {
        int mid = partition(array, left, right);

        if (mid == k)
            return array[mid];
        else if (k < mid)
            right = mid - 1;
        else
            left = mid + 1;

    }
}

void metrics::check_natively_over_CPU(float * input, metrics * calculated, int count_arrays, int array_size, float left_percentile, float right_percentile, float comparing_accuracy) {
    for(int i = 0; i < count_arrays; ++i, ++calculated){
        //right_percentile += 0.000001f;
        //left_percentile += 0.000001f;
        float * input_curr = input + i * array_size;



        float mn = *min_element(input_curr, input_curr + array_size);
        float mx = *max_element(input_curr, input_curr + array_size);
        int mx_i = (max_element(input_curr, input_curr + array_size) - input_curr);

        float med = findOrderStatistic(input_curr, array_size, array_size / 2);
        nth_element(input_curr, input_curr + array_size / 2, input_curr, greater<float>());

        if (med != input_curr[array_size / 2])
            cout << "my_nth != std_nth" << endl;

        double D = 0, av1 = 0, D2 = 0, av2 = 0;

        int left = round(array_size * right_percentile), right = round(array_size * left_percentile);
        float right_bound = findOrderStatistic(input_curr, array_size, array_size - left);
        float left_bound = findOrderStatistic(input_curr, array_size, array_size - right);

        //sort(input_curr, input_curr + array_size, greater<float>());

        int count = 0;
        for (int j = 0; j < array_size; ++j){
            av1 += input_curr[j];
            if (input_curr[j] > left_bound && input_curr[j] <= right_bound) {
                ++count;
                av2 += input_curr[j];
            }
        }

        if (count == 0)
            cout << "count == 0 err" << endl;
        if (abs((right_percentile - left_percentile + 0.00001) * array_size - count) > 5)
            cout << "count < 540 || count > 550 err" << endl;

        av1 /= array_size;
        av2 /= count;
        for (int j = 0; j < array_size; ++j){
            D += (input_curr[j] - av1) * (input_curr[j] - av1);
            if (input_curr[j] > left_bound && input_curr[j] <= right_bound)
                D2 += (input_curr[j] - av2) * (input_curr[j] - av2);
        }
        D /= array_size;
        D2 /= count;

        if (mn != calculated->min || mn == 0)
            cout << "mn " << i << endl;
        if (mx != calculated->max || mx == 0)
            cout << "mx " << i << endl;
        if (mx_i != calculated->max_ind)
            cout << "mx_i " << i << endl;
        if (fabs(av1 - calculated->average) > comparing_accuracy)
            cout << "average " << i << endl;
        if (med != calculated->median || med == 0)
            cout << "med " << i << endl;
        if (fabs(D - calculated->variance) > comparing_accuracy)
            cout << "D " << i << endl;
        if (fabs(D2 - calculated->variance_bounded) > comparing_accuracy)
            cout << "D2 " << i << endl;
//        if (left_bound != calculated->left_bound || left_bound == 0)
//            cout << "left_bound " << i << endl;
//        if (right_bound != calculated->right_bound || right_bound == 0)
//            cout << "right_bound " << i << endl;
    }
}