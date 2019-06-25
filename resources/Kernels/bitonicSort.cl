
inline void compare(
	__local float *A, 
	__local float *B,
	bool dir)
{
	if ((*A > *B) != dir) {
	//#pragma HLS PIPELINE 
		float t;
		t = *A; *A = *B; *B = t;
	}
}

inline void BitonicSort1024(
	__local float * arr, 
	const unsigned int pairCount, 
	__private int localId)
{ 	
	#pragma unroll
	for (int globalGran = 2; globalGran <= 1024; globalGran <<= 1)
	{
		for (int localGran = globalGran; localGran >= 2; localGran >>= 1)
		{
			if (localId < pairCount)
			{
				int halfLocalGran = localGran >> 1;
				int arrId = localId % halfLocalGran + (localId / halfLocalGran) * localGran;
				int arrPairId = arrId + halfLocalGran;
				bool dir = ((arrId / globalGran) % 2) == 0; // true = asc, false = desc

				compare(&arr[arrId], &arr[arrPairId], dir);
			}
			barrier(CLK_LOCAL_MEM_FENCE);
		}
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
    float left_bound; 			// the left bound value based on which the bounded var is calculated
    float right_bound; 			// the right bound value based on which the bounded var is calculated
};

__kernel void getMetrics(__global float* inp, __global struct metrics * output, const int array_size, const float left_percentile, const float right_percentile, __local float *work_inp, __local struct metrics *work_metrics)
{
	__global float * curr_in = &inp[get_group_id(1) * 1024];
	__global struct metrics * curr_out = &output[get_global_id(1)];

	int localId = get_local_id(0); // size: 512

	if (localId < 512)
	{
		work_inp[localId + localId] = curr_in[localId + localId];
		work_inp[localId + localId + 1] = curr_in[localId + localId + 1];
	}

	barrier(CLK_LOCAL_MEM_FENCE); // wait for the array to be copied to the local memory

	BitonicSort1024(work_inp, 512, localId);
	
	barrier(CLK_LOCAL_MEM_FENCE); // wait for the array to be copied to the local memory
	
	if (localId < 512)
	{
		curr_in[localId + localId] = work_inp[localId + localId];
		curr_in[localId + localId + 1] = work_inp[localId + localId + 1];
	}
	//barrier(CLK_LOCAL_MEM_FENCE); // wait for the array to be copied to the local memory
	
	if (localId == 0) {
		curr_out->min = work_inp[array_size - 1];
		curr_out->max = work_inp[0];
	}
}