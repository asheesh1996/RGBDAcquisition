#include "Acquisition.h"
#include "acquisition_setup.h"
#include "pluginLinker.h"

#include "../tools/Timers/timer.h"
#include "../tools/OperatingSystem/OperatingSystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NORMAL   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */

char warnDryRun=0;

//This should probably pass on to each of the sub modules
float minDistance = -10;
float scaleFactor = 0.0021;

unsigned int simulateTick=0;
unsigned long simulatedTickValue=0;


//This holds all the info on states of modules and devices
struct acquisitionModuleStates module[NUMBER_OF_POSSIBLE_MODULES]={0};



int acquisitionRegisterTerminationSignal(void * callback)
{
  return registerTerminationSignal(callback);
}

 int acquisitionCleanOverrides(ModuleIdentifier moduleID,DeviceIdentifier devID)
 {
    if ( module[moduleID].device[devID].overrideColorFrame!=0 )
        {
          free(module[moduleID].device[devID].overrideColorFrame);
          module[moduleID].device[devID].overrideColorFrame=0;
        }

    if ( module[moduleID].device[devID].overrideDepthFrame!=0 )
        {
          free(module[moduleID].device[devID].overrideDepthFrame);
          module[moduleID].device[devID].overrideDepthFrame=0;
        }

    return 1;
 }

int acquisitionSimulateTime(unsigned long timeInMillisecs)
{
  simulateTick=1;
  simulatedTickValue=timeInMillisecs;
  return 1;
}

unsigned long GetTickCount()
{
   if (simulateTick) { return simulatedTickValue; }
   return GetTickCountMilliseconds();
}

int acquisitionFileExists(char * filename)
{
  if (filename==0) { return 0; }
  FILE * fp  = fopen(filename,"r");
    if (fp!=0)
    {
      fclose(fp);
      return 1;
    }

  return 0;
}

int makepath(char * path)
{
  if (path==0) { return 0; }
    char command[2048];
    sprintf(command,"mkdir -p %s",path);
    fprintf(stderr,"Executing .. %s \n",command);

    return system(command);
}


void countdownDelay(int seconds)
{
    if (seconds==0) { return; } //No delay do nothing!
    int secCounter=seconds;

    for (secCounter=seconds; secCounter>0; secCounter--)
    {
      fprintf(stderr,"%u\n",secCounter);
      sleepMilliseconds(1000); // Waiting a while for the glitch frames to pass

    }
    sleepMilliseconds(1000); // Waiting a while for the glitch frames to pass
}


void acquisitionStartTimer(unsigned int timerID)
{
    StartTimer(timerID);
}

unsigned int acquisitionStopTimer(unsigned int timerID)
{
    return EndTimer(timerID);
}

float acquisitionGetTimerFPS(unsigned int timerID)
{
    return GetFPSTimer(timerID);
}



unsigned int simplePow(unsigned int base,unsigned int exp)
{
    if (exp==0) return 1;
    unsigned int retres=base;
    unsigned int i=0;
    for (i=0; i<exp-1; i++)
    {
        retres*=base;
    }
    return retres;
}


int savePCD_PointCloud(char * filename ,unsigned short * depthFrame ,unsigned char * colorFrame , unsigned int width , unsigned int height , float cx , float cy , float fx , float fy )
{
    if(depthFrame==0) { fprintf(stderr,"saveToPCD_PointCloud(%s) called for an unallocated (empty) depth frame , will not write any file output\n",filename); return 0; }
    if(colorFrame==0) { fprintf(stderr,"saveToPCD_PointCloud(%s) called for an unallocated (empty) color frame , will not write any file output\n",filename); return 0; }

    FILE *fd=0;
    fd = fopen(filename,"wb");
    if (fd!=0)
    {
        fprintf(fd, "# .PCD v.7 - Point Cloud Data file format\n");
        fprintf(fd, "FIELDS x y z rgb\n");
        fprintf(fd, "SIZE 4 4 4 4\n");
        fprintf(fd, "TYPE F F F U\n");
        fprintf(fd, "COUNT 1\n");
        fprintf(fd, "WIDTH %u\n",width);
        fprintf(fd, "HEIGHT %u\n",height);
        fprintf(fd, "POINTS %u\n",width*height);
        fprintf(fd, "DATA ascii\n");

        unsigned short * depthPTR = depthFrame;
        unsigned char  * colorPTR = colorFrame;

        unsigned int px=0,py=0;
        float x=0.0,y=0.0,z=0.0;
        unsigned char * r , * b , * g;
        unsigned int rgb=0;

        //Restart Depth
        depthPTR = depthFrame;


        for (py=0; py<height; py++)
        {
         for (px=0; px<width; px++)
         {
           z = * depthPTR; ++depthPTR;
           x = (px - cx) * (z + minDistance) * scaleFactor * (width/height) ;
           y = (py - cy) * (z + minDistance) * scaleFactor;

           r=colorPTR; ++colorPTR;
           g=colorPTR; ++colorPTR;
           b=colorPTR; ++colorPTR;


        /* To pack it :
            int rgb = ((int)r) << 16 | ((int)g) << 8 | ((int)b);

           To unpack it :
            int rgb = ...;
            uint8_t r = (rgb >> 16) & 0x0000ff;
            uint8_t g = (rgb >> 8) & 0x0000ff;
            uint8_t b = (rgb) & 0x0000ff; */
           rgb = ((int)*r) << 16 | ((int)*g) << 8 | ((int)*b);

           fprintf(fd, "%0.4f %0.4f %0.4f %u\n",x,y,z,rgb);
         }
        }
        fclose(fd);
        return 1;
    }
    else
    {
        fprintf(stderr,"SaveRawImageToFile could not open output file %s\n",filename);
        return 0;
    }

   return 0;
}



int savePCD_PointCloudNoEmpty(char * filename ,unsigned short * depthFrame ,unsigned char * colorFrame , unsigned int width , unsigned int height , float cx , float cy , float fx , float fy )
{
    if(depthFrame==0) { fprintf(stderr,"saveToPCD_PointCloud(%s) called for an unallocated (empty) depth frame , will not write any file output\n",filename); return 0; }
    if(colorFrame==0) { fprintf(stderr,"saveToPCD_PointCloud(%s) called for an unallocated (empty) color frame , will not write any file output\n",filename); return 0; }

    FILE *fd=0;
    fd = fopen(filename,"wb");
    if (fd!=0)
    {
        fprintf(fd, "# .PCD v.7 - Point Cloud Data file format\n");
        fprintf(fd, "FIELDS x y z rgb\n");
        fprintf(fd, "SIZE 4 4 4 4\n");
        fprintf(fd, "TYPE F F F U\n");
        fprintf(fd, "COUNT 1\n");

        unsigned short * depthPTR = depthFrame;
        unsigned char  * colorPTR = colorFrame;

        unsigned int px=0,py=0;
        float x=0.0,y=0.0,z=0.0;
        unsigned char * r , * b , * g;
        unsigned int rgb=0;

        unsigned int totalPoints = 0;
        for (py=0; py<height; py++)
        {
         for (px=0; px<width; px++)
         {
           if (*depthPTR!=0) { ++totalPoints; }  ++depthPTR;
         }
        }
        fprintf(fd, "WIDTH %u\n",totalPoints);
        fprintf(fd, "HEIGHT %u\n",1);
        fprintf(fd, "POINTS %u\n",totalPoints);
        fprintf(fd, "DATA ascii\n");


        //Restart Depth
        depthPTR = depthFrame;


        for (py=0; py<height; py++)
        {
         for (px=0; px<width; px++)
         {
           z = * depthPTR; ++depthPTR;
           x = (px - cx) * (z + minDistance) * scaleFactor * (width/height) ;
           y = (py - cy) * (z + minDistance) * scaleFactor;

           r=colorPTR; ++colorPTR;
           g=colorPTR; ++colorPTR;
           b=colorPTR; ++colorPTR;


        /* To pack it :
            int rgb = ((int)r) << 16 | ((int)g) << 8 | ((int)b);

           To unpack it :
            int rgb = ...;
            uint8_t r = (rgb >> 16) & 0x0000ff;
            uint8_t g = (rgb >> 8) & 0x0000ff;
            uint8_t b = (rgb) & 0x0000ff; */
           rgb = ((int)*r) << 16 | ((int)*g) << 8 | ((int)*b);

           if (z!=0)
              { fprintf(fd, "%0.4f %0.4f %0.4f %u\n",x,y,z,rgb); }
         }
        }
        fclose(fd);
        return 1;
    }
    else
    {
        fprintf(stderr,"SaveRawImageToFile could not open output file %s\n",filename);
        return 0;
    }

   return 0;
}


