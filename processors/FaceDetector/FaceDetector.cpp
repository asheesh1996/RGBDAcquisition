#include <iostream>
#include "FaceDetector.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#define NORMAL   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */

using namespace std;

unsigned char * colorFrame = 0;
unsigned int colorWidth=0,colorHeight=0,colorChannels=0,colorBitsperpixel=0;

unsigned short * depthFrame = 0;
unsigned int depthWidth=0,depthHeight=0,depthChannels=0,depthBitsperpixel=0;

#define DISPARITYMAPPING_STRING_SIZE 512
unsigned int disparityCalibrationUsed=0;
char disparityCalibrationPath[DISPARITYMAPPING_STRING_SIZE]={0};
char disparityCalibrationOutputPath[DISPARITYMAPPING_STRING_SIZE]={0};

cv::CascadeClassifier face_cascade;
int transmit= 0;


int initArgs_FaceDetector(int argc, char *argv[])
{
  fprintf(stderr,GREEN "\nFace Detector now parsing initialization parameters\n" NORMAL);
  fprintf(stderr,GREEN "_________________________________________________________________\n" NORMAL);

  int i=0;
  for (i=0; i<argc; i++)
  {
    if (strcmp(argv[i],"--transmit")==0) { transmit=1;}

  }

 // Load Face cascade (.xml file)
 face_cascade.load( "/usr/share/opencv/haarcascades/haarcascade_frontalface_alt2.xml" );
 //cv::namedWindow( "window1", 1 );


 fprintf(stderr,GREEN "_________________________________________________________________\n\n" NORMAL);
 return 1;
}



int setConfigStr_FaceDetector(char * label,char * value)
{
 return 0;
}

int setConfigInt_FaceDetector(char * label,int value)
{
return 0;
}



unsigned char * getDataOutput_FaceDetector(unsigned int stream , unsigned int * width, unsigned int * height,unsigned int * channels,unsigned int * bitsperpixel)
{
 return 0;
}



int addDataInput_FaceDetector(unsigned int stream , void * data, unsigned int width, unsigned int height,unsigned int channels,unsigned int bitsperpixel)
{
  if (stream==0)
  {
    unsigned int colorFrameSize = width*height*channels*(bitsperpixel/8);
    colorFrame = (unsigned char* ) malloc(colorFrameSize);
    if (colorFrame!=0)
    {
      memcpy(colorFrame,data,colorFrameSize);
      colorWidth=width; colorHeight=height;  colorChannels=channels; colorBitsperpixel=bitsperpixel;
    }
    return 1;
  } else
  if (stream==1)
  {
    unsigned int depthFrameSize = width*height*channels*(bitsperpixel/8);
    depthFrame = (unsigned short* ) malloc(depthFrameSize);
    if (depthFrame!=0)
    {
      memcpy(depthFrame,data,depthFrameSize);
      depthWidth=width; depthHeight=height;  depthChannels=channels; depthBitsperpixel=bitsperpixel;
    }
    return 1;
   }


 return 0;
}




unsigned short * getDepth_FaceDetector(unsigned int * width, unsigned int * height,unsigned int * channels,unsigned int * bitsperpixel)
{
    *width=depthWidth; *height=depthHeight; *channels=depthChannels;  *bitsperpixel=depthBitsperpixel;
    return depthFrame;
}


unsigned char * getColor_FaceDetector(unsigned int * width, unsigned int * height,unsigned int * channels,unsigned int * bitsperpixel)
{
    *width=colorWidth; *height=colorHeight; *channels=colorChannels;  *bitsperpixel=colorBitsperpixel;
    return colorFrame;
}


int transmitHeadPosition(float x,  float y , float z)
{
  char url[512]={0};
  fprintf(stderr,"transmitHeadPosition(%0.2f,%0.2f,%0.2f)\n",x,y,z);

  snprintf(url,512,"wget -qO- \"http://127.0.0.1:8080/control.html?x=%0.2f&y=%0.2f&z=%0.2f&qX=0&qY=0&qZ=0\" &> /dev/null ",x,y,z);
  int i=system(url);
 return (i==0);
}



int processData_FaceDetector()
{
    int retres=0;
    // Start and end times
    time_t startTime , endTime;
    time(&startTime);

    unsigned char * colorPTR = colorFrame ;


    cv::Mat colImNeeds_RGB_2_BGR_Flip(cv::Size(colorWidth,colorHeight), CV_8UC3, (char *) colorFrame, cv::Mat::AUTO_STEP);
    cv::Mat image;
    cv::cvtColor(colImNeeds_RGB_2_BGR_Flip, image, CV_RGB2BGR);
    cv::Mat depthIm(cv::Size(depthWidth,depthHeight), CV_16UC1, (char *) depthFrame , cv::Mat::AUTO_STEP);

  /////// imshow( "window1", image );

    // Detect faces
    std::vector<cv::Rect> faces;
    face_cascade.detectMultiScale( image, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, cv::Size(30, 30) );

    // Draw circles on the detected faces
    for( int i = 0; i < faces.size(); i++ )
    {
       if (faces[i].width>110)
       {
        cv::Point center( faces[i].x + faces[i].width*0.5, faces[i].y + faces[i].height*0.5 );
        cv::ellipse( image, center, cv::Size( faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, cv::Scalar( 255, 0, 255 ), 4, 8, 0 );

        if (transmit)
        {
         float x = (float) ( (float) colorWidth/2)  - faces[0].x;
         float y = (float) ( (float) colorHeight/2) - faces[0].y;
         float z = 0.0;
         transmitHeadPosition( (float) -x/20, (float) y/20 , z);
        }
       }
    }

   imshow( "Detected Face", image );


 time(&endTime);



 // Time elapsed
 double seconds = difftime (endTime, startTime);

 if (seconds == 0.0 ) { seconds = 0.0001; }
 fprintf(stderr,"FaceDetector Node Achieving %0.2f fps \n",(float) 1/seconds);


 return retres;
}



int cleanup_FaceDetector()
{
    if (colorFrame!=0) { free(colorFrame); colorFrame=0; }
    if (depthFrame!=0) { free(depthFrame); depthFrame=0; }
    return 1;
}



int stop_FaceDetector()
{

  fprintf(stderr,GREEN "\nDisparity Mapping Processor now gracefully stopping\n" NORMAL);
  fprintf(stderr,GREEN "_________________________________________________________________\n" NORMAL);
    cleanup_FaceDetector();

  fprintf(stderr,GREEN "_________________________________________________________________\n" NORMAL);
    return 1;
}


