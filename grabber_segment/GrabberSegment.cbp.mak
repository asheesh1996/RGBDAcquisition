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
CFLAGS =  -Wall
RESINC = 
LIBDIR = 
LIB =  -lGL -lGLU -lX11 ../acquisition/libAcquisition.so ../acquisition_mux/libAcquisitionMux.so ../3dparty/OpenNI/Platform/Linux/Bin/x64-Release/libnimCodecs.so ../3dparty/OpenNI/Platform/Linux/Bin/x64-Release/libnimMockNodes.so ../3dparty/OpenNI/Platform/Linux/Bin/x64-Release/libnimRecorder.so ../3dparty/OpenNI/Platform/Linux/Bin/x64-Release/libOpenNI.jni.so ../3dparty/OpenNI/Platform/Linux/Bin/x64-Release/libOpenNI.so ../3dparty/OpenNI2/Bin/x64-Release/libOpenNI2.so ../openni1_acquisition_shared_library/libOpenNI1Acquisition.so ../openni2_acquisition_shared_library/libOpenNI2Acquisition.so ../opengl_acquisition_shared_library/libOpenGLAcquisition.so ../template_acquisition_shared_library/libTemplateAcquisition.so ../v4l2_acquisition_shared_library/libV4L2Acquisition.a ../v4l2stereo_acquisition_shared_library/libV4L2StereoAcquisition.a ../acquisitionSegment/libacquisitionSegment.a ../tools/Calibration/libCalibrationLibrary.a
LDFLAGS = 

INC_DEBUG =  $(INC)
CFLAGS_DEBUG =  $(CFLAGS) -g
RESINC_DEBUG =  $(RESINC)
RCFLAGS_DEBUG =  $(RCFLAGS)
LIBDIR_DEBUG =  $(LIBDIR)
LIB_DEBUG = $(LIB)
LDFLAGS_DEBUG =  $(LDFLAGS)
OBJDIR_DEBUG = obj/Debug
DEP_DEBUG = 
OUT_DEBUG = GrabberSegment

INC_RELEASE =  $(INC)
CFLAGS_RELEASE =  $(CFLAGS) -O2
RESINC_RELEASE =  $(RESINC)
RCFLAGS_RELEASE =  $(RCFLAGS)
LIBDIR_RELEASE =  $(LIBDIR)
LIB_RELEASE = $(LIB)
LDFLAGS_RELEASE =  $(LDFLAGS) -s
OBJDIR_RELEASE = obj/Release
DEP_RELEASE = 
OUT_RELEASE = GrabberSegment

OBJ_DEBUG = $(OBJDIR_DEBUG)/main.o

OBJ_RELEASE = $(OBJDIR_RELEASE)/main.o

all: debug release

clean: clean_debug clean_release

before_debug: 
	test -d $(OBJDIR_DEBUG) || mkdir -p $(OBJDIR_DEBUG)

after_debug: 

debug: before_debug out_debug after_debug

out_debug: before_debug $(OBJ_DEBUG) $(DEP_DEBUG)
	$(LD) $(LIBDIR_DEBUG) -o $(OUT_DEBUG) $(OBJ_DEBUG)  $(LDFLAGS_DEBUG) $(LIB_DEBUG)

$(OBJDIR_DEBUG)/main.o: main.c
	$(CC) $(CFLAGS_DEBUG) $(INC_DEBUG) -c main.c -o $(OBJDIR_DEBUG)/main.o

clean_debug: 
	rm -f $(OBJ_DEBUG) $(OUT_DEBUG)
	rm -rf $(OBJDIR_DEBUG)

before_release: 
	test -d $(OBJDIR_RELEASE) || mkdir -p $(OBJDIR_RELEASE)

after_release: 

release: before_release out_release after_release

out_release: before_release $(OBJ_RELEASE) $(DEP_RELEASE)
	$(LD) $(LIBDIR_RELEASE) -o $(OUT_RELEASE) $(OBJ_RELEASE)  $(LDFLAGS_RELEASE) $(LIB_RELEASE)

$(OBJDIR_RELEASE)/main.o: main.c
	$(CC) $(CFLAGS_RELEASE) $(INC_RELEASE) -c main.c -o $(OBJDIR_RELEASE)/main.o

clean_release: 
	rm -f $(OBJ_RELEASE) $(OUT_RELEASE)
	rm -rf $(OBJDIR_RELEASE)

.PHONY: before_debug after_debug clean_debug before_release after_release clean_release