int swapEndiannessPNM(void * pixels , unsigned int width , unsigned int height , unsigned int channels , unsigned int bitsperpixel)
{
  unsigned char * traverser=(unsigned char * ) pixels;
  unsigned char * traverserSwap1=(unsigned char * ) pixels;
  unsigned char * traverserSwap2=(unsigned char * ) pixels;

  unsigned int bytesperpixel = (bitsperpixel/8);
  unsigned char * endOfMem = traverser + width * height * channels * bytesperpixel;

  unsigned char tmp ;
  while ( ( traverser < endOfMem)  )
  {
    traverserSwap1 = traverser;
    traverserSwap2 = traverser+1;

    tmp = *traverserSwap1;
    *traverserSwap1 = *traverserSwap2;
    *traverserSwap2 = tmp;

    traverser += bytesperpixel;
  }

 return 1;
}


int acquisitionSaveRawImageToFile(char * filename,unsigned char * pixels , unsigned int width , unsigned int height , unsigned int channels , unsigned int bitsperpixel)
{
   // fprintf(stderr,"acquisitionSaveRawImageToFile(%s) called\n",filename);

    #if USE_REGULAR_BYTEORDER_FOR_PNM
     //Want Conformance to the NETPBM spec http://en.wikipedia.org/wiki/Netpbm_format#16-bit_extensions
     if (bitsperpixel==16) { swapEndiannessPNM(pixels , width , height , channels , bitsperpixel); }
    #else
      #warning "We are using Our Local Byte Order for saving files , this makes things fast but is incompatible with other PNM loaders"
    #endif // USE_REGULAR_BYTEORDER_FOR_PNM

    if ( (width==0) || (height==0) || (channels==0) || (bitsperpixel==0) ) { fprintf(stderr,"acquisitionSaveRawImageToFile(%s) called with zero dimensions\n",filename); return 0;}
    if(pixels==0) { fprintf(stderr,"acquisitionSaveRawImageToFile(%s) called for an unallocated (empty) frame , will not write any file output\n",filename); return 0; }
    if (bitsperpixel>16) { fprintf(stderr,"PNM does not support more than 2 bytes per pixel..!\n"); return 0; }

    FILE *fd=0;
    fd = fopen(filename,"wb");

    if (fd!=0)
    {
        unsigned int n;
        if (channels==3) fprintf(fd, "P6\n");
        else if (channels==1) fprintf(fd, "P5\n");
        else
        {
            fprintf(stderr,"Invalid channels arg (%u) for SaveRawImageToFile\n",channels);
            fclose(fd);
            return 1;
        }

        /*
        char timeStampStr[256]={0};
        GetDateString(timeStampStr,"TIMESTAMP",1,0,0,0,0,0,0,0);
        fprintf(fd, "#%s\n", timeStampStr );*/

        fprintf(fd, "#TIMESTAMP %lu\n",GetTickCount());


        fprintf(fd, "%d %d\n%u\n", width, height , simplePow(2 ,bitsperpixel)-1);

        float tmp_n = (float) bitsperpixel/ 8;
        tmp_n = tmp_n *  width * height * channels ;
        n = (unsigned int) tmp_n;

        fwrite(pixels, 1 , n , fd);
        fflush(fd);
        fclose(fd);
        return 1;
    }
    else
    {
        fprintf(stderr,"SaveRawImageToFile could not open output file %s\n",filename);
        return 0;
    }
    return 0;
}

//Ok this is basically casting the 2 bytes of depth into 3 RGB bytes leaving one color channel off (the blue one)
//depth is casted to char to simplify things , but that adds sizeof(short) to the pointer arethemetic!
unsigned char * convertShortDepthToRGBDepth(unsigned short * depth,unsigned int width , unsigned int height)
{
  if (depth==0)  { fprintf(stderr,"Depth is not allocated , cannot perform DepthToRGB transformation \n"); return 0; }
  unsigned char * depthPTR= (unsigned char*) depth; // This will be the traversing pointer for input
  unsigned char * depthLimit = (unsigned char*) depth + width*height * sizeof(unsigned short); //<- we use sizeof(short) because we have casted to char !


  unsigned char * outFrame = (unsigned char*) malloc(width*height*3*sizeof(unsigned char));
  if (outFrame==0) { fprintf(stderr,"Could not perform DepthToRGB transformation\nNo memory for new frame\n"); return 0; }

  unsigned char * outFramePTR = outFrame; // This will be the traversing pointer for output
  while ( depthPTR<depthLimit )
  {
     * outFramePTR = *depthPTR; ++outFramePTR; ++depthPTR;
     * outFramePTR = *depthPTR; ++outFramePTR; ++depthPTR;
     * outFramePTR = 0;         ++outFramePTR;
  }
 return outFrame;
}


//Ok this is basically casting the 2 bytes of depth into 3 RGB bytes leaving one color channel off (the blue one)
//depth is casted to char to simplify things , but that adds sizeof(short) to the pointer arethemetic!
unsigned char * convertShortDepthToCharDepth(unsigned short * depth,unsigned int width , unsigned int height , unsigned int min_depth , unsigned int max_depth)
{
  if (depth==0)  { fprintf(stderr,"Depth is not allocated , cannot perform DepthToRGB transformation \n"); return 0; }
  unsigned short * depthPTR= depth; // This will be the traversing pointer for input
  unsigned short * depthLimit =  depth + width*height; //<- we use sizeof(short) because we have casted to char !


  unsigned char * outFrame = (unsigned char*) malloc(width*height*1*sizeof(char));
  if (outFrame==0) { fprintf(stderr,"Could not perform DepthToRGB transformation\nNo memory for new frame\n"); return 0; }

  float depth_range = max_depth-min_depth;
  if (depth_range ==0 ) { depth_range = 1; }
  float multiplier = 255 / depth_range;


  unsigned char * outFramePTR = outFrame; // This will be the traversing pointer for output
  while ( depthPTR<depthLimit )
  {
     unsigned int scaled = (unsigned int) (*depthPTR) * multiplier;
     unsigned char scaledChar = (unsigned char) scaled;
     * outFramePTR = scaledChar;

     ++outFramePTR;
     ++depthPTR;
  }
 return outFrame;
}


