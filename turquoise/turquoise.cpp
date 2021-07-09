#include <engine.h>

int main(int argc, char *argv[]) {
#ifndef __APPLE__
    std::setlocale(LC_ALL, "");
#else
    wcout << "Apple" << endl;
    setlocale(LC_ALL, "");
    std::wcout.imbue(std::locale("en_US.UTF-8"));
#endif
    bool doPerf = false;
    if (argc > 1) {
        if (!strcmp(argv[1], "-p"))
            doPerf = true;
    }

    miniGame();
    return 0;
}
