#ifndef ANZEIGE_H
#define ANZEIGE_H

#include <stdint.h>
#include <cairo/cairo.h>
#include <string>

namespace anzeige {
    std::string console;
    char *programm;
    void loop();
    void readThread();

    unsigned int lineCount =0;
    unsigned int packetCount=0;

    bool running = true;
    bool displayOn = true;
    void turnDisplayOn();
    void turnDisplayOff();

    int comPipe[2];
    int stdio;
    uint pid;
    void runGNURadio();

    time_t last_action;
    bool firstStart = true;

    void writeString(cairo_t *cr, int posX, int posY, std::string string);

    bool errorOccured= false;
    std::string errorOverlayText;
    bool showErrorOverlay = false;
    void setErrorSpace(cairo_t* cr, bool error);

}

#endif