//Ok this is basically casting the 2 bytes of depth into 3 RGB bytes ( grayscale )
unsigned char * convertShortDepthTo3CharDepth(unsigned short * depth,unsigned int width , unsigned int height , unsigned int min_depth , unsigned int max_depth)
{
  if (depth==0)  { fprintf(stderr,"Depth is not allocated , convertShortDepthTo3CharDepth cannot continue \n"); return 0; }
  unsigned short * depthPTR= depth; // This will be the traversing pointer for input
  unsigned short * depthLimit =  depth + width*height; //<- we use sizeof(short) because we have casted to char !


  unsigned char * outFrame = (unsigned char*) malloc(width*height*3*sizeof(unsigned char));
  if (outFrame==0) { fprintf(stderr,"Could not perform DepthToRGB transformation\nNo memory for new frame\n"); return 0; }

  float depth_range = max_depth-min_depth;
  if (depth_range ==0 ) { depth_range = 1; }
  float multiplier = 255 / depth_range;


  unsigned char * outFramePTR = outFrame; // This will be the traversing pointer for output
  while ( depthPTR<depthLimit )
  {
     unsigned int scaled = (unsigned int) (*depthPTR) * multiplier;
     unsigned char scaledChar = (unsigned char) scaled;
     * outFramePTR = scaledChar; ++outFramePTR;
     * outFramePTR = scaledChar; ++outFramePTR;
     * outFramePTR = scaledChar; ++outFramePTR;

     ++depthPTR;
  }
 return outFrame;
}


int acquisitionIsModuleAvailiable(ModuleIdentifier moduleID)
{
  if (moduleID < NUMBER_OF_POSSIBLE_MODULES)
  {
   char tmp[1024];
   return getPluginPath (
                         getPluginStr(moduleID,PLUGIN_PATH_STR) ,
                         getPluginStr(moduleID,PLUGIN_LIBNAME_STR) ,
                         tmp,
                         1024
                         );
  }
  return 0;
}


int acquisitionGetModulesCount()
{
  unsigned int modules = 0;

  if ( acquisitionIsModuleAvailiable(V4L2_ACQUISITION_MODULE) )          { fprintf(stderr,"V4L2 module found \n");       ++modules; }
  if ( acquisitionIsModuleAvailiable(V4L2STEREO_ACQUISITION_MODULE) )    { fprintf(stderr,"V4L2Stereo module found \n"); ++modules; }
  if ( acquisitionIsModuleAvailiable(OPENGL_ACQUISITION_MODULE) )        { fprintf(stderr,"OpenGL module found \n");     ++modules; }
  if ( acquisitionIsModuleAvailiable(TEMPLATE_ACQUISITION_MODULE) )      { fprintf(stderr,"Template module found \n");   ++modules; }
  if ( acquisitionIsModuleAvailiable(FREENECT_ACQUISITION_MODULE) )      { fprintf(stderr,"Freenect module found \n");   ++modules; }
  if ( acquisitionIsModuleAvailiable(OPENNI1_ACQUISITION_MODULE) )       { fprintf(stderr,"OpenNI1 module found \n");    ++modules; }
  if ( acquisitionIsModuleAvailiable(OPENNI2_ACQUISITION_MODULE) )       { fprintf(stderr,"OpenNI2 module found \n");    ++modules; }
  if ( acquisitionIsModuleAvailiable(NETWORK_ACQUISITION_MODULE) )       { fprintf(stderr,"Network module found \n");    ++modules; }

  return modules;
}


ModuleIdentifier getModuleIdFromModuleName(char * moduleName)
{
   ModuleIdentifier moduleID = 0;
          if (strcasecmp("FREENECT",moduleName)==0 )  { moduleID = FREENECT_ACQUISITION_MODULE; } else
          if (strcasecmp("OPENNI",moduleName)==0 )  { moduleID = OPENNI1_ACQUISITION_MODULE;  } else
          if (strcasecmp("OPENNI1",moduleName)==0 )  { moduleID = OPENNI1_ACQUISITION_MODULE;  } else
          if (strcasecmp("OPENNI2",moduleName)==0 )  { moduleID = OPENNI2_ACQUISITION_MODULE;  } else
          if (strcasecmp("OPENGL",moduleName)==0 )   { moduleID = OPENGL_ACQUISITION_MODULE;   } else
          if (strcasecmp("V4L2",moduleName)==0 )   { moduleID = V4L2_ACQUISITION_MODULE;   } else
          if (strcasecmp("V4L2STEREO",moduleName)==0 )   { moduleID = V4L2STEREO_ACQUISITION_MODULE;   } else
          if (strcasecmp("TEMPLATE",moduleName)==0 )  { moduleID = TEMPLATE_ACQUISITION_MODULE; } else
          if (strcasecmp("NETWORK",moduleName)==0 )   { moduleID = NETWORK_ACQUISITION_MODULE; }
   return moduleID;
}


char * getModuleNameFromModuleID(ModuleIdentifier moduleID)
{
  switch (moduleID)
    {
      case V4L2_ACQUISITION_MODULE    :  return (char*) "V4L2 MODULE"; break;
      case V4L2STEREO_ACQUISITION_MODULE    :  return (char*) "V4L2STEREO MODULE"; break;
      case FREENECT_ACQUISITION_MODULE:  return (char*) "FREENECT MODULE"; break;
      case OPENNI1_ACQUISITION_MODULE :  return (char*) "OPENNI1 MODULE"; break;
      case OPENNI2_ACQUISITION_MODULE :  return (char*) "OPENNI2 MODULE"; break;
      case OPENGL_ACQUISITION_MODULE :  return (char*) "OPENGL MODULE"; break;
      case TEMPLATE_ACQUISITION_MODULE    :  return (char*) "TEMPLATE MODULE"; break;
      case NETWORK_ACQUISITION_MODULE    :  return (char*) "NETWORK MODULE"; break;
    };
    return (char*) "UNKNOWN MODULE";
}





void printCall(ModuleIdentifier moduleID,DeviceIdentifier devID,char * fromFunction , char * file , int line)
{
   #if PRINT_DEBUG_EACH_CALL
    fprintf(stderr,"called %s module %u , device %u  , file %s , line %d ..\n",fromFunction,moduleID,devID,file,line);
   #endif
}



void MeaningfullWarningMessage(ModuleIdentifier moduleFailed,DeviceIdentifier devFailed,char * fromFunction)
{
  if (!acquisitionIsModuleAvailiable(moduleFailed))
   {
       fprintf(stderr,"%s is not linked in this build of the acquisition library system..\n",getModuleNameFromModuleID(moduleFailed));
       return ;
   }

   fprintf(stderr,"%s hasn't got an implementation for function %s ..\n",getModuleNameFromModuleID(moduleFailed),fromFunction);
}




/*! ------------------------------------------
    BASIC START STOP MECHANISMS FOR MODULES..
   ------------------------------------------*/


int acquisitionPluginIsLoaded(ModuleIdentifier moduleID)
{
 return isPluginLoaded(moduleID);
}

int acquisitionLoadPlugin(ModuleIdentifier moduleID)
{
  return   linkToPlugin(getPluginStr(moduleID,PLUGIN_NAME_STR),
                       getPluginStr(moduleID,PLUGIN_PATH_STR),
                       getPluginStr(moduleID,PLUGIN_LIBNAME_STR),
                       moduleID);
}

int acquisitionUnloadPlugin(ModuleIdentifier moduleID)
{
  return  unlinkPlugin(moduleID);
}

int acquisitionStartModule(ModuleIdentifier moduleID,unsigned int maxDevices,char * settings)
{
  if (moduleID < NUMBER_OF_POSSIBLE_MODULES)
  {
    if (!acquisitionLoadPlugin(moduleID))
        { fprintf(stderr,RED "Could not find %s plugin shared object \n" NORMAL,getModuleNameFromModuleID(moduleID)); return 0; }

    if (*plugins[moduleID].startModule!=0) { return (*plugins[moduleID].startModule) (maxDevices,settings); }
  }

    MeaningfullWarningMessage(moduleID,0,"acquisitionStartModule");
    return 0;
}


int acquisitionStopModule(ModuleIdentifier moduleID)
{
    if (*plugins[moduleID].stopModule!=0) { return (*plugins[moduleID].stopModule) (); }
    acquisitionUnloadPlugin(moduleID);

    MeaningfullWarningMessage(moduleID,0,"acquisitionStopModule");
    return 0;
}


