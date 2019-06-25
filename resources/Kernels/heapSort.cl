/*
good working heapsort. Local size - 8-16: 12
const size_t size_chunk_floats = 800;
const size_t global_workgroup = 33 * 48 * 4;
const size_t buffer_size = sizeof(cl_float) * global_workgroup * size_chunk_floats;
const size_t local_workgroup = 12;
*/

inline void swap(__global float *a, __global float *b) {
	float tmp;
	tmp = *b;
	*b = *a;
	*a = tmp;
}

void siftDown(int i, int heap_size, __global float* curr){
    while (2 * i + 1 < heap_size) { 
        int left = 2 * i + 1;            
        int right = 2 * i + 2;    
        if (right < heap_size && curr[right] < curr[left])
            left = right;
        if (curr[i] <= curr[left])
            break;
        swap(&curr[i], &curr[left]);
        i = left;
    }
}


struct metrics {
    float min; 					// minimum of all
    float max; 					// maximum of all
    float max_ind; 				// index of maximum of all
    float average; 				// average of all
    float median; 				// median of all
    float variance; 			// variance of all
    float variance_bounded; 	// variance of bounded
    //float left_bound; 			// the left bound value based on which the bounded var is calculated
    //float right_bound; 			// the right bound value based on which the bounded var is calculated
};


__kernel void getMetrics(__global float* inp, __global struct metrics * output, const int array_size, const float left_percentile, const float right_percentile){
    int heap_size = array_size;
	const int right = round(array_size * left_percentile);
	const int left = round(array_size * right_percentile);
	
	
	__global float* curr_in = &inp[get_global_id(0) * array_size];
	__global struct metrics * curr_out = &output[get_global_id(0)];
	
	
	float max = -100;
	int max_i;
	for (int i = 0; i < array_size; ++i){
		float curr_val = curr_in[i];
		if (curr_val > max){
			max = curr_in[i];
			max_i = i;
		}
	}
	
	
    for (int i = heap_size / 2; i >= 0; --i)
        siftDown(i, heap_size, curr_in);
    
    for (int i = 0; i < array_size - 1; ++i){
        swap(&curr_in[0], &curr_in[heap_size - 1]);
        --heap_size;
        siftDown(0, heap_size, curr_in);
    }
	
	

	float right_bound = curr_in[array_size - left]; 
	float left_bound = curr_in[array_size - right];
	float med_value = curr_in[array_size / 2];
	
	
	double av = 0;
	double av_sqt = 0;
	double av_bounded = 0;
	double av_sqt_bounded = 0;
	
	int count = 0;
	for (int i = 0; i < array_size; ++i){
		float curr_val = curr_in[i];
		
		if (curr_val > left_bound && curr_val <= right_bound){
			av_bounded += curr_val;
			av_sqt_bounded += curr_val * curr_val;
			++count;
		}
		
		av += curr_val;
		av_sqt += curr_val * curr_val;
	}
	
	av /= array_size;
	av_sqt /= array_size;
	av_bounded /= count;
	av_sqt_bounded /= count;
	
	double D = sqrt(av_sqt - av * av);
	double D_bounded = sqrt(av_sqt_bounded - av_bounded * av_bounded);
	
	curr_out->min = (float)curr_in[array_size - 1]; 
	curr_out->max = (float)curr_in[0];
	curr_out->max_ind = (float)max_i;
	curr_out->average = (float)av;
	curr_out->median = med_value;
	curr_out->variance = (float)D;
	curr_out->variance_bounded = (float)D_bounded;
	//curr_out->left_bound = (float)left_bound;
	//curr_out->right_bound = (float)right_bound;
}
