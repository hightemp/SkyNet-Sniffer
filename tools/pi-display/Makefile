FLAGS = -pthread -I/usr/include/pango-1.0 -I/usr/include/cairo -I/usr/include/glib-2.0 -I/usr/lib/arm-linux-gnueabihf/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include/freetype2 -I/usr/include/libpng12 -lpangocairo-1.0 -lpango-1.0 -lgobject-2.0 -lglib-2.0 -lcairo -lbcm2835

all: anzeige

anzeige: anzeige.o tft.o ST7789.o
	g++ anzeige.o tft.o ST7789.o -std=gnu++11  $(FLAGS) -o anzeige

tft.o: tft.c tft.h ST7789.h
	g++ -Os -c tft.c
	
ST7789.o: ST7789.c ST7789.h
	g++ -Os -c ST7789.c
	
anzeige.o: anzeige.cpp anzeige.h ST7789.h
	g++ -Os -c anzeige.cpp  -std=gnu++11 $(FLAGS)

clean:
	rm -rf *o anzeige

