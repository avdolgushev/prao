//
// Created by sorrow on 18.02.19.
//
/* this is opencl sandbox */

#include <memory>
#include <iostream>
#include <string>
#include <fstream>
#include <memory>
#include <map>

#include "../Config/Config.h"

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


    /**
     * Оператор присваивания. просто копирует все поля
     * @param oclContext
     * @return
     */
    OpenCLContext &operator=(const OpenCLContext &oclContext);


private: /* gpu properties */
    cl_context context;
    cl_command_queue command_queue;
    cl_device_id device;

private: /* kernels */

    cl_kernel workingKernel;

private:
    /**
 * Собирает и компилирует ядро
 * @param filename - путь к ядру. можно и относительный.
 * @param kernelName - само название ядра. хранится внутри файла *.cl
 * @return скомпилированный и готовый к работе кернел
 */
    cl_kernel compile_kernel(const char filename[], const char kernelName[]);

    /**
     * Сканирует все доступные девайсы и получает от пользователя ответ, какой девайс использовать
     */
    void scanDevices();

public:

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
