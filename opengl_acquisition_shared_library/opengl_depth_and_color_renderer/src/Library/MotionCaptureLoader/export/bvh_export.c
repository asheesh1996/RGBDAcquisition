#include <stdio.h>
#include <stdlib.h>
#include "bvh_export.h"
#include "bvh_to_csv.h"
#include "bvh_to_svg.h"


#define NORMAL   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */



int performPointProjectionsForFrameForcingPositionAndRotation(
                                                              struct BVH_MotionCapture * mc,
                                                              struct BVH_Transform * bvhTransform,
                                                              unsigned int fID,
                                                              struct simpleRenderer * renderer,
                                                              float * forcePosition,
                                                              float * forceRotation,
                                                              unsigned int occlusions,
                                                              unsigned int directRendering
                                                             )
{
   BVHJointID rootJoint=0;

   if (
        !bvh_getRootJointID(
                             mc,
                             &rootJoint
                           )
      )
      {
        fprintf(stderr,"Error accessing root joint for frame %u\n",fID);
        return 0;
      }

   float dataOriginal[6]={0};
   if (!bhv_populatePosXYZRotXYZ(mc,rootJoint,fID,dataOriginal,sizeof(dataOriginal)))
      {
        fprintf(stderr,"Error accessing original position/rotation data for frame %u\n",fID);
        return 0;
      }

   float dataOur[6]={0};
   dataOur[0]=forcePosition[0];
   dataOur[1]=forcePosition[1];
   dataOur[2]=forcePosition[2];
   dataOur[3]=forceRotation[0];
   dataOur[4]=forceRotation[1];
   dataOur[5]=forceRotation[2];


   if (!bhv_setPosXYZRotXYZ(mc,rootJoint,fID,dataOur,sizeof(dataOur)))
      {
        fprintf(stderr,"Error adjusting position/rotation data for frame %u\n",fID);
        return 0;
      }

  //Try to load the 3D positions of each joint for this particular frame..
   if (bvh_loadTransformForFrame(mc,fID,bvhTransform,1/*Populate extra structures*/))
       {
        //If we succeed then we can perform the point projections to 2D..
        //Project 3D positions on 2D frame and save results..

        if (!bhv_setPosXYZRotXYZ(mc,rootJoint,fID,dataOriginal,sizeof(dataOriginal)))
        {
         fprintf(stderr,"Error restoring original data for frame %u\n",fID);
         return 0;
        }

        return bvh_projectTo2D(mc,bvhTransform,renderer,occlusions,directRendering);
       } else
       //If we fail to load transform , then we can't do any projections and need to clean up
       {
         bvh_cleanTransform(mc,bvhTransform);
       }
   //----------------------------------------------------------
 return 0;
}



int performPointProjectionsForFrame(
                                     struct BVH_MotionCapture * mc,
                                     struct BVH_Transform * bvhTransform,
                                     unsigned int fID,
                                     struct simpleRenderer * renderer,
                                     unsigned int occlusions,
                                     unsigned int directRendering
                                    )
{
  //Try to load the 3D positions of each joint for this particular frame..
   if (bvh_loadTransformForFrame(mc,fID,bvhTransform,1/*Populate extra structures*/))
       {
        //If we succeed then we can perform the point projections to 2D..
        //Project 3D positions on 2D frame and save results..
        return bvh_projectTo2D(mc,bvhTransform,renderer,occlusions,directRendering);
       } else
       //If we fail to load transform , then we can't do any projections and need to clean up
       { bvh_cleanTransform(mc,bvhTransform); }
   //----------------------------------------------------------
 return 0;
}

int performPointProjectionsForMotionBuffer(
                                            struct BVH_MotionCapture * mc,
                                            struct BVH_Transform * bvhTransform,
                                            float * motionBuffer,
                                            struct simpleRenderer * renderer,
                                            unsigned int occlusions,
                                            unsigned int directRendering
                                           )
{
  //First load the 3D positions of each joint from a motion buffer (instead of a frame [see performPointProjectionsForFrame]..
  if (bvh_loadTransformForMotionBuffer(mc,motionBuffer,bvhTransform,1/*Populate extra structures*/))
       {
        //If we succeed then we can perform the point projections to 2D..
        //Project 3D positions on 2D frame and save results..
         return bvh_projectTo2D(mc,bvhTransform,renderer,occlusions,directRendering);
       } else
       //If we fail to load transform , then we can't do any projections and need to clean up
       { bvh_cleanTransform(mc,bvhTransform); }
   //----------------------------------------------------------
 return 0;
}


