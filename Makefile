CC=g++
CFLAGS=-g -fPIC -DFANR_OUTPUT_MIDI -DCAPTURE_JACK -DCAPTURE_ALSA
LDFLAGS=-lQt5Gui -lQt5Core -ljack -lasound -lpthread
INCLUDE=-Ilibs -I/usr/include/qt5
SRCS=main.cpp ANR.cpp CaptureThread.cpp CaptureThread_moc.cpp
LIBS=libs/Music/libMusic.a libs/CppAddons/libCppAddons.a
OBJS=$(SRCS:.cpp=.o)
DEPS=$(SRCS:.cpp=.dep)
TARGET=coucher

all: $(TARGET)

$(TARGET): $(OBJS) $(LIBS)
	$(CC) -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

$(OBJS): %.o: %.cpp %.dep Makefile
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

include *.dep

$(DEPS): %.dep: %.cpp Makefile
	$(CC) $(CFLAGS) $(INCLUDE) -MM $< > $@

CaptureThread_moc.cpp: CaptureThread.h
	/usr/lib64/qt5/bin/moc CaptureThread.h > CaptureThread_moc.cpp

$(LIBS):
	make -C libs/Music
	make -C libs/CppAddons

clean:
	-rm -f *~ *.o $(TARGET) CaptureThread_moc.cpp
	-make -C libs/Music clean
	-make -C libs/CppAddons clean
