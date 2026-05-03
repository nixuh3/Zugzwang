#include "pch.h"
#include "uci.h"

int main(int argc, char** argv) {
    Zugzwang::UCIEngine uci(argc, argv);
    uci.Loop();

    return 0;
}
