export CC=g++
export AR=ar
export RANLIB=ranlib
CFLAGS=-g -fPIC -DFANR_OUTPUT_MIDI -DCAPTURE_SOUNDFILE #-DCAPTURE_JACK -DCAPTURE_ALSA
# This works in Ubuntu with qt4
LDFLAGS=-lQtCore -lQtGui -ljack -lasound -lsndfile -lpthread
# This works on Fedora with qt5
LDFLAGS=-lQt5Core -lQt5Gui -lQt5Widgets -ljack -lasound -lsndfile -lpthread
export INCLUDE=-Ilibs
INCLUDE+=-I$(shell qmake -query QT_INSTALL_HEADERS)
INCLUDE+=-I$(shell qmake -query QT_INSTALL_HEADERS)/QtGui
INCLUDE+=-I$(shell qmake -query QT_INSTALL_HEADERS)/QtCore
INCLUDE+=-I$(shell qmake -query QT_INSTALL_HEADERS)/QtWidgets
SRCS=$(wildcard *.cpp)
HEADERS=$(wildcard *.h)
SRCS_MOC=$(HEADERS:.h=_moc.cpp)
OBJS_MOC=$(SRCS_MOC:.cpp=.o)
LIBS=libs/Music/libMusic.a libs/CppAddons/libCppAddons.a
OBJS=$(SRCS:.cpp=.o)
DEPS=$(SRCS:.cpp=.dep)
TARGET=coucher

all: $(TARGET)

$(TARGET): $(OBJS) $(LIBS) $(OBJS_MOC) ANR_moc.o
	$(CC) -o $@ $(OBJS) $(OBJS_MOC) $(LIBS) $(LDFLAGS)

%.o: %.cpp Makefile
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

%_moc.o: %_moc.cpp Makefile
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

%_moc.cpp: %.h Makefile
	moc $< > $@

$(LIBS): libs Makefile
	make -C libs/Music
	make -C libs/CppAddons

clean:
	-rm -f *~ *.o $(TARGET) *_moc.cpp
	-make -C libs/Music clean
	-make -C libs/CppAddons clean
