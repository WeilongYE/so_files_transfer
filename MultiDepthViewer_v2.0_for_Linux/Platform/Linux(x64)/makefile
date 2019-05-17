CC = g++

INCPATH = -I../../ThirdParty/OpenNI2/Include \
        	        -I../../ThirdParty/LibUSB/Include

LIBPATH = -L../../ThirdParty/OpenNI2/Lib \
                       -L../../ThirdParty/OpenNI2/Redist/Linux \
	  	  
LDLIBS += -lOpenNI2 -lusb-1.0 -lGL -lGLU -lglut


CFLAGS=-O0 -DLINUX -g -Wall  -fpermissive -std=c++11  -Wno-unused-result
LDFLAGS += -Wl,-rpath=.

SOURCE = ../../src/main.cpp \
                       ../../src/Viewer.cpp \
                       ../../src/COBDevice.cpp \

TARGET = MultiDepthViewer1

all:
	$(CC) $(CFLAGS)  $(INCPATH) $(LIBPATH)  $(SOURCE) $(LDFLAGS) $(LDLIBS) -lpthread  -o $(TARGET)
	cp -r ../../ThirdParty/OpenNI2/Redist/Linux/*  ./
clean:
	rm -f *.o $(TARGET) *~

