#------------------------------------------------------------------------------#
# This makefile was generated by 'cbp2make' tool rev.138                       #
#------------------------------------------------------------------------------#


WORKDIR = `pwd`

CC = gcc
CXX = g++
AR = ar
LD = g++
WINDRES = windres

INC = 
CFLAGS = 
RESINC = 
LIBDIR = 
LIB = 
LDFLAGS = 

INC_DEBUG =  $(INC)
CFLAGS_DEBUG =  $(CFLAGS) -Wall -g
RESINC_DEBUG =  $(RESINC)
RCFLAGS_DEBUG =  $(RCFLAGS)
LIBDIR_DEBUG =  $(LIBDIR)
LIB_DEBUG = $(LIB)
LDFLAGS_DEBUG =  $(LDFLAGS)
OBJDIR_DEBUG = obj/Debug
DEP_DEBUG = 
OUT_DEBUG = libV4L2Acquisition.a

INC_RELEASE =  $(INC)
CFLAGS_RELEASE =  $(CFLAGS) -Wall -O2
RESINC_RELEASE =  $(RESINC)
RCFLAGS_RELEASE =  $(RCFLAGS)
LIBDIR_RELEASE =  $(LIBDIR)
LIB_RELEASE = $(LIB)
LDFLAGS_RELEASE =  $(LDFLAGS) -s
OBJDIR_RELEASE = obj/Release
DEP_RELEASE = 
OUT_RELEASE = libV4L2Acquisition.a

OBJ_DEBUG = $(OBJDIR_DEBUG)/PixelFormatConversions.o $(OBJDIR_DEBUG)/V4L2Acquisition.o $(OBJDIR_DEBUG)/V4L2IntrinsicCalibration.o $(OBJDIR_DEBUG)/V4L2Wrapper.o $(OBJDIR_DEBUG)/V4L2_c.o

OBJ_RELEASE = $(OBJDIR_RELEASE)/PixelFormatConversions.o $(OBJDIR_RELEASE)/V4L2Acquisition.o $(OBJDIR_RELEASE)/V4L2IntrinsicCalibration.o $(OBJDIR_RELEASE)/V4L2Wrapper.o $(OBJDIR_RELEASE)/V4L2_c.o

all: debug release

clean: clean_debug clean_release

before_debug: 
	test -d $(OBJDIR_DEBUG) || mkdir -p $(OBJDIR_DEBUG)

after_debug: 

debug: before_debug out_debug after_debug

out_debug: before_debug $(OBJ_DEBUG) $(DEP_DEBUG)
	$(AR) rcs $(OUT_DEBUG) $(OBJ_DEBUG)

$(OBJDIR_DEBUG)/PixelFormatConversions.o: PixelFormatConversions.c
	$(CC) $(CFLAGS_DEBUG) $(INC_DEBUG) -c PixelFormatConversions.c -o $(OBJDIR_DEBUG)/PixelFormatConversions.o

$(OBJDIR_DEBUG)/V4L2Acquisition.o: V4L2Acquisition.c
	$(CC) $(CFLAGS_DEBUG) $(INC_DEBUG) -c V4L2Acquisition.c -o $(OBJDIR_DEBUG)/V4L2Acquisition.o

$(OBJDIR_DEBUG)/V4L2IntrinsicCalibration.o: V4L2IntrinsicCalibration.c
	$(CC) $(CFLAGS_DEBUG) $(INC_DEBUG) -c V4L2IntrinsicCalibration.c -o $(OBJDIR_DEBUG)/V4L2IntrinsicCalibration.o

$(OBJDIR_DEBUG)/V4L2Wrapper.o: V4L2Wrapper.c
	$(CC) $(CFLAGS_DEBUG) $(INC_DEBUG) -c V4L2Wrapper.c -o $(OBJDIR_DEBUG)/V4L2Wrapper.o

$(OBJDIR_DEBUG)/V4L2_c.o: V4L2_c.c
	$(CC) $(CFLAGS_DEBUG) $(INC_DEBUG) -c V4L2_c.c -o $(OBJDIR_DEBUG)/V4L2_c.o

clean_debug: 
	rm -f $(OBJ_DEBUG) $(OUT_DEBUG)
	rm -rf $(OBJDIR_DEBUG)

before_release: 
	test -d $(OBJDIR_RELEASE) || mkdir -p $(OBJDIR_RELEASE)

after_release: 

release: before_release out_release after_release

out_release: before_release $(OBJ_RELEASE) $(DEP_RELEASE)
	$(AR) rcs $(OUT_RELEASE) $(OBJ_RELEASE)

$(OBJDIR_RELEASE)/PixelFormatConversions.o: PixelFormatConversions.c
	$(CC) $(CFLAGS_RELEASE) $(INC_RELEASE) -c PixelFormatConversions.c -o $(OBJDIR_RELEASE)/PixelFormatConversions.o

$(OBJDIR_RELEASE)/V4L2Acquisition.o: V4L2Acquisition.c
	$(CC) $(CFLAGS_RELEASE) $(INC_RELEASE) -c V4L2Acquisition.c -o $(OBJDIR_RELEASE)/V4L2Acquisition.o

$(OBJDIR_RELEASE)/V4L2IntrinsicCalibration.o: V4L2IntrinsicCalibration.c
	$(CC) $(CFLAGS_RELEASE) $(INC_RELEASE) -c V4L2IntrinsicCalibration.c -o $(OBJDIR_RELEASE)/V4L2IntrinsicCalibration.o

$(OBJDIR_RELEASE)/V4L2Wrapper.o: V4L2Wrapper.c
	$(CC) $(CFLAGS_RELEASE) $(INC_RELEASE) -c V4L2Wrapper.c -o $(OBJDIR_RELEASE)/V4L2Wrapper.o

$(OBJDIR_RELEASE)/V4L2_c.o: V4L2_c.c
	$(CC) $(CFLAGS_RELEASE) $(INC_RELEASE) -c V4L2_c.c -o $(OBJDIR_RELEASE)/V4L2_c.o

clean_release: 
	rm -f $(OBJ_RELEASE) $(OUT_RELEASE)
	rm -rf $(OBJDIR_RELEASE)

.PHONY: before_debug after_debug clean_debug before_release after_release clean_release