int acquisitionGetModuleDevices(ModuleIdentifier moduleID)
{
    printCall(moduleID,0,"acquisitionGetModuleDevices", __FILE__, __LINE__);
    if (plugins[moduleID].getNumberOfDevices!=0) { return (*plugins[moduleID].getNumberOfDevices) (); }
    MeaningfullWarningMessage(moduleID,0,"acquisitionGetModuleDevices");
    return 0;
}



int acquisitionMayBeVirtualDevice(ModuleIdentifier moduleID,DeviceIdentifier devID , char * devName)
{
  if ( (moduleID==OPENNI2_ACQUISITION_MODULE) && (acquisitionFileExists(devName)) )
      {
        fprintf(stderr,"Hoping that %s is a valid virtual device\n",devName);
       return 1;
      }

  return 0;
}

/*! ------------------------------------------
    FRAME SNAPPING MECHANISMS FOR MODULES..
   ------------------------------------------*/


int acquisitionListDevices(ModuleIdentifier moduleID,DeviceIdentifier devID,char * output, unsigned int maxOutput)
{
    printCall(moduleID,devID,"acquisitionListDevices", __FILE__, __LINE__);
    if (plugins[moduleID].listDevices!=0) { return (*plugins[moduleID].listDevices) (devID,output,maxOutput); }
    MeaningfullWarningMessage(moduleID,devID,"acquisitionListDevices");
    return 0;
}


int acquisitionChangeResolution(ModuleIdentifier moduleID,DeviceIdentifier devID,unsigned int width,unsigned int height)
{
  printCall(moduleID,devID,"acquisitionChangeResolution", __FILE__, __LINE__);
  if (plugins[moduleID].changeResolution!=0) { return (*plugins[moduleID].changeResolution) (width,height); }

  plugins[moduleID].forcedWidth=width;
  plugins[moduleID].forcedHeight=height;
  plugins[moduleID].forceResolution=1;
  return 1;
}


int acquisitionOpenDevice(ModuleIdentifier moduleID,DeviceIdentifier devID,char * devName,unsigned int width,unsigned int height,unsigned int framerate)
{
    printCall(moduleID,devID,"acquisitionOpenDevice", __FILE__, __LINE__);
    if (plugins[moduleID].createDevice!=0) { return (*plugins[moduleID].createDevice) (devID,devName,width,height,framerate); }
    MeaningfullWarningMessage(moduleID,devID,"acquisitionOpenDevice");
    return 0;
}


 int acquisitionCloseDevice(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
    acquisitionCleanOverrides(moduleID,devID);
    printCall(moduleID,devID,"acquisitionCloseDevice", __FILE__, __LINE__);
    if (plugins[moduleID].destroyDevice!=0) { return (*plugins[moduleID].destroyDevice) (devID); }
    MeaningfullWarningMessage(moduleID,devID,"acquisitionCloseDevice");
    return 0;
}


int acquisitionGetTotalFrameNumber(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
  printCall(moduleID,devID,"acquisitionGetTotalFrameNumber", __FILE__, __LINE__);
  if (*plugins[moduleID].getTotalFrameNumber!=0) { return (*plugins[moduleID].getTotalFrameNumber) (devID); }

  MeaningfullWarningMessage(moduleID,devID,"acquisitionGetTotalFrameNumber");
  return 0;
}


int acquisitionPrepareDifferentResolutionFrames(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
    if ( plugins[moduleID].forceResolution )
    {
      fprintf(stderr,"TODO :");
      return 1;
    }

  return 0;
}

int acquisitionGetCurrentFrameNumber(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
  printCall(moduleID,devID,"acquisitionGetCurrentFrameNumber", __FILE__, __LINE__);
  if (*plugins[moduleID].getCurrentFrameNumber!=0) { return (*plugins[moduleID].getCurrentFrameNumber) (devID); }

  MeaningfullWarningMessage(moduleID,devID,"acquisitionGetCurrentFrameNumber");
  return 0;
}



 int acquisitionSeekFrame(ModuleIdentifier moduleID,DeviceIdentifier devID,unsigned int seekFrame)
{
    printCall(moduleID,devID,"acquisitionSeekFrame", __FILE__, __LINE__);
    if (*plugins[moduleID].seekFrame!=0) { return (*plugins[moduleID].seekFrame) (devID,seekFrame); }

    MeaningfullWarningMessage(moduleID,devID,"acquisitionSeekFrame");
    return 0;
}


 int acquisitionSeekRelativeFrame(ModuleIdentifier moduleID,DeviceIdentifier devID,signed int seekFrame)
{
    printCall(moduleID,devID,"acquisitionSeekRelativeFrame", __FILE__, __LINE__);
    if (*plugins[moduleID].seekRelativeFrame!=0) { return (*plugins[moduleID].seekRelativeFrame) (devID,seekFrame); }

    MeaningfullWarningMessage(moduleID,devID,"acquisitionSeekRelativeFrame");
    return 0;
}

 int acquisitionSnapFrames(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
    printCall(moduleID,devID,"acquisitionSnapFrames", __FILE__, __LINE__);

    //In case the last frame had some overrides we "clear" them on our new frame so our new frame will not also appear overrided
    acquisitionCleanOverrides(moduleID,devID);

    StartTimer(FRAME_SNAP_DELAY);

    if (*plugins[moduleID].snapFrames!=0)
    {
      EndTimer(FRAME_SNAP_DELAY);
      return (*plugins[moduleID].snapFrames) (devID);
    }

    EndTimer(FRAME_SNAP_DELAY);
    MeaningfullWarningMessage(moduleID,devID,"acquisitionSnapFrames");
    return 0;
}

 int acquisitionSaveColorFrame(ModuleIdentifier moduleID,DeviceIdentifier devID,char * filename)
{
    printCall(moduleID,devID,"acquisitionSaveColorFrame", __FILE__, __LINE__);
    char filenameFull[2048]={0};
    sprintf(filenameFull,"%s.pnm",filename);



         if (
              (*plugins[moduleID].getColorPixels!=0) && (*plugins[moduleID].getColorWidth!=0) && (*plugins[moduleID].getColorHeight!=0) &&
              (*plugins[moduleID].getColorChannels!=0) && (*plugins[moduleID].getColorBitsPerPixel!=0)
            )
         {
            int retres = acquisitionSaveRawImageToFile(
                                            filenameFull,
                                            // (*plugins[moduleID].getColorPixels)      (devID),
                                            acquisitionGetColorFrame(moduleID,devID)        ,
                                            (*plugins[moduleID].getColorWidth)       (devID),
                                            (*plugins[moduleID].getColorHeight)      (devID),
                                            (*plugins[moduleID].getColorChannels)    (devID),
                                            (*plugins[moduleID].getColorBitsPerPixel)(devID)
                                           );


           // V4L2 Specific JPS compression ------------------------------------------------------------------------------------------------------------------------------
           if ( (retres) && (moduleID==V4L2STEREO_ACQUISITION_MODULE) )
              {  //V4L2Stereo images are huge so until we fix jpeg compression for all templates ( already there but there are some managment decisions to be made :P )
                 //we do a simple hack here :p
                 char convertToJPEGString[4096]={0};
                 sprintf(convertToJPEGString , "convert %s.pnm %s.jpg && rm  %s.pnm && mv %s.jpg %s.jps",filename,filename,filename,filename,filename);
                 int i = system(convertToJPEGString);
                 if (i==0) { fprintf(stderr,"Success converting to jpeg\n"); } else
                           { fprintf(stderr,"Failure converting to jpeg\n"); }
              }
           // V4L2 Specific JPS compression ------------------------------------------------------------------------------------------------------------------------------

            return retres;
         }


    MeaningfullWarningMessage(moduleID,devID,"acquisitionSaveColorFrame");
    return 0;
}



 int acquisitionSavePCDPointCoud(ModuleIdentifier moduleID,DeviceIdentifier devID,char * filename)
{
  unsigned int width;
  unsigned int height;
  unsigned int channels;
  unsigned int bitsperpixel;
  acquisitionGetDepthFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel);

  return savePCD_PointCloud(filename,acquisitionGetDepthFrame(moduleID,devID),acquisitionGetColorFrame(moduleID,devID),
                            width,height,width/2,height/2, 1.0 /*DUMMY fx*/, 1.0 /*DUMMY fy*/ );
}



 int acquisitionSaveDepthFrame(ModuleIdentifier moduleID,DeviceIdentifier devID,char * filename)
{
    printCall(moduleID,devID,"acquisitionSaveDepthFrame", __FILE__, __LINE__);
    char filenameFull[2048]={0};
    sprintf(filenameFull,"%s.pnm",filename);


          if (
              (*plugins[moduleID].getDepthPixels!=0) && (*plugins[moduleID].getDepthWidth!=0) && (*plugins[moduleID].getDepthHeight!=0) &&
              (*plugins[moduleID].getDepthChannels!=0) && (*plugins[moduleID].getDepthBitsPerPixel!=0)
             )
         {
            return acquisitionSaveRawImageToFile(
                                      filenameFull,
                                      // (*plugins[moduleID].getDepthPixels)      (devID),
                                      (unsigned char*) acquisitionGetDepthFrame(moduleID,devID)  ,
                                      (*plugins[moduleID].getDepthWidth)       (devID),
                                      (*plugins[moduleID].getDepthHeight)      (devID),
                                      (*plugins[moduleID].getDepthChannels)    (devID),
                                      (*plugins[moduleID].getDepthBitsPerPixel)(devID)
                                     );
         }

    MeaningfullWarningMessage(moduleID,devID,"acquisitionSaveDepthFrame");
    return 0;
}

 int acquisitionSaveColoredDepthFrame(ModuleIdentifier moduleID,DeviceIdentifier devID,char * filename)
{
    printCall(moduleID,devID,"acquisitionSaveColoredDepthFrame", __FILE__, __LINE__);

    char filenameFull[1024]={0};
    sprintf(filenameFull,"%s.pnm",filename);

    unsigned int width = 0 ;
    unsigned int height = 0 ;
    unsigned int channels = 0 ;
    unsigned int bitsperpixel = 0 ;
    unsigned short * inFrame = 0;
    unsigned char * outFrame = 0 ;

    inFrame = acquisitionGetDepthFrame(moduleID,devID);
    if (inFrame!=0)
      {
       acquisitionGetDepthFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel);
       outFrame = convertShortDepthToRGBDepth(inFrame,width,height);
       if (outFrame!=0)
        {
         acquisitionSaveRawImageToFile(filenameFull,outFrame,width,height,3,8);
         free(outFrame);
         return 1;
        }
      }

    MeaningfullWarningMessage(moduleID,devID,"acquisitionSaveColoredDepthFrame");
    return 0;
}


