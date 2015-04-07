#ifndef ANZEIGE_H
#define ANZEIGE_H

#include <stdint.h>
#include <cairo/cairo.h>

namespace anzeige {
    std::string console;
    void loop();
    void readThread();
    int lineCount =0;
    bool errorOccured= false;
    bool running = true;
    void runGNURadio();
    int comPipe[2];
    int stdio;
    uint pid;
    void setErrorSpace(cairo_t* cr, bool error);

}

#endif
