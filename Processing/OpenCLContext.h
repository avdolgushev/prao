//
// Created by sorrow on 18.02.19.
//
/* this is opencl sandbox */
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else

#include <CL/cl.h>

#endif

#ifndef PRAO_COMPRESSER_GPUCONTEXT_H
#define PRAO_COMPRESSER_GPUCONTEXT_H

#define MAX_SOURCE_SIZE (0x100000)


class OpenCLContext {
public:
    OpenCLContext() = default;

    OpenCLContext(int deviceType, int algorithmType);

    /**
     * Оператор присваивания. просто копирует все поля
     * @param oclContext
     * @return
     */
    OpenCLContext &operator=(const OpenCLContext &oclContext);


private: /* gpu properties */
    cl_platform_id platform_id;
    cl_uint ret_num_platforms;
    cl_uint ret_num_devices;
    cl_context context;
    cl_command_queue command_queue;
    cl_device_id device;

private: /* kernels */

    cl_kernel workingKernel;
    /**
     * CPU или GPU
     * GPU = 0, CPU = 1
     */
    int deviceType;

    /**
     * Используемый алгоритм.
     * nth_element = 0, heapSort = 1
     */
    int algorithm;

private:
    /**
 * Собирает и компилирует ядро
 * @param filename - путь к ядру. можно и относительный.
 * @param kernelName - само название ядра. хранится внутри файла *.cl
 * @return скомпилированный и готовый к работе кернел
 */
    cl_kernel compile_kernel(const char filename[], const char kernelName[]);

public:
    /**
     * сортирует ядра подсчета метрик
     */
    void initMetricsKernels();

    /** инициализирует контекст и все остальное */
    void initContext();

public: /* getters */

    /* context */
    cl_context &getContext() { return context; }

    const cl_context &getContext() const { return context; }

    /* command_queue */
    cl_command_queue &getClCommandQueue() { return command_queue; }

    const cl_command_queue &getClCommandQueue() const { return command_queue; }


    /* metrics kernel */
    cl_kernel &getWorkingKernel() { return workingKernel; }

    const cl_kernel &getWorkingKernel() const { return workingKernel; }

};


#endif //PRAO_COMPRESSER_GPUCONTEXT_H