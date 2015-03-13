#ifndef ANZEIGE_H
#define ANZEIGE_H

#include <stdint.h>

namespace anzeige {
    std::string console;
    void loop();
    void readThread();
    int lineCount =0;
}

#endif
