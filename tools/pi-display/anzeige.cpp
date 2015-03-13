
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

#define DELAY 100000  //10 fps

int main(int argc, char ** argv) {
    if (!bcm2835_init())
        return 1;

    TFT_init_board();
    TFT_hard_reset();

    STcontroller_init();

    TFT_SetBacklightPWMValue( 100);
    std::thread t1(anzeige::readThread);

    anzeige::loop();
    t1.join();
}

namespace anzeige {

void readThread() {
    while(true) {
        std::string input;

        std::getline(std::cin,input );
        if(input.length()>0) {
            console += "\n"+input;
            lineCount ++;
            if(lineCount>13) {
                console = console.substr(console.find("\n")+1);
                lineCount--;
            }
        }
        usleep(DELAY);
    }
}


void loop()
{

    while(true) {
        time_t rawtime;
        struct tm * timeinfo;
        time(&rawtime);
        struct statvfs64 diskData;
        statvfs64(".", &diskData);
        unsigned long long available = diskData.f_bavail*diskData.f_bsize;
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

        std::string space =  "Frei:\n" + std::to_string(available)+ end;

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

        cairo_set_source_rgb(cr, .0, 1.0, .0);
        cairo_rectangle(cr,0,36,71,35);
        cairo_fill(cr);


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
    bcm2835_close();

}
}