int acquisitionSaveDepthFrame1C(ModuleIdentifier moduleID,DeviceIdentifier devID,char * filename)
{
    printCall(moduleID,devID,"acquisitionSaveColoredDepthFrame", __FILE__, __LINE__);

    char filenameFull[1024]={0};
    sprintf(filenameFull,"%s.pnm",filename);

    unsigned int width = 0 ;
    unsigned int height = 0 ;
    unsigned int channels = 0 ;
    unsigned int bitsperpixel = 0 ;
    unsigned short * inFrame = 0;
    unsigned char * outFrame = 0 ;

    inFrame = acquisitionGetDepthFrame(moduleID,devID);
    if (inFrame!=0)
      {
       acquisitionGetDepthFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel);
       outFrame = convertShortDepthToCharDepth(inFrame,width,height,0,7000);
       if (outFrame!=0)
        {
         acquisitionSaveRawImageToFile(filenameFull,outFrame,width,height,1,8);
         free(outFrame);
         return 1;
        }
      }

    MeaningfullWarningMessage(moduleID,devID,"acquisitionSaveColoredDepthFrame");
    return 0;
}



int acquisitionGetColorCalibration(ModuleIdentifier moduleID,DeviceIdentifier devID,struct calibration * calib)
{
   printCall(moduleID,devID,"acquisitionGetColorCalibration", __FILE__, __LINE__);
   if (*plugins[moduleID].getColorCalibration!=0) { return (*plugins[moduleID].getColorCalibration) (devID,calib); }
   MeaningfullWarningMessage(moduleID,devID,"acquisitionGetColorCalibration");
   return 0;
}

int acquisitionGetDepthCalibration(ModuleIdentifier moduleID,DeviceIdentifier devID,struct calibration * calib)
{
   printCall(moduleID,devID,"acquisitionGetDepthCalibration", __FILE__, __LINE__);
   if (*plugins[moduleID].getDepthCalibration!=0) { return (*plugins[moduleID].getDepthCalibration) (devID,calib); }
   MeaningfullWarningMessage(moduleID,devID,"acquisitionGetDepthCalibration");
   return 0;
}





int acquisitionSetColorCalibration(ModuleIdentifier moduleID,DeviceIdentifier devID,struct calibration * calib)
{
   printCall(moduleID,devID,"acquisitionGetColorCalibration", __FILE__, __LINE__);
   if (*plugins[moduleID].setColorCalibration!=0) { return (*plugins[moduleID].setColorCalibration) (devID,calib); }
   MeaningfullWarningMessage(moduleID,devID,"acquisitionGetColorCalibration");
   return 0;
}

int acquisitionSetDepthCalibration(ModuleIdentifier moduleID,DeviceIdentifier devID,struct calibration * calib)
{
   printCall(moduleID,devID,"acquisitionGetDepthCalibration", __FILE__, __LINE__);
   if (*plugins[moduleID].setDepthCalibration!=0) { return (*plugins[moduleID].setDepthCalibration) (devID,calib); }
   MeaningfullWarningMessage(moduleID,devID,"acquisitionGetDepthCalibration");
   return 0;
}


unsigned long acquisitionGetColorTimestamp(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
   printCall(moduleID,devID,"acquisitionGetColorTimestamp", __FILE__, __LINE__);
   if (*plugins[moduleID].getLastColorTimestamp!=0) { return (*plugins[moduleID].getLastColorTimestamp) (devID); }
   MeaningfullWarningMessage(moduleID,devID,"acquisitionGetColorTimestamp");
   return 0;
}

unsigned long acquisitionGetDepthTimestamp(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
   printCall(moduleID,devID,"acquisitionGetDepthTimestamp", __FILE__, __LINE__);
   if (*plugins[moduleID].getLastDepthTimestamp!=0) { return (*plugins[moduleID].getLastDepthTimestamp) (devID); }
   MeaningfullWarningMessage(moduleID,devID,"acquisitionGetDepthTimestamp");
   return 0;
}


