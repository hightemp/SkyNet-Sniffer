
#include <sys/statvfs.h>
#include <iostream>
#include <stdint.h>
#include "ST7789.h"
#include "anzeige.h"
#include <stdlib.h>
#include "math.h"
#include <ctime>
#include "unistd.h"
#include <cairo/cairo.h>
#include <bcm2835.h>
#include "tft.h"
#include <pango/pangocairo.h>
#include <thread>
#include <sys/wait.h>
#include <signal.h>

#define DELAY 200000  //5 fps
#define PIN 21

int main(int argc, char ** argv) {

    if(argc <2) {
        std::cerr << "Usage: " << argv[0] << " [grc Flowgraph]"<<std::endl;
        return 2;
    }
    anzeige::programm = argv[1];

    if (!bcm2835_init())
        return 1;

    //Taster Initalisierung
    bcm2835_gpio_fsel(PIN, BCM2835_GPIO_FSEL_INPT);
    // with a pullup
    bcm2835_gpio_set_pud(PIN, BCM2835_GPIO_PUD_UP);
    bcm2835_gpio_afen(PIN); // Detect Fallen Edge

    // Display Initialisierung
    TFT_init_board();
    TFT_hard_reset();
    STcontroller_init();

    TFT_SetBacklightPWMValue( 100);

    if(pipe(anzeige::comPipe)) {
        anzeige::errorOccured = true;
        anzeige::console = "Pipe failed\n";
    }
    anzeige::stdio = dup(STDIN_FILENO);
    dup2(anzeige::comPipe[0],STDIN_FILENO);


    anzeige::runGNURadio();
    anzeige::firstStart = false;

    std::thread t1(anzeige::readThread);

    anzeige::last_action = std::time(0);
    anzeige::loop(); // returned erst wieder, wenn Programm sich beendet

    kill(anzeige::pid,9);
//    TFT_SetBacklightPWMValue( 0);
//    TFT_turnDisplayOff();

    bcm2835_gpio_clr_afen(PIN);     //Disable Falling Edge Detection
    bcm2835_gpio_set_eds(PIN);
    bcm2835_close();

    //t1.join();
    char* args[2];
    args[0] = "shutdown";
    args[1] = "now";
    execvp("shutdown",args);

    return 0;
}

