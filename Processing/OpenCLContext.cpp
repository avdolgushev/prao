//
// Created by sorrow on 18.02.19.
//

#include "OpenCLContext.h"

cl_kernel OpenCLContext::compile_kernel(const char filename[], const char kernelName[]) {
    cl_program program = nullptr;
    cl_kernel kernel = nullptr;

    int ret = -1;
    FILE *fp;
    size_t source_size;
    char *source_str;

    /* считывание файла с кернелом */
    try {
        fp = fopen(filename, "r");
        if (!fp) {
            fprintf(stderr, "Failed to load kernel.\n");
            exit(1);
        }
        source_str = (char *) malloc(MAX_SOURCE_SIZE);
        source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
        fclose(fp);
    }
    catch (int a) {
        printf("%i", a);
    }


    /* создать бинарник из кода программы */
    program = clCreateProgramWithSource(context, 1, (const char **) &source_str, (const size_t *) &source_size, &ret);
    if (ret != 0) {
        std::cout << "error creating binary file. ret: " << ret << std::endl;
        exit(-1);
    }

    /* скомпилировать программу */
    ret = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
    if (ret == -11) {
        // Determine the size of the log
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);

        // Allocate memory for the log
        auto log = (char *) malloc(log_size);

        // Get the log
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, nullptr);

        // Print the log
        printf("%s\n", log);
        exit(-1);
    } else if (ret != 0 && ret != -11) {
        std::cout << "kernel compiling error. ret: " << ret << std::endl;
        exit(-1);
    }

    /* создать кернел */
    kernel = clCreateKernel(program, kernelName, &ret);
    if (ret != 0) {
        std::cout << "error creating kernel. ret: " << ret << std::endl;
        exit(-1);
    }

    return kernel;
}


OpenCLContext &OpenCLContext::operator=(const OpenCLContext &oclContext) {
    if (this == &oclContext)
        return *this;

    context = oclContext.context;
    command_queue = oclContext.command_queue;
    device = oclContext.device;
    workingKernel = oclContext.workingKernel;

    return *this;
}

void OpenCLContext::initContext() {
    //std::cout << "initializating context..." << std::endl;
    int ret = -1;
    scanDevices();
    /* создание контекста */
    context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &ret);
    if (ret != 0) {
        std::cout << "fail creating context. ret: " << ret <<
                  std::endl;
        exit(-1);
    }

    /* создание command queue (пока не совсем понятно что это */
    command_queue = clCreateCommandQueue(context, device, 0, &ret);
    if (ret != 0) {
        std::cout << "fail creating command_queue. ret: " << ret <<
                  std::endl;
        exit(-1);
    }

    workingKernel = compile_kernel(Configuration.kernelPath.c_str(), "getMetrics");
}


void OpenCLContext::scanDevices() {
    cl_int ret = -1;
    cl_uint ret_num_platforms;
    ret = clGetPlatformIDs(0, nullptr, &ret_num_platforms);
    auto *platforms = new cl_platform_id[ret_num_platforms];
    ret = clGetPlatformIDs(ret_num_platforms, platforms, nullptr);
    int globalDeviceCount = 0;
    std::map<int, cl_device_id> deviceIdMap;
    std::map<int, char*> deviceNameMap;
    std::cout << "choose device number" << std::endl;
    for (int i = 0; i < ret_num_platforms; ++i) {
        for (int j = 1; j < 3; j++) {
            cl_platform_id platform_id = platforms[i];

            cl_uint ret_num_devices;
            ret = clGetDeviceIDs(platform_id, 1 << j, 0, nullptr, &ret_num_devices);
            if (ret == CL_DEVICE_NOT_FOUND)
                continue;

            cl_device_id devices[ret_num_devices];
            ret = clGetDeviceIDs(platform_id, 1 << j, ret_num_devices, devices, nullptr);
            if (ret != 0) {
                std::cout << "failed getting devices" << std::endl;
                exit(-1);
            }
            size_t size;
            for (int z = 0; z < ret_num_devices; z++) {
                globalDeviceCount++;
                deviceIdMap[globalDeviceCount] = devices[z];
                clGetDeviceInfo(devices[z], CL_DEVICE_NAME, 0, nullptr, &size);
                char *vendor = (char *) malloc(sizeof(char) * size);
                clGetDeviceInfo(devices[z], CL_DEVICE_NAME, size, vendor, nullptr);
                std::cout << globalDeviceCount << ") " << vendor << std::endl;
                deviceNameMap[globalDeviceCount] = vendor;
            }
        }
    }
    int chosenDeviceNumber;
    std::cin >> chosenDeviceNumber;
    device = deviceIdMap[chosenDeviceNumber];
    LOGGER("<< Selected device: %s", deviceNameMap[chosenDeviceNumber]);
}