int acquisitionOverrideColorFrame(ModuleIdentifier moduleID , DeviceIdentifier devID , unsigned char * newColor , unsigned int newColorByteSize)
{
  if (newColor==0) { fprintf(stderr,"acquisitionOverrideColorFrame called with null new buffer");  return 0; }
  //Ok when someone overrides a color frame there are two chances..
  //He either has already done it ( there is already an overriden frame ) , Or it is the first time he does it ( there is NO overriden frame existing )
  //Since allocating/freeing chunks of memory means syscalls and since most of the time the size of each frame is the same in case there is an already
  //Overriden frame we just use the space allocated for it in order to be economic
  if ( module[moduleID].device[devID].overrideColorFrame!=0 )
  {
     fprintf(stderr,"Override Color Frame called while already overriden!\n");
     if ( module[moduleID].device[devID].overrideColorFrameByteSize < newColorByteSize )
     {
      fprintf(stderr,"Old override frame is not big enough to fit our frame , will not use it!\n");
      free(module[moduleID].device[devID].overrideColorFrame);
      module[moduleID].device[devID].overrideColorFrame=0;
      module[moduleID].device[devID].overrideColorFrameByteSize  = 0;
     } else
     {
      fprintf(stderr,"Old override frame is big enough , we will just use it instead of doing more syscalls to achieve the same result!\n");
     }
  }

  //In case the frame is not allocated ( or it was but it was not big enough ) , we have to malloc a new memory chunk to accomodate our frame
  if ( module[moduleID].device[devID].overrideColorFrame==0 )
  {
      module[moduleID].device[devID].overrideColorFrame = ( unsigned char * ) malloc(newColorByteSize*sizeof(unsigned char));
      if (module[moduleID].device[devID].overrideColorFrame==0)
      {
        module[moduleID].device[devID].overrideColorFrameByteSize  = 0;
        fprintf(stderr,"Could not allocate a new override frame of size %u\n",newColorByteSize);
        return 0;
      }
      module[moduleID].device[devID].overrideColorFrameByteSize  = newColorByteSize;
  }

  //Ok so after everything if we enter here it means we can <<at-last>> do our memcpy and get this over with
  if ( module[moduleID].device[devID].overrideColorFrame!=0 )
  {
      memcpy(module[moduleID].device[devID].overrideColorFrame , newColor , newColorByteSize);
      return 1;
  }

  return 0;
}






int acquisitionGetNumberOfColorStreams(ModuleIdentifier moduleID , DeviceIdentifier devID)
{
  printCall(moduleID,devID,"acquisitionGetNumberOfColorStreams", __FILE__, __LINE__);
  //If getNumberOfColorStreams is declared for the plugin return its values
  if (*plugins[moduleID].getNumberOfColorStreams!=0) { return (*plugins[moduleID].getNumberOfColorStreams) (devID); } else
  //If we dont find a declared getNumberOfColorStreams BUT the getColorPixels is declared it looks like a missing function on the plugin
  //So we return 1 device ( the default
  if (*plugins[moduleID].getColorPixels!=0) { return 1; }

  //If we don't find it return 0
  MeaningfullWarningMessage(moduleID,devID,"acquisitionGetNumberOfColorStreams");
  return 0;
}



int acquisitionSwitchToColorStream(ModuleIdentifier moduleID , DeviceIdentifier devID , unsigned int streamToActivate)
{
  printCall(moduleID,devID,"acquisitionSwitchToColorStream", __FILE__, __LINE__);
  if (*plugins[moduleID].switchToColorStream!=0) { return (*plugins[moduleID].switchToColorStream) (devID,streamToActivate); }
  MeaningfullWarningMessage(moduleID,devID,"acquisitionSwitchToColorStream");
  return 0;
}







unsigned char * acquisitionGetColorFrame(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
  printCall(moduleID,devID,"acquisitionGetColorFrame", __FILE__, __LINE__);

  if (module[moduleID].device[devID].overrideColorFrame!=0) {
                                                              fprintf(stderr,"Returning overriden color frame \n");
                                                              return module[moduleID].device[devID].overrideColorFrame;
                                                            }

  if (*plugins[moduleID].getColorPixels!=0) { return (*plugins[moduleID].getColorPixels) (devID); }
  MeaningfullWarningMessage(moduleID,devID,"acquisitionGetColorFrame");
  return 0;
}

unsigned int acquisitionCopyColorFrame(ModuleIdentifier moduleID,DeviceIdentifier devID,unsigned char * mem,unsigned int memlength)
{
  printCall(moduleID,devID,"acquisitionCopyColorFrame", __FILE__, __LINE__);
  if ( (mem==0) || (memlength==0) )
  {
    fprintf(stderr,RED "acquisitionCopyColorFrame called with incorrect target for memcpy, %u bytes size" NORMAL,memlength);
    return 0;
  }


  unsigned char * color = acquisitionGetColorFrame(moduleID,devID);
  if (color==0) { return 0; }
  unsigned int width , height , channels , bitsperpixel;
  acquisitionGetColorFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel);
  unsigned int copySize = width*height*channels*(bitsperpixel/8);
  memcpy(mem,color,copySize);
  return copySize;
}


unsigned int acquisitionCopyColorFramePPM(ModuleIdentifier moduleID,DeviceIdentifier devID,unsigned char * mem,unsigned int memlength)
{
  printCall(moduleID,devID,"acquisitionCopyColorFramePPM", __FILE__, __LINE__);
  if ( (mem==0) || (memlength==0) )
  {
    fprintf(stderr,RED "acquisitionCopyColorFramePPM called with incorrect target for memcpy, %u bytes size" NORMAL,memlength);
    return 0;
  }

  char * memC = (char*) mem;

  unsigned char * color = acquisitionGetColorFrame(moduleID,devID);
  if (color==0) { return 0; }
  unsigned int width , height , channels , bitsperpixel;
  acquisitionGetColorFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel);

  sprintf(memC, "P6%d %d\n%u\n", width, height , simplePow(2 ,bitsperpixel)-1);
  unsigned int payloadStart = strlen(memC);

  unsigned char * memPayload = mem + payloadStart ;
  memcpy(memPayload,color,width*height*channels*(bitsperpixel/8));

  payloadStart += width*height*channels*(bitsperpixel/8);
  return payloadStart;
}



int acquisitionOverrideDepthFrame(ModuleIdentifier moduleID , DeviceIdentifier devID , unsigned short * newDepth , unsigned int newDepthByteSize)
{
  if (newDepth==0) { fprintf(stderr,"acquisitionOverrideDepthFrame called with null new buffer");  return 0; }
  //Ok when someone overrides a color frame there are two chances..
  //He either has already done it ( there is already an overriden frame ) , Or it is the first time he does it ( there is NO overriden frame existing )
  //Since allocating/freeing chunks of memory means syscalls and since most of the time the size of each frame is the same in case there is an already
  //Overriden frame we just use the space allocated for it in order to be economic
  if ( module[moduleID].device[devID].overrideDepthFrame!=0 )
  {
     fprintf(stderr,"Override Color Frame called while already overriden!\n");
     if ( module[moduleID].device[devID].overrideDepthFrameByteSize < newDepthByteSize )
     {
      fprintf(stderr,"Old override frame is not big enough to fit our frame , will not use it!\n");
      free(module[moduleID].device[devID].overrideDepthFrame);
      module[moduleID].device[devID].overrideDepthFrame=0;
      module[moduleID].device[devID].overrideDepthFrameByteSize  = 0;
     } else
     {
      fprintf(stderr,"Old override frame is big enough , we will just use it instead of doing more syscalls to achieve the same result!\n");
     }
  }

  //In case the frame is not allocated ( or it was but it was not big enough ) , we have to malloc a new memory chunk to accomodate our frame
  if ( module[moduleID].device[devID].overrideDepthFrame==0 )
  {
      module[moduleID].device[devID].overrideDepthFrame = ( unsigned char * ) malloc(newDepthByteSize /* *sizeof(unsigned short) */ );
      if (module[moduleID].device[devID].overrideDepthFrame==0)
      {
        module[moduleID].device[devID].overrideDepthFrameByteSize  = 0;
        fprintf(stderr,"Could not allocate a new override frame of size %u\n",newDepthByteSize);
        return 0;
      }
      module[moduleID].device[devID].overrideDepthFrameByteSize  = newDepthByteSize;
  }

  //Ok so after everything if we enter here it means we can <<at-last>> do our memcpy and get this over with
  if ( module[moduleID].device[devID].overrideDepthFrame!=0 )
  {
      memcpy(module[moduleID].device[devID].overrideDepthFrame , newDepth , newDepthByteSize);
      return 1;
  }

  return 0;
}


