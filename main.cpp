#include "Calibration/test.h"
#include "Reader/testReader.h"
#include "Compresser.h"

#include "Config/Config.h"
using namespace std;

//
// Created by sorrow on 30.01.19.
//
int main(int argc, char **argv) {
    char * path_config;
    std::vector<std::string> args;
    if (argc == 2 && string(argv[1]) == "-h") {
        cout << "To provide a configuration file use -c %path_to_config." << endl;
        cout << "If configuration file was not passed, it will use the default path \"config.json\"." << endl;
        cout << "To see help message use -h." << endl;
        exit(0);
    }
    if (argc == 3 && string(argv[1]) == "-c")
        path_config = argv[2];
    else if (argc == 1)
        path_config = "config.json";
    else {
        std::cout << "Not valid arguments. Use -h to get help." << std::endl;
        exit(-1);
    }
    if (Configuration.readFrom(path_config))
        return 1;

    OpenCLContext context = OpenCLContext();
    context.initContext();
    Compresser compresser = Compresser(context);
    clock_t tStart = clock();
    compresser.run();
    std::cout << "elapsed time: " << (float) (clock() - tStart) / CLOCKS_PER_SEC << "s" << std::endl;
    return 0;
}

