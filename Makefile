CC=g++
CFLAGS=-g -fPIC -DFANR_OUTPUT_MIDI -DCAPTURE_JACK -DCAPTURE_ALSA -DCAPTURE_SOUNDFILE
LDFLAGS=-lQt5Gui -lQt5Widgets -lQt5Core -ljack -lasound -lpthread
INCLUDE=-Ilibs -I/usr/include/qt5
#SRCS=main.cpp ANR.cpp CaptureThread.cpp #CaptureThread_moc.cpp
#SRCS=$(filter-out *_moc.cpp, $(wildcard *.cpp))
SRCS=$(wildcard *.cpp)
HEADERS=$(wildcard *.h)
SRCS_MOC=$(HEADERS:.h=_moc.cpp)
OBJS_MOC=$(SRCS_MOC:.cpp=.o)
LIBS=libs/Music/libMusic.a libs/CppAddons/libCppAddons.a
OBJS=$(SRCS:.cpp=.o)
DEPS=$(SRCS:.cpp=.dep)
TARGET=coucher

$(info $(SRCS))
$(info $(OBJS))
$(info $(SRCS_MOC))
$(info $(OBJS_MOC))

all: $(TARGET)
	$(info all)

$(TARGET): $(OBJS) $(LIBS) $(OBJS_MOC) ANR_moc.o
	$(info  target)
	$(CC) -o $@ $(OBJS) $(OBJS_MOC) $(LIBS) $(LDFLAGS)

%.o: %.cpp Makefile
	$(info o)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

%_moc.o: %_moc.cpp Makefile
	$(info moc.o)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

#include *.dep

#$(DEPS): $(SRCS) $(DEPS) Makefile
#	$(CC) $(CFLAGS) $(INCLUDE) -MM $< > $@

#CaptureThread_moc.cpp: CaptureThread.h
#	/usr/lib64/qt5/bin/moc CaptureThread.h > CaptureThread_moc.cpp

%_moc.cpp: %.h Makefile
	$(info moc)
	/usr/lib64/qt5/bin/moc $< > $@

$(LIBS): libs Makefile
	$(info libs)
	make -C libs/Music
	make -C libs/CppAddons

clean:
	$(info clean)
	-rm -f *~ *.o $(TARGET) *_moc.cpp
	-make -C libs/Music clean
	-make -C libs/CppAddons clean