namespace anzeige {

void runGNURadio() {

    pid = fork();
    if (pid == 0) {

        dup2(stdio,STDIN_FILENO);
        dup2(anzeige::comPipe[1],STDOUT_FILENO);
        dup2(anzeige::comPipe[1],STDERR_FILENO);
        if(firstStart) {
            time_t t = time(0) +60;   // get time now
            struct tm * now = localtime( & t );
            char buffer[9];
            std::strftime(buffer,9,"%H:%M:%S",now);

            std::cout << "Waiting 60 Seconds before starting GNURadio. The sniffer starts at "<< std::string(buffer)<< std::endl;
            sleep(60);
        }
        execvp(programm,NULL);
        std::cout << "Error running Skynet Sniffer"<<std::endl;
        exit(1);

    } else if(pid < 0) {
        anzeige::errorOccured = true;
        anzeige::console = "Fork failed\n";
    } else {

    }
}

void readThread() {
    while(running) {
        std::string input;

        std::getline(std::cin,input );
        if(input.length()>0) {
            console += "\n"+input;
            lineCount ++;
            if(lineCount>13) {
                console = console.substr(console.find("\n")+1);
                lineCount--;
            }
            if(input.find("error") != std::string::npos  || input.find("FATAL") != std::string::npos) {
                errorOccured = true;
                if(input.find("FATAL: No supported devices found to pick from.") != std::string::npos) {
                    errorOverlayText = "ERROR: Plug in RTL-SDR Stick and restart.";
                    showErrorOverlay = true;
                }
            } else if(input.find("cancel") != std::string::npos || input.find("vector") != std::string::npos || input.find("OOOO") != std::string::npos) {
                errorOccured = true;
                kill(anzeige::pid,9);

            } else if(input.find("Using Volk machine: neon_hardfp") != std::string::npos) {
                errorOccured = false;
            }
            else if(input.find("Packet received")!= std::string::npos)
                packetCount++;
        }
        //usleep(1000);
    }
}



void loop()
{

    while(running) {

        if (bcm2835_gpio_eds(PIN))
        {
            bcm2835_delay(1000);

            // Now clear the eds flag by setting it to 1
            uint8_t level = bcm2835_gpio_lev(PIN);
            bcm2835_gpio_set_eds(PIN);

            if( level == 0) { // LOW da durch Button auf ground gezogen.
                printf("Shutting Down\n");
                console+= "\nShutting Down";
                errorOverlayText = "System is shutting down. You can unplug the pi in 10 Seconds.";
                showErrorOverlay = true;
                running = false;
            } else {
                if(displayOn) {
                    turnDisplayOff();
                } else {
                    turnDisplayOn();
                }

            }
        }
        int event;
        if(waitpid(pid, &event,WNOHANG) > 0) { // Der Kindprozess ist gestorben.
            //std::cout << WIFEXITED(event) << " "<< WIFSIGNALED(event) << " "<< WTERMSIG( event) << " "<< WIFSTOPPED( event) << " "<< WSTOPSIG( event) << " "<< WEXITSTATUS(event)<< std::endl;
            runGNURadio();
        }

        if((errorOccured || showErrorOverlay ) && !displayOn) {
            turnDisplayOn();
        }

        if(displayOn) {

            if(std::time(0) - last_action > 300){ //Automatische Displayabschaltung nach 5 Minuten
                turnDisplayOff();
            }


            cairo_surface_t *surface =
                    cairo_image_surface_create (CAIRO_FORMAT_ARGB32, DISPLAY_WIDTH, DISPLAY_HEIGHT);
            cairo_t *cr =
                    cairo_create (surface);
            //cairo_move_to(cr,260,0);
            cairo_set_source_rgb(cr, .0, .0, .0);
            cairo_rectangle(cr,0,0,320,240);
            cairo_fill(cr);



            cairo_set_line_width(cr, 0.5);
            cairo_set_source_rgb(cr, .5, .5, .5);
            cairo_move_to(cr,73,0);
            cairo_line_to(cr,73,240);
            cairo_move_to(cr,0,35);
            cairo_line_to(cr,72,35);
            cairo_move_to(cr,0,72);
            cairo_line_to(cr,72,72);
            cairo_move_to(cr,0,108);
            cairo_line_to(cr,72,108);
            cairo_move_to(cr,0,143);
            cairo_line_to(cr,72,143);
            cairo_stroke(cr);

            setErrorSpace(cr, errorOccured || showErrorOverlay);


            { // Freispeicher anzeige

                struct statvfs64 diskData;
                statvfs64(".", &diskData);
                unsigned long long available = diskData.f_bavail*diskData.f_bsize;
                if(available < 34146848) // 2 MB
                    errorOccured = true;
                std::string end;
                if(available/1024> 1) {
                    end = " kb";
                    available/= 1024;
                    if(available/1024> 1) {
                        end = " Mb";
                        available/= 1024;
                        if(available/1024> 2) {
                            end = " Gb";
                            available/= 1024;
                        }
                    }
                }



                std::string space =  "Free:\n" + std::to_string(available)+ end;

                writeString(cr,0,0,space);
            }


            { // Load Average

                double avg[1];
                getloadavg(avg,1);
                std::string load =  "Load: " + std::to_string(avg[0]).substr(0,4);
                writeString(cr,0,73,load);
            }

            {   //packageCount

                writeString(cr,0,109,"Packets:"+std::to_string(packetCount));
            }

            { // Uhrzeit

                time_t t = time(0);   // get time now
                struct tm * now = localtime( & t );
                char buffer[9];

                std::strftime(buffer,9,"%H:%M:%S",now);
                writeString(cr,0,223,std::string(buffer));
            }




            { // Consolenausgabe
                PangoFontDescription *desc;
                PangoLayout* layout =  pango_cairo_create_layout(cr);

                pango_layout_set_width(layout,245 *PANGO_SCALE);

                pango_layout_set_wrap (layout,PANGO_WRAP_WORD_CHAR);

                pango_layout_set_text(layout, console.c_str(), -1);
                desc = pango_font_description_from_string("Sans Bold 11");
                pango_layout_set_font_description(layout, desc);
                pango_font_description_free(desc);

                cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
                pango_cairo_update_layout(cr, layout);
                int width, height;
                pango_layout_get_size (layout, &width, &height);
                width /= PANGO_SCALE;
                height /= PANGO_SCALE;

                cairo_move_to(cr,75,240-height);
                pango_cairo_show_layout(cr, layout);

                g_object_unref(layout);
            }


            if(showErrorOverlay) {
                cairo_set_source_rgba(cr, .0, .0, .0,.9);
                cairo_rectangle(cr,0,0,320,240);
                cairo_fill(cr);

                PangoFontDescription *desc;
                PangoLayout* layout =  pango_cairo_create_layout(cr);

                pango_layout_set_width(layout,310 *PANGO_SCALE);
                pango_layout_set_alignment(layout,PANGO_ALIGN_CENTER);

                pango_layout_set_wrap (layout,PANGO_WRAP_WORD_CHAR);

                pango_layout_set_text(layout, errorOverlayText.c_str(), -1);

                desc = pango_font_description_from_string("Sans Bold 20");
                pango_layout_set_font_description(layout, desc);
                pango_font_description_free(desc);

                int width, height;
                pango_layout_get_size (layout, &width, &height);
                width /= PANGO_SCALE;
                height /= PANGO_SCALE;

                cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
                cairo_move_to(cr,5,120- height/2 );
                pango_cairo_update_layout(cr, layout);
                pango_cairo_show_layout(cr, layout);

                g_object_unref(layout);

            }



            cairo_destroy (cr);


            unsigned char* data = cairo_image_surface_get_data(surface);

            STcontroller_Write_Picture ( (uint32_t*) data, PICTURE_PIXELS );
            //cairo_surface_write_to_png(surface,"image.png");
            cairo_surface_destroy (surface);

        }

        usleep(DELAY);
    }

}

void writeString(cairo_t *cr, int posX, int posY, std::string string)
{
    PangoFontDescription *desc;
    PangoLayout* layout =  pango_cairo_create_layout(cr);

    pango_layout_set_width(layout,72 *PANGO_SCALE);
    pango_layout_set_alignment(layout,PANGO_ALIGN_CENTER);

    pango_layout_set_wrap (layout,PANGO_WRAP_WORD_CHAR);

    pango_layout_set_text(layout, string.c_str(), -1);

    desc = pango_font_description_from_string("Sans Bold 11");
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_move_to(cr,posX,posY);
    pango_cairo_update_layout(cr, layout);
    pango_cairo_show_layout(cr, layout);

    g_object_unref(layout);
}


void setErrorSpace(cairo_t* cr, bool error) {
    if(error)
        cairo_set_source_rgb(cr, 1.0, .0, .0);
    else
        cairo_set_source_rgb(cr, .0, 1.0, .0);
    cairo_rectangle(cr,0,36,71,35);
    cairo_fill(cr);
}

void turnDisplayOn() {
    std::cout << "Turning Display on"<<std::endl;
    anzeige::last_action = std::time(0);
    TFT_turnDisplayOn();
    TFT_SetBacklightPWMValue(100);
    TFT_init_board();
    TFT_hard_reset();
    STcontroller_init();
    displayOn = true;
}

void turnDisplayOff() {
    TFT_turnDisplayOff();
    std::cout << "Turning Display off"<<std::endl;
    displayOn = false;
}

}