unsigned short * acquisitionGetDepthFrame(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
  printCall(moduleID,devID,"acquisitionGetDepthFrame", __FILE__, __LINE__);

  if (module[moduleID].device[devID].overrideDepthFrame!=0) {
                                                              fprintf(stderr,"Returning overriden depth frame \n");
                                                              return module[moduleID].device[devID].overrideDepthFrame;
                                                            }

  if (*plugins[moduleID].getDepthPixels!=0) { return (unsigned short*) (*plugins[moduleID].getDepthPixels) (devID); }
  MeaningfullWarningMessage(moduleID,devID,"acquisitionGetDepthFrame");
  return 0;
}


unsigned int acquisitionCopyDepthFrame(ModuleIdentifier moduleID,DeviceIdentifier devID,unsigned short * mem,unsigned int memlength)
{
  printCall(moduleID,devID,"acquisitionCopyDepthFrame", __FILE__, __LINE__);
  if ( (mem==0) || (memlength==0) )
  {
    fprintf(stderr,RED "acquisitionCopyDepthFrame called with incorrect target for memcpy , %u bytes size" NORMAL,memlength);
    return 0;
  }

  unsigned short * depth = acquisitionGetDepthFrame(moduleID,devID);
  if (depth==0) { return 0; }
  unsigned int width , height , channels , bitsperpixel;
  acquisitionGetDepthFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel);
  unsigned int copySize = width*height*channels*(bitsperpixel/8);
  memcpy(mem,depth,copySize);
  return copySize;
}


unsigned int acquisitionCopyDepthFramePPM(ModuleIdentifier moduleID,DeviceIdentifier devID,unsigned short * mem,unsigned int memlength)
{
  printCall(moduleID,devID,"acquisitionCopyDepthFramePPM", __FILE__, __LINE__);
  if ( (mem==0) || (memlength==0) )
  {
    fprintf(stderr,RED "acquisitionCopyDepthFramePPM called with incorrect target for memcpy , %u bytes size" NORMAL,memlength);
    return 0;
  }

  unsigned short * depth = acquisitionGetDepthFrame(moduleID,devID);
  if (depth==0) { return 0; }

  unsigned int width , height , channels , bitsperpixel;
  acquisitionGetDepthFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel);

  sprintf((char*) mem, "P5%d %d\n%u\n", width, height , simplePow(2 ,bitsperpixel)-1);
  unsigned int payloadStart = strlen((char*) mem);

  unsigned short * memPayload = mem + payloadStart ;
  memcpy(memPayload,depth,width*height*channels*(bitsperpixel/8));

  payloadStart += width*height*channels*(bitsperpixel/8);
  return payloadStart;
}



int acquisitionGetColorRGBAtXY(ModuleIdentifier moduleID,DeviceIdentifier devID,unsigned int x2d, unsigned int y2d , unsigned char * R ,unsigned char * G , unsigned char * B )
{
    unsigned char * colorFrame = acquisitionGetColorFrame(moduleID,devID);
    if (colorFrame == 0 ) { MeaningfullWarningMessage(moduleID,devID,"acquisitionGetColorRGBAtXY , getting color frame"); return 0; }

    unsigned int width; unsigned int height; unsigned int channels; unsigned int bitsperpixel;
    if (! acquisitionGetColorFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel) )
        {  MeaningfullWarningMessage(moduleID,devID,"acquisitionGetColorRGBAtXY getting depth frame dims"); return 0; }

    if ( (x2d>=width) || (y2d>=height) )
        { MeaningfullWarningMessage(moduleID,devID,"acquisitionGetColorRGBAtXY incorrect 2d x,y coords"); return 0; }


    unsigned char * colorValue = colorFrame + ( (y2d * width *  channels)  + (x2d * channels ) );
    *R = *colorValue; ++colorValue;
    *G = *colorValue; ++colorValue;
    *B = *colorValue;

    return 1;
}



unsigned short acquisitionGetDepthValueAtXY(ModuleIdentifier moduleID,DeviceIdentifier devID,unsigned int x2d, unsigned int y2d )
{
    unsigned short * depthFrame = acquisitionGetDepthFrame(moduleID,devID);
    if (depthFrame == 0 ) { MeaningfullWarningMessage(moduleID,devID,"acquisitionGetDepthValueAtXY , getting depth frame"); return 0; }

    unsigned int width; unsigned int height; unsigned int channels; unsigned int bitsperpixel;
    if (! acquisitionGetDepthFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel) )
        {  MeaningfullWarningMessage(moduleID,devID,"acquisitionGetDepthValueAtXY getting depth frame dims"); return 0; }

    if ( (x2d>=width) || (y2d>=height) )
        { MeaningfullWarningMessage(moduleID,devID,"acquisitionGetDepthValueAtXY incorrect 2d x,y coords"); return 0; }


    unsigned short * depthValue = depthFrame + (y2d * width + x2d );
    unsigned short result = * depthValue;

    return result;
}


int acquisitionGetDepth3DPointAtXYCameraSpace(ModuleIdentifier moduleID,DeviceIdentifier devID,unsigned int x2d, unsigned int y2d , float *x, float *y , float *z  )
{
    struct calibration calib;
    unsigned short depthValue = acquisitionGetDepthValueAtXY(moduleID,devID,x2d,y2d);
    if (depthValue==0) { fprintf(stderr,"acquisitionGetDepth3DPointAtXYCameraSpace point %u,%u has no depth\n",x2d,y2d); return 0; }
    if ( !acquisitionGetDepthCalibration(moduleID,devID,&calib) )
        {
          if ( !acquisitionGetColorCalibration(moduleID,devID,&calib) )
          { fprintf(stderr,"Could not get Depth Calibration , cannot get 3D point\n"); return 0; }
        }

    return transform2DProjectedPointTo3DPoint(&calib , x2d , y2d  , depthValue , x , y , z);
}


int acquisitionGetDepth3DPointAtXY(ModuleIdentifier moduleID,DeviceIdentifier devID,unsigned int x2d, unsigned int y2d , float *x, float *y , float *z  )
{
    struct calibration calib;
    unsigned short depthValue = acquisitionGetDepthValueAtXY(moduleID,devID,x2d,y2d);
    if (depthValue==0) { fprintf(stderr,"acquisitionGetDepth3DPointAtXY point %u,%u has no depth\n",x2d,y2d); return 0; }
    if ( !acquisitionGetDepthCalibration(moduleID,devID,&calib) )
        {
          if ( !acquisitionGetColorCalibration(moduleID,devID,&calib) )
          { fprintf(stderr,"Could not get Depth Calibration , cannot get 3D point\n"); return 0; }
        }

    transform2DProjectedPointTo3DPoint(&calib , x2d , y2d  , depthValue , x , y , z);

    if (calib.extrinsicParametersSet)
    {
      return transform3DPointUsingCalibration(&calib , x , y , z);
    }
    return 1;
}

int acquisitionGetColorFrameDimensions(ModuleIdentifier moduleID,DeviceIdentifier devID ,
                                       unsigned int * width , unsigned int * height , unsigned int * channels , unsigned int * bitsperpixel )
{
  printCall(moduleID,devID,"acquisitionGetColorFrameDimensions", __FILE__, __LINE__);

  if ( (width==0)||(height==0)||(channels==0)||(bitsperpixel==0) )
    {
        fprintf(stderr,"acquisitionGetColorFrameDimensions called with invalid arguments .. \n");
        return 0;
    }


         if (
              (*plugins[moduleID].getColorWidth!=0) && (*plugins[moduleID].getColorHeight!=0) &&
              (*plugins[moduleID].getColorChannels!=0) && (*plugins[moduleID].getColorBitsPerPixel!=0)
            )
            {
              *width        = (*plugins[moduleID].getColorWidth)        (devID);
              *height       = (*plugins[moduleID].getColorHeight)       (devID);
              *channels     = (*plugins[moduleID].getColorChannels)     (devID);
              *bitsperpixel = (*plugins[moduleID].getColorBitsPerPixel) (devID);
              return 1;
            }

  MeaningfullWarningMessage(moduleID,devID,"acquisitionGetColorFrameDimensions");
  return 0;
}



