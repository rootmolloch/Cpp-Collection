CFLAGS = -O3 -funroll-loops -fomit-frame-pointer -march=nehalem
LDFLAGS = -lstdc++ -lOpenCL -lm

CFLAGS += `pkg-config --cflags gtk+-3.0 gstreamer-1.0 gstreamer-video-1.0 alsa`
LDFLAGS += `pkg-config --libs gtk+-3.0 gstreamer-1.0 gstreamer-video-1.0 alsa`

CPPFLAGS = $(CFLAGS)


TARGETS = tarot.exe hello.exe player.exe pl.exe fractal.exe sat3.exe brain5.exe melody.exe Music2.exe equation.exe


.PHONY : clean

all : $(TARGETS)

clean : 
	for x in $(TARGETS) $(TARGETS:%.exe=%.o); do if [ -f "$$x" ]; then rm "$$x";fi;done

%.exe: %.cpp
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)


%.exe: %.c
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

brain5.exe : myBib.hpp
sat2.exe : myBib.hpp


