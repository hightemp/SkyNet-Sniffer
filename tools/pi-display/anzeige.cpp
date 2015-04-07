
#include <sys/statvfs.h>
#include <iostream>
#include <stdint.h>
#include "ST7789.h"
#include "anzeige.h"
#include <stdlib.h>
#include "math.h"
#include <time.h>
#include "unistd.h"
#include <cairo/cairo.h>
#include <bcm2835.h>
#include <string>
#include "tft.h"
#include <pango/pangocairo.h>
#include <thread>
#include <sys/wait.h>
#include <signal.h>

#define DELAY 100000  //10 fps
#define PIN 7

int main(int argc, char ** argv) {
    if (!bcm2835_init())
        return 1;


    bcm2835_gpio_fsel(PIN, BCM2835_GPIO_FSEL_INPT);
    // with a pullup
    bcm2835_gpio_set_pud(PIN, BCM2835_GPIO_PUD_UP);
    bcm2835_gpio_afen(PIN); // Detect Fallen Edge

    TFT_init_board();
    TFT_hard_reset();

    STcontroller_init();

    TFT_SetBacklightPWMValue( 100);
    //TFT_turnDisplayOff();

    if(pipe(anzeige::comPipe)) {
        anzeige::errorOccured = true;
        anzeige::console = "Pipe failed\n";
    }
    anzeige::stdio = dup(STDIN_FILENO);
    dup2(anzeige::comPipe[0],STDIN_FILENO);


    anzeige::runGNURadio();

    std::thread t1(anzeige::readThread);

    anzeige::loop();

    kill(anzeige::pid,9);
    TFT_SetBacklightPWMValue( 0);
    TFT_turnDisplayOff();

    bcm2835_gpio_clr_afen(PIN);
    bcm2835_gpio_set_eds(PIN);
    bcm2835_close();

    //t1.join();
    char* args[2];
    args[0] = "shutdown";
    args[1] = "now";
    //execvp("shutdown",args);

    return 0;
}

namespace anzeige {

void runGNURadio() {

    pid = fork();
    if (pid == 0) {
        dup2(stdio,STDIN_FILENO);
        dup2(anzeige::comPipe[1],STDOUT_FILENO);
        dup2(anzeige::comPipe[1],STDERR_FILENO);
        execvp("./skynet_sniffer.py",NULL);
        std::cout << "error running skynet Sniffer"<<std::endl;
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
            }
        }
        usleep(DELAY);
    }
}


void loop()
{

    while(running) {

        if (bcm2835_gpio_eds(PIN))
        {
            // Now clear the eds flag by setting it to 1
            bcm2835_gpio_set_eds(PIN);
            printf("Shutting Down\n");
            console+= "\nShutting Down";
            running = false;
        }
        int event;
        if(waitpid(pid, &event,WNOHANG) > 0) { // Unser Kind ist tot
            //std::cout << WIFEXITED(event) << " "<< WIFSIGNALED(event) << " "<< WTERMSIG( event) << " "<< WIFSTOPPED( event) << " "<< WSTOPSIG( event) << " "<< WEXITSTATUS(event)<< std::endl;
            runGNURadio();
        }


        struct statvfs64 diskData;
        statvfs64(".", &diskData);
        unsigned long long available = diskData.f_bavail*diskData.f_bsize;
        if(available < 34146848) // 2 MB
            errorOccured = true;
        //available = 2072693248;
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
        cairo_move_to(cr,72,0);
        cairo_line_to(cr,72,240);
        cairo_move_to(cr,0,35);
        cairo_line_to(cr,71,35);
        cairo_move_to(cr,0,72);
        cairo_line_to(cr,71,72);
        cairo_move_to(cr,0,108);
        cairo_line_to(cr,71,108);
        cairo_stroke(cr);

        setErrorSpace(cr, errorOccured);


        { // Freispeicher anzeige
            PangoFontDescription *desc;
            PangoLayout* layout =  pango_cairo_create_layout(cr);

            pango_layout_set_width(layout,71 *PANGO_SCALE);
            pango_layout_set_alignment(layout,PANGO_ALIGN_CENTER);

            pango_layout_set_wrap (layout,PANGO_WRAP_WORD_CHAR);

            pango_layout_set_text(layout, space.c_str(), -1);

            desc = pango_font_description_from_string("Sans Bold 11");
            pango_layout_set_font_description(layout, desc);
            pango_font_description_free(desc);

            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_move_to(cr,0,0);
            pango_cairo_update_layout(cr, layout);
            pango_cairo_show_layout(cr, layout);

            g_object_unref(layout);
        }


        { // Load Average

            double avg[1];
            getloadavg(avg,1);
            std::string load =  "Load: " + std::to_string(avg[0]).substr(0,4);
            PangoFontDescription *desc;
            PangoLayout* layout =  pango_cairo_create_layout(cr);

            pango_layout_set_width(layout,71 *PANGO_SCALE);
            pango_layout_set_alignment(layout,PANGO_ALIGN_CENTER);

            pango_layout_set_wrap (layout,PANGO_WRAP_WORD_CHAR);

            pango_layout_set_text(layout, load.c_str(), -1);

            desc = pango_font_description_from_string("Sans Bold 11");
            pango_layout_set_font_description(layout, desc);
            pango_font_description_free(desc);

            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_move_to(cr,0,73);
            pango_cairo_update_layout(cr, layout);
            pango_cairo_show_layout(cr, layout);

            g_object_unref(layout);
        }

        { // Uhrzeit

            time_t t = time(0);   // get time now
            struct tm * now = localtime( & t );
            std::string timeString =  std::to_string(now->tm_hour) + ":" + std::to_string(now->tm_min); // + ":" + std::to_string(now->tm_sec);
            PangoFontDescription *desc;
            PangoLayout* layout =  pango_cairo_create_layout(cr);

            pango_layout_set_width(layout,71 *PANGO_SCALE);
            pango_layout_set_alignment(layout,PANGO_ALIGN_CENTER);

            pango_layout_set_wrap (layout,PANGO_WRAP_WORD_CHAR);

            pango_layout_set_text(layout, timeString.c_str(), -1);

            desc = pango_font_description_from_string("Sans Bold 11");
            pango_layout_set_font_description(layout, desc);
            pango_font_description_free(desc);

            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_move_to(cr,0,223);
            pango_cairo_update_layout(cr, layout);
            pango_cairo_show_layout(cr, layout);

            g_object_unref(layout);
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



        cairo_destroy (cr);


        unsigned char* data = cairo_image_surface_get_data(surface);

        STcontroller_Write_Picture ( (uint32_t*) data, PICTURE_PIXELS );
        //cairo_surface_write_to_png(surface,"image.png");
        cairo_surface_destroy (surface);




        usleep(DELAY);



    }

}

void setErrorSpace(cairo_t* cr, bool error) {
    if(error)
        cairo_set_source_rgb(cr, 1.0, .0, .0);
    else
        cairo_set_source_rgb(cr, .0, 1.0, .0);
    cairo_rectangle(cr,0,36,71,35);
    cairo_fill(cr);
}
}