int acquisitionGetDepthFrameDimensions(ModuleIdentifier moduleID,DeviceIdentifier devID ,
                                       unsigned int * width , unsigned int * height , unsigned int * channels , unsigned int * bitsperpixel )
{
  printCall(moduleID,devID,"acquisitionGetDepthFrameDimensions", __FILE__, __LINE__);

  if ( (width==0)||(height==0)||(channels==0)||(bitsperpixel==0) )
    {
        fprintf(stderr,"acquisitionGetDepthFrameDimensions called with invalid arguments .. \n");
        return 0;
    }


         if (
              (*plugins[moduleID].getDepthWidth!=0) && (*plugins[moduleID].getDepthHeight!=0) &&
              (*plugins[moduleID].getDepthChannels!=0) && (*plugins[moduleID].getDepthBitsPerPixel!=0)
            )
            {
              *width        = (*plugins[moduleID].getDepthWidth)        (devID);
              *height       = (*plugins[moduleID].getDepthHeight)       (devID);
              *channels     = (*plugins[moduleID].getDepthChannels)     (devID);
              *bitsperpixel = (*plugins[moduleID].getDepthBitsPerPixel) (devID);
              return 1;
            }
   MeaningfullWarningMessage(moduleID,devID,"acquisitionGetDepthFrameDimensions");
   return 0;
}


 int acquisitionMapDepthToRGB(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
    printCall(moduleID,devID,"acquisitionMapDepthToRGB", __FILE__, __LINE__);
    if  (*plugins[moduleID].mapDepthToRGB!=0) { return  (*plugins[moduleID].mapDepthToRGB) (devID); }
    MeaningfullWarningMessage(moduleID,devID,"acquisitionMapDepthToRGB");
    return 0;
}


 int acquisitionMapRGBToDepth(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
    printCall(moduleID,devID,"acquisitionMapRGBToDepth", __FILE__, __LINE__);
    if  (*plugins[moduleID].mapRGBToDepth!=0) { return  (*plugins[moduleID].mapRGBToDepth) (devID); }
    MeaningfullWarningMessage(moduleID,devID,"acquisitionMapRGBToDepth");
    return 0;
}


/*
   LAST BUT NOT LEAST acquisition can also relay its state through a TCP/IP network
*/
int acquisitionInitiateTargetForFrames(ModuleIdentifier moduleID,DeviceIdentifier devID,char * target)
{
  if (strstr(target,"/dev/null")!=0)
  {
    module[moduleID].device[devID].dryRunOutput=1;
    return 1;
  } else
  if ( strstr(target,"tcp://")!=0 )
  {
    if (
         !linkToNetworkTransmission(
                                     getPluginStr(NETWORK_ACQUISITION_MODULE,PLUGIN_NAME_STR) ,
                                     getPluginStr(NETWORK_ACQUISITION_MODULE,PLUGIN_PATH_STR)  ,
                                     getPluginStr(NETWORK_ACQUISITION_MODULE,PLUGIN_LIBNAME_STR)
                                   )
        )
      {
        fprintf(stderr,RED "Cannot link to network transmission framework , so will not be able to transmit output..!\n" NORMAL);
      } else
      {
       if  (*startPushingToRemoteNetwork!=0)
         {
            module[moduleID].device[devID].frameServerID = (*startPushingToRemoteNetwork) ("0.0.0.0",1234);
            module[moduleID].device[devID].networkOutput=1;
            return 1;
         }
      }
  } else
  {
    module[moduleID].device[devID].fileOutput=1;
    strcpy(module[moduleID].device[devID].outputString , target);

    //fprintf(stderr,"acquisitionInitiateTargetForFrames! Module %u , Device %u = %s \n",moduleID,devID, module[moduleID].device[devID].outputString);
    //Prepare path
    makepath(target);
    return 1;
  }

  fprintf(stderr,RED "acquisitionInitiateTargetForFrames did not decide on a method for passing frames to target\n" NORMAL);
  return 0;
}


int acquisitionStopTargetForFrames(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
  if (module[moduleID].device[devID].networkOutput)
  {
    //If we were using networkOutput lets stop using it!
    if (stopPushingToRemoteNetwork==0) { fprintf(stderr,RED "stopPushingToRemoteNetwork has not been linked to network plugin\n" NORMAL); return 0; }
    return (*stopPushingToRemoteNetwork) (module[moduleID].device[devID].frameServerID);
  }
  return 1;
}



int acquisitionPassFramesToTarget(ModuleIdentifier moduleID,DeviceIdentifier devID,unsigned int frameNumber)
{
  //fprintf(stderr,"acquisitionPassFramesToTarget not fully implemented yet! Module %u , Device %u = %s \n",moduleID,devID, module[moduleID].device[devID].outputString);

  if (module[moduleID].device[devID].dryRunOutput)
  {
    if (!warnDryRun)
     { fprintf(stderr,GREEN "Grabber Running on Dry Run mode , grabbing frames and doing nothing [ This message appears only one time ]\n" NORMAL); warnDryRun=1; }

    //Lets access the frames like we want to use them but not use them..

    StartTimer(FRAME_PASS_TO_TARGET_DELAY);
    unsigned int width, height , channels , bitsperpixel;
    acquisitionGetColorFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel);
    acquisitionGetColorFrame(moduleID,devID);

    acquisitionGetDepthFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel);
    acquisitionGetDepthFrame(moduleID,devID);
    //Done doing nothing with our input..!
    EndTimer(FRAME_PASS_TO_TARGET_DELAY);
  } else
  if (module[moduleID].device[devID].fileOutput)
  {
   StartTimer(FRAME_PASS_TO_TARGET_DELAY);
   char outfilename[2048]={0};
   sprintf(outfilename,"%s/colorFrame_%u_%05u",module[moduleID].device[devID].outputString,devID,frameNumber);
   acquisitionSaveColorFrame(moduleID,devID,outfilename);

   sprintf(outfilename,"%s/depthFrame_%u_%05u",module[moduleID].device[devID].outputString,devID,frameNumber);
   acquisitionSaveDepthFrame(moduleID,devID,outfilename);
   EndTimer(FRAME_PASS_TO_TARGET_DELAY);

  } else
  if (module[moduleID].device[devID].networkOutput)
  {
    StartTimer(FRAME_PASS_TO_TARGET_DELAY);

    unsigned int width, height , channels , bitsperpixel;
    acquisitionGetColorFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel);
    pushImageToRemoteNetwork(module[moduleID].device[devID].frameServerID , 0 , (void*) acquisitionGetColorFrame(moduleID,devID) , width , height , channels , bitsperpixel);

    acquisitionGetDepthFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel);
    pushImageToRemoteNetwork(module[moduleID].device[devID].frameServerID , 1 , (void*) acquisitionGetDepthFrame(moduleID,devID) , width , height , channels , bitsperpixel);

    EndTimer(FRAME_PASS_TO_TARGET_DELAY);
  } else
  {
    fprintf(stderr,RED "acquisitionPassFramesToTarget cannot find a method to use for module %u , device %u , has acquisitionInitiateTargetForFrames been called?\n" NORMAL , moduleID , devID );
    return 0;
  }

  return 1;
}



