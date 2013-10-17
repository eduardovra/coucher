CC=g++
AR=ar
RANLIB=ranlib
CFLAGS=-g -fPIC -I..
SRCS=$(wildcard *.cpp)
OBJS=$(SRCS:.cpp=.o)
TARGET=libMusic.a

$(TARGET): $(OBJS)
	rm -f $(TARGET)
	$(AR) cru $@ $(OBJS)
	$(RANLIB) $(TARGET)

%.o: %.cpp Makefile
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(OBJS) $(TARGET)
