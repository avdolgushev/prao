#include "Calibration/test.h"
#include "Reader/testReader.h"
#include "Compresser.h"



//
// Created by sorrow on 30.01.19.
//
int main(int argc, char **argv) {

    std::vector<std::string> args;
    if (argc != 3) {
        std::cout << "provide correct number of args!" << std::endl;
        exit(-1);
    }
    for (int i = 0; i < argc; ++i) {
        std::string arg(argv[i]);
        args.push_back(arg);
    }
    if (args[1] != "-config") {
        std::cout << "provide --c or -config argument" << std::endl;
        std::cout << args[1] << std::endl;
        exit(-1);
    }

    OpenCLContext context = OpenCLContext();
    context.initContext();
    Compresser compresser = Compresser(argv[2], context);
    clock_t tStart = clock();
    compresser.run();
    std::cout << "elapsed time: " << (float) (clock() - tStart) / CLOCKS_PER_SEC << "s" << std::endl;
    return 0;
}