int dumpBVHToSVGCSV(
                    const char * directory,
                    const char * filename,
                    int convertToSVG,
                    int convertToCSV,int useCSV_2D_Output,int useCSV_3D_Output,int useCSV_BVH_Output,
                    struct BVH_MotionCapture * mc,
                    unsigned int csvOrientation,
                    struct BVH_RendererConfiguration * renderConfig,
                    struct filteringResults * filterStats,
                    unsigned int occlusions,
                    unsigned int filterOutSkeletonsWithAnyLimbsBehindTheCamera,
                    unsigned int filterOutSkeletonsWithAnyLimbsOutOfImage,
                    unsigned int filterWeirdSkeletons,
                    unsigned int encodeRotationsAsRadians
                   )
{
  struct BVH_Transform bvhTransform={0};

  char svgFilename[512];
  //Declare and populate csv output files, we have 2D,3D and BVH files...
  char csvFilename2D[512]={0};
  char csvFilename3D[512]={0};
  char csvFilenameBVH[512]={0};
  if (useCSV_2D_Output)  { snprintf(csvFilename2D,512,"%s/2d_%s",directory,filename);  }
  if (useCSV_3D_Output)  { snprintf(csvFilename3D,512,"%s/3d_%s",directory,filename);  }
  if (useCSV_BVH_Output) { snprintf(csvFilenameBVH,512,"%s/bvh_%s",directory,filename);}

  struct simpleRenderer renderer={0};
  //Declare and populate the simpleRenderer that will project our 3D points

  if (renderConfig->isDefined)
  {
    renderer.fx=renderConfig->fX;
    renderer.fy=renderConfig->fY;
    renderer.skew=1.0;
    renderer.cx=renderConfig->cX;
    renderer.cy=renderConfig->cY;
    renderer.near=1.0;
    renderer.far=10000.0;
    renderer.width=renderConfig->width;
    renderer.height=renderConfig->height;

    //renderer.cameraOffsetPosition[4];
    //renderer.cameraOffsetRotation[4];
    //renderer.removeObjectPosition;


    //renderer.projectionMatrix[16];
    //renderer.viewMatrix[16];
    //renderer.modelMatrix[16];
    //renderer.modelViewMatrix[16];
    //renderer.viewport[4];

    simpleRendererInitializeFromExplicitConfiguration(&renderer);
    fprintf(stderr,"Direct Rendering is not implemented yet, please don't use it..\n");
    exit(1);
  } else
  {
   //This is the normal rendering where we just simulate our camera center
   simpleRendererDefaults(
                          &renderer,
                          renderConfig->width,
                          renderConfig->height,
                          renderConfig->fX,
                          renderConfig->fY
                         );
    simpleRendererInitialize(&renderer);
  }



  //If we are dumping to CSV  we need to populate the header, this happens only one time for the file
  if (convertToCSV)
   {
    dumpBVHToCSVHeader(
                        mc,
                        csvFilename2D,
                        csvFilename3D,
                        csvFilenameBVH
                      );
   }




  //------------------------------------------------------------------------------------------
  //                                    For every frame
  //------------------------------------------------------------------------------------------
  unsigned int framesDumped=0;
  unsigned int fID=0;
  for (fID=0; fID<mc->numberOfFrames; fID++)
  {
   if (
       !performPointProjectionsForFrame(
                                        mc,
                                        &bvhTransform,
                                        fID,
                                        &renderer,
                                        occlusions,
                                        renderConfig->isDefined
                                       )
       )
   {
       fprintf(stderr,RED "Could not perform projection for frame %u\n" NORMAL,fID);
   }


  //Having projected our BVH data to 3D points using our simpleRenderer Configuration we can store our output to CSV or SVG files..

  //CSV output
  //------------------------------------------------------------------------------------------
   if (convertToCSV)
   {
            dumpBVHToCSVBody(
                             mc,
                             &bvhTransform,
                             &renderer,
                             fID,
                             csvFilename2D,
                             csvFilename3D,
                             csvFilenameBVH,
                             csvOrientation,
                             filterStats,
                             filterOutSkeletonsWithAnyLimbsBehindTheCamera,
                             filterOutSkeletonsWithAnyLimbsOutOfImage,
                             filterWeirdSkeletons,
                             encodeRotationsAsRadians
                            );
   }
  //------------------------------------------------------------------------------------------


  //SVG output
  //------------------------------------------------------------------------------------------
   if (convertToSVG)
   {
     snprintf(svgFilename,512,"%s/%06u.svg",directory,fID);
     framesDumped +=  dumpBVHToSVGFrame(
                                       svgFilename,
                                       mc,
                                       &bvhTransform,
                                       fID,
                                       &renderer
                                      );
   }
  //------------------------------------------------------------------------------------------
  } //For every frame.. ----------------------------------------------------------------------
  //------------------------------------------------------------------------------------------



  //------------------------------------------------------------------------------------------
  //                            GIVE FINAL CONSOLE OUTPUT HERE
  //------------------------------------------------------------------------------------------
  if (filterStats->visibleJoints!=0)
  {
    fprintf(stderr,"Joint Visibility = %0.2f %%\n",(float) 100*filterStats->invisibleJoints/filterStats->visibleJoints);
  }
  fprintf(stderr,"CSV Outputs: 2D:%d, 3D:%d, BVH:%d\n",useCSV_2D_Output,useCSV_3D_Output,useCSV_BVH_Output);
  //------------------------------------------------------------------------------------------
  fprintf(stderr,"Joints : %u invisible / %u visible ",filterStats->invisibleJoints,filterStats->visibleJoints);
  if (occlusions) { fprintf(stderr,"(occlusions enabled)\n"); } else
                  { fprintf(stderr,"(occlusions disabled)\n");}
  if (renderConfig->isDefined)  { fprintf(stderr,"External Renderer Configuration\n");  } else
                                { fprintf(stderr,"Regular camera position rendering\n");}
  fprintf(stderr,"Filtered out CSV poses : %u\n",filterStats->filteredOutCSVPoses);
  fprintf(stderr,"Filtered behind camera : %u\n",filterStats->filteredOutCSVBehindPoses);
  fprintf(stderr,"Filtered out of camera frame : %u\n",filterStats->filteredOutCSVOutPoses);
  if (mc->numberOfFrames!=0)
  {
   float usedPercentage = (float) 100*(mc->numberOfFrames-filterStats->filteredOutCSVPoses)/mc->numberOfFrames;
   if (usedPercentage==0.0) { fprintf(stderr,RED "----------------\n----------------\n----------------\n"); }
   fprintf(stderr,"Used %0.2f%% of dataset\n",usedPercentage);
   if (usedPercentage==0.0) { fprintf(stderr, "----------------\n----------------\n----------------\n" NORMAL); }

  }
  //------------------------------------------------------------------------------------------
  //------------------------------------------------------------------------------------------


 return (framesDumped==mc->numberOfFrames);
}
