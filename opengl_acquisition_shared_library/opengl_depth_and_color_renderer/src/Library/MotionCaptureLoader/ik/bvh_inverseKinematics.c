#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include <time.h>

#include "bvh_inverseKinematics.h"
#include "levmar.h"

#include "../export/bvh_to_svg.h"
#include "../edit/bvh_cut_paste.h"

#define MAXIMUM_CHAINS 10
#define MAXIMUM_PARTS_OF_CHAIN 10


#define NORMAL   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */


#define DISCARD_POSITIONAL_COMPONENT 0
const float distance=-150;

unsigned long tickBaseIK = 0;


unsigned long GetTickCountMicrosecondsIK()
{
    struct timespec ts;
    if ( clock_gettime(CLOCK_MONOTONIC,&ts) != 0)
        {
            return 0;
        }

    if (tickBaseIK==0)
        {
            tickBaseIK = ts.tv_sec*1000000 + ts.tv_nsec/1000;
            return 0;
        }

    return ( ts.tv_sec*1000000 + ts.tv_nsec/1000 ) - tickBaseIK;
}


unsigned long GetTickCountMillisecondsIK()
{
    return (unsigned long) GetTickCountMicrosecondsIK()/1000;
}


struct ikChainParts
{
 BVHJointID jID;
 BVHMotionChannelID mIDStart;
 BVHMotionChannelID mIDEnd;
 char evaluated;
 char endEffector;
};
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------

struct ikChain
{
  unsigned int jobID;
  unsigned int groupID;

  unsigned int numberOfParts;
  struct ikChainParts part[MAXIMUM_PARTS_OF_CHAIN];

  struct MotionBuffer * currentSolution;
  struct BVH_Transform current2DProjectionTransform;

  float initialError;
  float previousError;
  float currentError;
};
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------

struct ikProblem
{
 //BVH file that reflects our problem
 struct BVH_MotionCapture * mc;
 //Renderer that handles 3D projections
 struct simpleRenderer *renderer;


 //Initial solution
 struct MotionBuffer * initialSolution;
 //Current solution
 struct MotionBuffer * currentSolution;

 //2D Projections Targeted
 struct BVH_Transform * bvhTarget2DProjectionTransform;

 //Chain of subproblems that need to be solved
 unsigned int numberOfChains;
 unsigned int numberOfGroups;
 unsigned int numberOfJobsPerGroup;

 struct ikChain chain[MAXIMUM_CHAINS];
};
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------

void freeSolutionBuffer(struct MotionBuffer * mb)
{
 if (mb!=0)
 {
  if(mb->motion!=0)
   {
     free(mb->motion);
     mb->motion=0;
   }
  free(mb);
 }
}
//---------------------------------------------------------

int copyMotionBuffer(struct MotionBuffer * dst,struct MotionBuffer * src)
{
  if (src->bufferSize != dst->bufferSize)
  {
    fprintf(stderr,"Buffer Size mismatch..\n");
    return 0;
  }

  for (unsigned int i=0; i<dst->bufferSize; i++)
  {
    dst->motion[i] = src->motion[i];
  }
  return 1;
}

struct MotionBuffer * mallocNewSolutionBuffer(struct BVH_MotionCapture * mc)
{
  struct MotionBuffer * newBuffer = (struct MotionBuffer *)  malloc(sizeof(struct MotionBuffer));
  if (newBuffer!=0)
  {
    newBuffer->bufferSize = mc->numberOfValuesPerFrame;
    newBuffer->motion = (float *) malloc(sizeof(float) * newBuffer->bufferSize);
    if (newBuffer->motion!=0)
    {
      memset(newBuffer->motion,0,sizeof(float) * newBuffer->bufferSize);
    }
  }

  return newBuffer;
}
//---------------------------------------------------------


struct MotionBuffer * mallocNewSolutionBufferAndCopy(struct BVH_MotionCapture * mc,struct MotionBuffer * whatToCopy)
{
  struct MotionBuffer * newBuffer = (struct MotionBuffer *)  malloc(sizeof(struct MotionBuffer));
  if (newBuffer!=0)
  {
    newBuffer->bufferSize = mc->numberOfValuesPerFrame;
    newBuffer->motion = (float *) malloc(sizeof(float) * newBuffer->bufferSize);
    if (newBuffer->motion!=0)
    {
      for (unsigned int i=0; i<newBuffer->bufferSize; i++)
      {
        newBuffer->motion[i]=whatToCopy->motion[i];
      }
    }
  }

  return newBuffer;
}
//---------------------------------------------------------



void compareMotionBuffers(const char * msg,struct MotionBuffer * guess,struct MotionBuffer * groundTruth)
{
  fprintf(stderr,"%s \n",msg);
  fprintf(stderr,"___________\n");

  if (guess->bufferSize != groundTruth->bufferSize)
  {
    fprintf(stderr,"Buffer Size mismatch..\n");
    return ;
  }

  //--------------------------------------------------
  fprintf(stderr,"Guess : ");
  for (unsigned int i=0; i<guess->bufferSize; i++)
  {
    fprintf(stderr,"%0.2f " ,guess->motion[i]);
  }
  fprintf(stderr,"\n");
  //--------------------------------------------------
  fprintf(stderr,"Truth : ");
  for (unsigned int i=0; i<groundTruth->bufferSize; i++)
  {
    fprintf(stderr,"%0.2f " ,groundTruth->motion[i]);
  }
  fprintf(stderr,"\n");
  //--------------------------------------------------


  fprintf(stderr,"Diff : ");

  for (unsigned int i=0; i<guess->bufferSize; i++)
  {
    float diff=fabs(groundTruth->motion[i] - guess->motion[i]);
    if (fabs(diff)<0.1) { fprintf(stderr,GREEN "%0.2f " ,diff); } else
                         { fprintf(stderr,RED "%0.2f " ,diff); }
  }
  fprintf(stderr,NORMAL "\n___________\n");
}


void compareTwoMotionBuffers(struct BVH_MotionCapture * mc,const char * msg,struct MotionBuffer * guessA,struct MotionBuffer * guessB,struct MotionBuffer * groundTruth)
{
  fprintf(stderr,"%s \n",msg);
  fprintf(stderr,"___________\n");

  if ( (guessA->bufferSize != groundTruth->bufferSize) || (guessB->bufferSize != groundTruth->bufferSize) )
  {
    fprintf(stderr,"Buffer Size mismatch..\n");
    return ;
  }


  fprintf(stderr,"Diff : ");
  for (unsigned int i=0; i<guessA->bufferSize; i++)
  {
    float diffA=fabs(groundTruth->motion[i] - guessA->motion[i]);
    float diffB=fabs(groundTruth->motion[i] - guessB->motion[i]);
    if ( (diffA==0.0) && (diffA==diffB) )  { fprintf(stderr,BLUE  "%0.2f ",diffA-diffB); } else
    {
     if (diffA>=diffB)                     { fprintf(stderr,GREEN "%0.2f ",diffA-diffB); } else
                                           { fprintf(stderr,RED   "%0.2f ",diffB-diffA); }

     unsigned int jID =mc->motionToJointLookup[i].jointID;
     unsigned int chID=mc->motionToJointLookup[i].channelID;
     fprintf(stderr,NORMAL "(%s#%u/%0.2f->%0.2f/%0.2f) ",mc->jointHierarchy[jID].jointName,chID,guessA->motion[i],guessB->motion[i],groundTruth->motion[i]);
    }
  }
  fprintf(stderr,NORMAL "\n___________\n");
}


void clear_line()
{
  fputs("\033[A\033[2K\033[A\033[2K",stdout);
  rewind(stdout);
  int i=ftruncate(1,0);
  if (i!=0) { /*fprintf(stderr,"Error with ftruncate\n");*/ }
}

float getSquared2DPointDistance(float aX,float aY,float bX,float bY)
{
  float diffX = (float) aX-bX;
  float diffY = (float) aY-bY;
    //We calculate the distance here..!
  return (diffX*diffX) + (diffY*diffY);
}


float get2DPointDistance(float aX,float aY,float bX,float bY)
{
  return sqrt(getSquared2DPointDistance(aX,aY,bX,bY));
}


float meanSquaredBVH2DDistace(
                              struct BVH_MotionCapture * mc,
                              struct simpleRenderer *renderer,
                              int useAllJoints,
                              BVHMotionChannelID onlyConsiderChildrenOfThisJoint,
                              struct BVH_Transform * bvhSourceTransform,
                              struct BVH_Transform * bvhTargetTransform
                             )
{
   if (
        (bvh_projectTo2D(mc,bvhSourceTransform,renderer,0,0)) &&
        (bvh_projectTo2D(mc,bvhTargetTransform,renderer,0,0))
      )
      {
       //-----------------
       float sumOf2DDistances=0.0;
       unsigned int numberOfSamples=0;
       for (unsigned int jID=0; jID<mc->jointHierarchySize; jID++)
            {
              int isSelected = 1;

              if (mc->selectedJoints!=0)
              {
                if (!mc->selectedJoints[jID])
                {
                  isSelected=0;
                }
              }

               if ( (isSelected) && ( (useAllJoints) || (mc->jointHierarchy[jID].parentJoint == onlyConsiderChildrenOfThisJoint) ) )
               {
                float thisSquared2DDistance=getSquared2DPointDistance(
                                                                      (float) bvhSourceTransform->joint[jID].pos2D[0],
                                                                      (float) bvhSourceTransform->joint[jID].pos2D[1],
                                                                      (float) bvhTargetTransform->joint[jID].pos2D[0],
                                                                      (float) bvhTargetTransform->joint[jID].pos2D[1]
                                                                     );
               fprintf(stderr,"%0.2f,%0.2f -> %0.2f,%0.2f : ",bvhSourceTransform->joint[jID].pos2D[0],bvhSourceTransform->joint[jID].pos2D[1],bvhTargetTransform->joint[jID].pos2D[0],bvhTargetTransform->joint[jID].pos2D[1]);
               fprintf(stderr,"Joint squared %s distance is %0.2f\n",mc->jointHierarchy[jID].jointName,thisSquared2DDistance);

               numberOfSamples+=1;
               sumOf2DDistances+=thisSquared2DDistance;
              }
            }

       if (numberOfSamples>0)
       {
         return (float)  sumOf2DDistances/numberOfSamples;
       }
     } //-----------------

 return 0.0;
}

int prepareProblem(
                   struct ikProblem * problem,
                   struct BVH_MotionCapture * mc,
                   struct simpleRenderer *renderer,
                   struct MotionBuffer * solution,
                   float * averageError,
                   struct BVH_Transform * bvhTargetTransform
                  )
{
  problem->mc = mc;
  problem->renderer = renderer;
  problem->initialSolution = solution ;

  problem->currentSolution=mallocNewSolutionBufferAndCopy(mc,problem->initialSolution);

  //2D Projections Targeted
  //----------------------------------------------------------
  problem->bvhTarget2DProjectionTransform = bvhTargetTransform;

 //Chain of subproblems that need to be solved
  //----------------------------------------------------------
  problem->numberOfChains = 5;
  problem->numberOfGroups = 2;
  problem->numberOfJobsPerGroup = 6;



  //Chain #0 is Joint Hip-> to all its children
  //----------------------------------------------------------
  //----------------------------------------------------------
  //----------------------------------------------------------
  unsigned int groupID=0;
  unsigned int jobID=0;
  unsigned int chainID=0;
  unsigned int partID=0;
  BVHJointID thisJID=0;
  //----------------------------------------------------------

  //Chain 0 is the Hip and all of the rigid torso
  //----------------------------------------------------------
  problem->chain[chainID].groupID=groupID;
  problem->chain[chainID].jobID=jobID;
  problem->chain[chainID].currentSolution=mallocNewSolutionBufferAndCopy(mc,problem->initialSolution);

/*
  #if DISCARD_POSITIONAL_COMPONENT
   fprintf(stderr,"Ignoring positional component..\n");
  #else
   if (bvh_getJointIDFromJointName(mc,"hip",&thisJID) )
   {
     problem->chain[chainID].part[partID].evaluated=0; //Not evaluated yet
     problem->chain[chainID].part[partID].jID=thisJID;
     problem->chain[chainID].part[partID].mIDStart=0; //First Rotation
     problem->chain[chainID].part[partID].mIDEnd=2; //First Rotation
     ++partID;
   }
  #endif // DISCARD_POSITIONAL_COMPONENT
*/

  if (bvh_getJointIDFromJointName(mc,"hip",&thisJID) )
  {
   problem->chain[chainID].part[partID].evaluated=0; //Not evaluated yet
   problem->chain[chainID].part[partID].jID=thisJID;
   problem->chain[chainID].part[partID].mIDStart=3; //First Rotation
   problem->chain[chainID].part[partID].mIDEnd=5; //First Rotation
   ++partID;
  }

  if (bvh_getJointIDFromJointName(mc,"neck",&thisJID) )
  {
   problem->chain[chainID].part[partID].evaluated=0; //Not evaluated yet
   problem->chain[chainID].part[partID].jID=thisJID;
   problem->chain[chainID].part[partID].endEffector=1;
   ++partID;
  }

  if (bvh_getJointIDFromJointName(mc,"rshoulder",&thisJID) )
  {
   problem->chain[chainID].part[partID].evaluated=0; //Not evaluated yet
   problem->chain[chainID].part[partID].jID=thisJID;
   problem->chain[chainID].part[partID].endEffector=1;
   ++partID;
  }

  if (bvh_getJointIDFromJointName(mc,"lshoulder",&thisJID) )
  {
   problem->chain[chainID].part[partID].evaluated=0; //Not evaluated yet
   problem->chain[chainID].part[partID].jID=thisJID;
   problem->chain[chainID].part[partID].endEffector=1;
   ++partID;
  }

  if (bvh_getJointIDFromJointName(mc,"rhip",&thisJID) )
  {
   problem->chain[chainID].part[partID].evaluated=0; //Not evaluated yet
   problem->chain[chainID].part[partID].jID=thisJID;
   problem->chain[chainID].part[partID].endEffector=1;
   ++partID;
  }

  if (bvh_getJointIDFromJointName(mc,"lhip",&thisJID) )
  {
   problem->chain[chainID].part[partID].evaluated=0; //Not evaluated yet
   problem->chain[chainID].part[partID].jID=thisJID;
   problem->chain[chainID].part[partID].endEffector=1;

   ++partID;
  }

 problem->chain[chainID].numberOfParts=partID;

 ++chainID;
 ++groupID;
  //----------------------------------------------------------
  //----------------------------------------------------------
  //----------------------------------------------------------


  //Chain 1 is the Right Arm
  //----------------------------------------------------------
  //----------------------------------------------------------
  //----------------------------------------------------------
  partID=0;
  problem->chain[chainID].groupID=groupID;
  problem->chain[chainID].jobID=jobID;
  problem->chain[chainID].currentSolution=mallocNewSolutionBufferAndCopy(mc,problem->initialSolution);



  if (bvh_getJointIDFromJointName(mc,"rshoulder",&thisJID) )
  {
   problem->chain[chainID].part[partID].evaluated=0; //Not evaluated yet
   problem->chain[chainID].part[partID].endEffector=0;
   problem->chain[chainID].part[partID].jID=thisJID;
   problem->chain[chainID].part[partID].mIDStart=mc->jointToMotionLookup[thisJID].jointMotionOffset; //First Rotation
   problem->chain[chainID].part[partID].mIDEnd=problem->chain[chainID].part[partID].mIDStart + mc->jointHierarchy[thisJID].loadedChannels-1;
   ++partID;
  }

  if (bvh_getJointIDFromJointName(mc,"relbow",&thisJID) )
  {
   problem->chain[chainID].part[partID].evaluated=0; //Not evaluated yet
   problem->chain[chainID].part[partID].endEffector=0;
   problem->chain[chainID].part[partID].jID=thisJID;
   problem->chain[chainID].part[partID].mIDStart=mc->jointToMotionLookup[thisJID].jointMotionOffset; //First Rotation
   problem->chain[chainID].part[partID].mIDEnd=problem->chain[chainID].part[partID].mIDStart + mc->jointHierarchy[thisJID].loadedChannels-1;
   ++partID;
  }

  if (bvh_getJointIDFromJointName(mc,"rhand",&thisJID) )
  {
   problem->chain[chainID].part[partID].evaluated=0; //Not evaluated yet
   problem->chain[chainID].part[partID].jID=thisJID;
   problem->chain[chainID].part[partID].endEffector=1;
   ++partID;
  }

 problem->chain[chainID].numberOfParts=partID;
 ++chainID;
 ++jobID;
  //----------------------------------------------------------
  //----------------------------------------------------------
  //----------------------------------------------------------





  //Chain 2 is the Left Arm
  //----------------------------------------------------------
  //----------------------------------------------------------
  //----------------------------------------------------------
  partID=0;
  problem->chain[chainID].groupID=groupID;
  problem->chain[chainID].jobID=jobID;
  problem->chain[chainID].currentSolution=mallocNewSolutionBufferAndCopy(mc,problem->initialSolution);



  if (bvh_getJointIDFromJointName(mc,"lshoulder",&thisJID) )
  {
   problem->chain[chainID].part[partID].evaluated=0; //Not evaluated yet
   problem->chain[chainID].part[partID].endEffector=0;
   problem->chain[chainID].part[partID].jID=thisJID;
   problem->chain[chainID].part[partID].mIDStart=mc->jointToMotionLookup[thisJID].jointMotionOffset; //First Rotation
   problem->chain[chainID].part[partID].mIDEnd=problem->chain[chainID].part[partID].mIDStart + mc->jointHierarchy[thisJID].loadedChannels-1;
   ++partID;
  }

  if (bvh_getJointIDFromJointName(mc,"lelbow",&thisJID) )
  {
   problem->chain[chainID].part[partID].evaluated=0; //Not evaluated yet
   problem->chain[chainID].part[partID].endEffector=0;
   problem->chain[chainID].part[partID].jID=thisJID;
   problem->chain[chainID].part[partID].mIDStart=mc->jointToMotionLookup[thisJID].jointMotionOffset; //First Rotation
   problem->chain[chainID].part[partID].mIDEnd=problem->chain[chainID].part[partID].mIDStart + mc->jointHierarchy[thisJID].loadedChannels-1;
   ++partID;
  }

  if (bvh_getJointIDFromJointName(mc,"lhand",&thisJID) )
  {
   problem->chain[chainID].part[partID].evaluated=0; //Not evaluated yet
   problem->chain[chainID].part[partID].jID=thisJID;
   problem->chain[chainID].part[partID].endEffector=1;
   ++partID;
  }

 problem->chain[chainID].numberOfParts=partID;
 ++chainID;
 ++jobID;
  //----------------------------------------------------------
  //----------------------------------------------------------
  //----------------------------------------------------------





  //Chain 3 is the Right Leg
  //----------------------------------------------------------
  //----------------------------------------------------------
  //----------------------------------------------------------
  partID=0;
  problem->chain[chainID].groupID=groupID;
  problem->chain[chainID].jobID=jobID;
  problem->chain[chainID].currentSolution=mallocNewSolutionBufferAndCopy(mc,problem->initialSolution);


  if (bvh_getJointIDFromJointName(mc,"rhip",&thisJID) )
  {
   problem->chain[chainID].part[partID].evaluated=0; //Not evaluated yet
   problem->chain[chainID].part[partID].endEffector=0;
   problem->chain[chainID].part[partID].jID=thisJID;
   problem->chain[chainID].part[partID].mIDStart=mc->jointToMotionLookup[thisJID].jointMotionOffset; //First Rotation
   problem->chain[chainID].part[partID].mIDEnd=problem->chain[chainID].part[partID].mIDStart + mc->jointHierarchy[thisJID].loadedChannels-1;
   ++partID;
  }

  if (bvh_getJointIDFromJointName(mc,"rknee",&thisJID) )
  {
   problem->chain[chainID].part[partID].evaluated=0; //Not evaluated yet
   problem->chain[chainID].part[partID].endEffector=0;
   problem->chain[chainID].part[partID].jID=thisJID;
   problem->chain[chainID].part[partID].mIDStart=mc->jointToMotionLookup[thisJID].jointMotionOffset; //First Rotation
   problem->chain[chainID].part[partID].mIDEnd=problem->chain[chainID].part[partID].mIDStart + mc->jointHierarchy[thisJID].loadedChannels-1;
   ++partID;
  }

  if (bvh_getJointIDFromJointName(mc,"rfoot",&thisJID) )
  {
   problem->chain[chainID].part[partID].evaluated=0; //Not evaluated yet
   problem->chain[chainID].part[partID].jID=thisJID;
   problem->chain[chainID].part[partID].endEffector=1;
   ++partID;
  }

 problem->chain[chainID].numberOfParts=partID;
 ++chainID;
 ++jobID;
  //----------------------------------------------------------
  //----------------------------------------------------------
  //----------------------------------------------------------




  //Chain 4 is the Left Leg
  //----------------------------------------------------------
  //----------------------------------------------------------
  //----------------------------------------------------------
  partID=0;
  problem->chain[chainID].groupID=groupID;
  problem->chain[chainID].jobID=jobID;
  problem->chain[chainID].currentSolution=mallocNewSolutionBufferAndCopy(mc,problem->initialSolution);

  if (bvh_getJointIDFromJointName(mc,"lhip",&thisJID) )
  {
   problem->chain[chainID].part[partID].evaluated=0; //Not evaluated yet
   problem->chain[chainID].part[partID].endEffector=0;
   problem->chain[chainID].part[partID].jID=thisJID;
   problem->chain[chainID].part[partID].mIDStart=mc->jointToMotionLookup[thisJID].jointMotionOffset; //First Rotation
   problem->chain[chainID].part[partID].mIDEnd=problem->chain[chainID].part[partID].mIDStart + mc->jointHierarchy[thisJID].loadedChannels-1;
   ++partID;
  }

  if (bvh_getJointIDFromJointName(mc,"lknee",&thisJID) )
  {
   problem->chain[chainID].part[partID].evaluated=0; //Not evaluated yet
   problem->chain[chainID].part[partID].endEffector=0;
   problem->chain[chainID].part[partID].jID=thisJID;
   problem->chain[chainID].part[partID].mIDStart=mc->jointToMotionLookup[thisJID].jointMotionOffset; //First Rotation
   problem->chain[chainID].part[partID].mIDEnd=problem->chain[chainID].part[partID].mIDStart + mc->jointHierarchy[thisJID].loadedChannels-1;
   ++partID;
  }

  if (bvh_getJointIDFromJointName(mc,"lfoot",&thisJID) )
  {
   problem->chain[chainID].part[partID].evaluated=0; //Not evaluated yet
   problem->chain[chainID].part[partID].jID=thisJID;
   problem->chain[chainID].part[partID].endEffector=1;
   ++partID;
  }

 problem->chain[chainID].numberOfParts=partID;
 ++chainID;
 ++jobID;
  //----------------------------------------------------------
  //----------------------------------------------------------
  //----------------------------------------------------------
  ++groupID;

  problem->numberOfChains = chainID;
  problem->numberOfGroups = groupID;

 return 1;
}


int viewProblem(
                struct ikProblem * problem
               )
{
 fprintf(stderr,"The IK problem we want to solve has %u groups of subproblems\n",problem->numberOfGroups);
 fprintf(stderr,"It is also ultimately divided into %u kinematic chains\n",problem->numberOfChains);

 for (unsigned int chainID=0; chainID<problem->numberOfChains; chainID++)
 {
   fprintf(stderr,"Chain %u has %u parts : ",chainID,problem->chain[chainID].numberOfParts);
   for (unsigned int partID=0; partID<problem->chain[chainID].numberOfParts; partID++)
   {
     unsigned int jID=problem->chain[chainID].part[partID].jID;

     if (problem->chain[chainID].part[partID].endEffector)
     {
      fprintf(
              stderr,"jID(%s/%u)->EndEffector ",
              problem->mc->jointHierarchy[jID].jointName,
              jID
             );
     } else
     {
     fprintf(
             stderr,"jID(%s/%u)->mID(%u to %u) ",
             problem->mc->jointHierarchy[jID].jointName,
             jID,
             problem->chain[chainID].part[partID].mIDStart,
             problem->chain[chainID].part[partID].mIDEnd
             );

     }
   }
   fprintf(stderr,"\n");
 }

 return 1;
}




float calculateChainLoss(
                         struct ikProblem * problem,
                         unsigned int chainID
                        )
{
  unsigned int numberOfSamples=0;
  float loss=0.0;
  if (chainID<problem->numberOfChains)
  {
   //fprintf(stderr,"Chain %u has %u parts : ",chainID,problem->chain[chainID].numberOfParts);

     if (
         bvh_loadTransformForMotionBuffer(
                                          problem->mc,
                                          problem->chain[chainID].currentSolution->motion,
                                          &problem->chain[chainID].current2DProjectionTransform
                                         )
        )
      {

       #if DISCARD_POSITIONAL_COMPONENT
        bvh_removeTranslationFromTransform(
                                            problem->mc,
                                            &problem->chain[chainID].current2DProjectionTransform
                                          );
       #endif // DISCARD_POSITIONAL_COMPONENT

      if (
          (bvh_projectTo2D(problem->mc,&problem->chain[chainID].current2DProjectionTransform,problem->renderer,0,0)) &&
          (bvh_projectTo2D(problem->mc,problem->bvhTarget2DProjectionTransform,problem->renderer,0,0))
         )
      {
       for (unsigned int partID=0; partID<problem->chain[chainID].numberOfParts; partID++)
       {
         unsigned int jID=problem->chain[chainID].part[partID].jID;
         float thisSquared2DDistance=getSquared2DPointDistance(
                                                                (float) problem->chain[chainID].current2DProjectionTransform.joint[jID].pos2D[0],
                                                                (float) problem->chain[chainID].current2DProjectionTransform.joint[jID].pos2D[1],
                                                                (float) problem->bvhTarget2DProjectionTransform->joint[jID].pos2D[0],
                                                                (float) problem->bvhTarget2DProjectionTransform->joint[jID].pos2D[1]
                                                               );
                                                               /*
         fprintf(stderr,"%0.2f,%0.2f -> %0.2f,%0.2f : ",
         problem->chain[chainID].current2DProjectionTransform.joint[jID].pos2D[0],
         problem->chain[chainID].current2DProjectionTransform.joint[jID].pos2D[1],
         problem->bvhTarget2DProjectionTransform->joint[jID].pos2D[0],
         problem->bvhTarget2DProjectionTransform->joint[jID].pos2D[1]
         );
         fprintf(stderr,"Joint squared %s distance is %0.2f\n",problem->mc->jointHierarchy[jID].jointName,thisSquared2DDistance);*/
         loss+=thisSquared2DDistance;
         ++numberOfSamples;
       }
     }

      } //Have a valid 2D transform
 } //Have a valid chain

 //I have left 0/0 on purpose to cause NaNs when projection errors occur
  loss = (float) loss/numberOfSamples;
  //fprintf(stderr,"loss %0.2f\n",loss);
  return loss;
}


float iteratePartLoss(
                         struct ikProblem * problem,
                         unsigned int chainID,
                         unsigned int partID,
                         unsigned int iterations
                        )
{
 unsigned int consecutiveBadSteps=0;
 unsigned int mIDS[3];
 mIDS[0]= problem->chain[chainID].part[partID].mIDStart;
 mIDS[1]= problem->chain[chainID].part[partID].mIDStart+1;
 mIDS[2]= problem->chain[chainID].part[partID].mIDStart+2;

 float originalLoss = calculateChainLoss(problem,chainID);


 float originalValues[3];
 originalValues[0] = problem->chain[chainID].currentSolution->motion[mIDS[0]];
 originalValues[1] = problem->chain[chainID].currentSolution->motion[mIDS[1]];
 originalValues[2] = problem->chain[chainID].currentSolution->motion[mIDS[2]];

 float previousValues[3];
 previousValues[0] = originalValues[0];
 previousValues[1] = originalValues[1];
 previousValues[2] = originalValues[2];

 float previousLoss[3];


 float currentValues[3];
 currentValues[0] = originalValues[0];
 currentValues[1] = originalValues[1];
 currentValues[2] = originalValues[2];

 float currentLoss[3];

 float bestValues[3];
 bestValues[0] = originalValues[0];
 bestValues[1] = originalValues[1];
 bestValues[2] = originalValues[2];

 float bestDelta[3]={0};
 float delta[3]={0};

 float initialLoss = calculateChainLoss(problem,chainID);

 previousLoss[0]=initialLoss;
 previousLoss[1]=initialLoss;
 previousLoss[2]=initialLoss;

 float bestLoss = initialLoss;
 float loss=initialLoss;

 unsigned int maximumConsecutiveBadLoss=4;
 float e=0.001;
 float d=0.01;
 float lr=0.0021;
 float gradient;

 delta[0] = d;
 delta[1] = d;
 delta[2] = d;
 //currentValues[0] += d;
 //currentValues[1] += d;
 //currentValues[2] += d;



 fprintf(stderr,"\nOptimizing %s \n",problem->mc->jointHierarchy[problem->chain[chainID].part[partID].jID].jointName);
 fprintf(stderr,"  State |   loss   | rX  |  rY  |  rZ \n");
 fprintf(stderr,"Initial | %0.1f | %0.2f  |  %0.2f  |  %0.2f \n",initialLoss,originalValues[0],originalValues[1],originalValues[2]);
 unsigned long startTime = GetTickCountMicrosecondsIK();

 for (unsigned int i=0; i<iterations; i++)
 {

 #define SHARE_LOSS 0

 #if SHARE_LOSS
 //-------------------
   problem->chain[chainID].currentSolution->motion[mIDS[0]] = currentValues[0] + delta[0];
   problem->chain[chainID].currentSolution->motion[mIDS[1]] = currentValues[1] + delta[1];
   problem->chain[chainID].currentSolution->motion[mIDS[2]] = currentValues[2] + delta[2];
 //-------------------
   currentLoss[0]=calculateChainLoss(problem,chainID);
   currentLoss[1]=currentLoss[0];
   currentLoss[2]=currentLoss[0];
 //-------------------
   problem->chain[chainID].currentSolution->motion[mIDS[0]] = currentValues[0];
   problem->chain[chainID].currentSolution->motion[mIDS[1]] = currentValues[1];
   problem->chain[chainID].currentSolution->motion[mIDS[2]] = currentValues[2];
 //-------------------
 #else
 //-------------------
   problem->chain[chainID].currentSolution->motion[mIDS[0]] = currentValues[0];
   currentLoss[0]=calculateChainLoss(problem,chainID);
   problem->chain[chainID].currentSolution->motion[mIDS[0]] = previousValues[0];
 //-------------------
   problem->chain[chainID].currentSolution->motion[mIDS[1]] = currentValues[1];
   currentLoss[1]=calculateChainLoss(problem,chainID);
   problem->chain[chainID].currentSolution->motion[mIDS[1]] = previousValues[1];
 //-------------------
   problem->chain[chainID].currentSolution->motion[mIDS[2]] = currentValues[2];
   currentLoss[2]=calculateChainLoss(problem,chainID);
   problem->chain[chainID].currentSolution->motion[mIDS[2]] = previousValues[2];
 //-------------------
 #endif


   //We multiply by 0.5 to do a "One Half Mean Squared Error"
   gradient =  (float) 0.5 * (previousLoss[0] - currentLoss[0]) / (delta[0]+e);
   delta[0] += (float) lr *  gradient;

   gradient =  (float) 0.5 * (previousLoss[1] - currentLoss[1]) / (delta[1]+e);
   delta[1] += (float) lr * gradient;

   gradient =  (float) 0.5 * (previousLoss[2] - currentLoss[2]) / (delta[2]+e);
   delta[2] += (float) lr * gradient;

   previousLoss[0]=currentLoss[0];
   previousLoss[1]=currentLoss[1];
   previousLoss[2]=currentLoss[2];

   previousValues[0] = currentValues[0];
   previousValues[1] = currentValues[1];
   previousValues[2] = currentValues[2];

   currentValues[0]+=delta[0];
   currentValues[1]+=delta[1];
   currentValues[2]+=delta[2];

   problem->chain[chainID].currentSolution->motion[mIDS[0]] = currentValues[0];
   problem->chain[chainID].currentSolution->motion[mIDS[1]] = currentValues[1];
   problem->chain[chainID].currentSolution->motion[mIDS[2]] = currentValues[2];
   loss=calculateChainLoss(problem,chainID);


   if ( (loss<bestLoss) && ( fabs(currentValues[0])<360 ) && ( fabs(currentValues[1])<360 ) && ( fabs(currentValues[2])<360 ) )
   {
     bestLoss=loss;
     bestDelta[0]=delta[0];
     bestDelta[1]=delta[1];
     bestDelta[2]=delta[2];

     bestValues[0]=currentValues[0];
     bestValues[1]=currentValues[1];
     bestValues[2]=currentValues[2];
     consecutiveBadSteps=0;
     fprintf(stderr,"%07u | %0.1f | %0.2f  |  %0.2f  |  %0.2f \n",i,loss,currentValues[0],currentValues[1],currentValues[2]);
   } else
   {
     ++consecutiveBadSteps;
     fprintf(stderr,YELLOW "%07u | %0.1f | %0.2f  |  %0.2f  |  %0.2f \n" NORMAL,i,loss,currentValues[0],currentValues[1],currentValues[2]);
   }



   if (consecutiveBadSteps>maximumConsecutiveBadLoss) { fprintf(stderr,YELLOW "Early Stopping\n" NORMAL); break; }
 }
 unsigned long endTime = GetTickCountMicrosecondsIK();


  fprintf(stderr,"Improved loss from %0.2f to %0.2f ( %0.2f%% ) in %lu microseconds \n",initialLoss,bestLoss, 100 - ( (float) 100* bestLoss/initialLoss ),endTime-startTime);
  fprintf(stderr,"Optimized values changed from %0.2f,%0.2f,%0.2f to %0.2f,%0.2f,%0.2f\n",originalValues[0],originalValues[1],originalValues[2],bestValues[0],bestValues[1],bestValues[2]);
  fprintf(stderr,"correction of %0.2f,%0.2f,%0.2f deg\n",bestValues[0]-originalValues[0],bestValues[1]-originalValues[1],bestValues[2]-originalValues[2]);

   problem->chain[chainID].currentSolution->motion[mIDS[0]] = bestValues[0];
   problem->chain[chainID].currentSolution->motion[mIDS[1]] = bestValues[1];
   problem->chain[chainID].currentSolution->motion[mIDS[2]] = bestValues[2];

  return bestLoss;

}








float iterateChainLoss(
                         struct ikProblem * problem,
                         unsigned int chainID,
                         unsigned int iterations
                        )
{

 copyMotionBuffer(problem->chain[chainID].currentSolution,problem->currentSolution);

 for (unsigned int partID=0; partID<problem->chain[chainID].numberOfParts; partID++)
 {
   if (!problem->chain[chainID].part[partID].endEffector)
   {
    iteratePartLoss(
                    problem,
                    chainID,
                    partID,
                    iterations
                   );
   }
 }

 copyMotionBuffer(problem->currentSolution,problem->chain[chainID].currentSolution);

 return calculateChainLoss(problem,chainID);
}







float approximateTargetFromMotionBuffer(
                                         struct BVH_MotionCapture * mc,
                                         struct simpleRenderer *renderer,
                                         struct MotionBuffer * solution,
                                         float * averageError,
                                         struct BVH_Transform * bvhTargetTransform,
                                         float * initialMAE,
                                         float * finalMAE
                                        )
{

 struct ikProblem problem={0};

 prepareProblem(
                 &problem,
                 mc,
                 renderer,
                 solution,
                 averageError,
                 bvhTargetTransform
                );

  viewProblem(&problem);



  //---------------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------------
  struct BVH_Transform bvhCurrentTransform={0};

  float forcePosition[3];
  forcePosition[0] = solution->motion[0];
  forcePosition[1] = solution->motion[1];
  forcePosition[2] = solution->motion[2];
  solution->motion[0]=0;
  solution->motion[1]=0;
  solution->motion[2]=distance;

  if (
         bvh_loadTransformForMotionBuffer(
                                          mc,
                                          solution->motion,
                                          &bvhCurrentTransform
                                         )
        )
      {
       #if DISCARD_POSITIONAL_COMPONENT
        bvh_removeTranslationFromTransform(
                                            mc,
                                            &bvhCurrentTransform
                                          );
       #endif // DISCARD_POSITIONAL_COMPONENT
        *initialMAE = meanSquaredBVH2DDistace(
                                             mc,
                                             renderer,
                                             1,
                                             0,
                                             &bvhCurrentTransform,
                                             bvhTargetTransform
                                            );

        solution->motion[0]=forcePosition[0];
        solution->motion[1]=forcePosition[1];
        solution->motion[2]=forcePosition[2];
      }

  dumpBVHToSVGFrame(
                     "initial.svg",
                      mc,
                      &bvhCurrentTransform,
                      0,
                      renderer
                     );

  //---------------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------------




  unsigned int maximumIterations=1000;
  float loss;

  for (int t=0; t<3; t++)
  {

  loss = iterateChainLoss(
                          &problem,
                          0,
                          maximumIterations
                         );

   loss = iterateChainLoss(
                            &problem,
                            1,
                            maximumIterations
                          );

   loss = iterateChainLoss(
                            &problem,
                            2,
                            maximumIterations
                          );


   loss = iterateChainLoss(
                            &problem,
                            3,
                            maximumIterations
                          );

   loss = iterateChainLoss(
                            &problem,
                            4,
                            maximumIterations
                          );
  }

   copyMotionBuffer(solution,problem.currentSolution);






  //---------------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------------
  forcePosition[0] = solution->motion[0];
  forcePosition[1] = solution->motion[1];
  forcePosition[2] = solution->motion[2];
  solution->motion[0]=0;
  solution->motion[1]=0;
  solution->motion[2]=distance;

  if (
         bvh_loadTransformForMotionBuffer(
                                          mc,
                                          solution->motion,
                                          &bvhCurrentTransform
                                         )
        )
      {
       #if DISCARD_POSITIONAL_COMPONENT
        bvh_removeTranslationFromTransform(
                                            mc,
                                            &bvhCurrentTransform
                                          );
       #endif // DISCARD_POSITIONAL_COMPONENT
        *finalMAE  = meanSquaredBVH2DDistace(
                                             mc,
                                             renderer,
                                             1,
                                             0,
                                             &bvhCurrentTransform,
                                             bvhTargetTransform
                                            );


        solution->motion[0]=forcePosition[0];
        solution->motion[1]=forcePosition[1];
        solution->motion[2]=forcePosition[2];

      }
  //---------------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------------

  dumpBVHToSVGFrame(
                     "target.svg",
                      mc,
                      bvhTargetTransform,
                      1,
                      renderer
                     );

  dumpBVHToSVGFrame(
                     "solution.svg",
                      mc,
                      &bvhCurrentTransform,
                      0,
                      renderer
                     );




 return loss;
}





//./BVHTester --from Motions/05_01.bvh --selectJoints 0 23 hip eye.r eye.l abdomen chest neck head rshoulder relbow rhand lshoulder lelbow lhand rhip rknee rfoot lhip lknee lfoot toe1-2.r toe5-3.r toe1-2.l toe5-3.l --testIK 4 100

int BVHTestIK(
              struct BVH_MotionCapture * mc,
              unsigned int fIDSource,
              unsigned int fIDTarget
             )
{
  int result=0;

  struct BVH_Transform bvhTargetTransform={0};

  struct simpleRenderer renderer={0};
  simpleRendererDefaults(
                         &renderer,
                         1920, 1080, 582.18394,   582.52915 // https://gopro.com/help/articles/Question_Answer/HERO4-Field-of-View-FOV-Information
                        );
  simpleRendererInitialize(&renderer);

  fprintf(stderr,"BVH file has motion files with %u elements\n",mc->numberOfValuesPerFrame);

  float initialMAE=0.0,finalMAE=0.0;

  //Load all motion buffers
  struct MotionBuffer * groundTruth     = mallocNewSolutionBuffer(mc);
  struct MotionBuffer * initialSolution = mallocNewSolutionBuffer(mc);
  struct MotionBuffer * solution        = mallocNewSolutionBuffer(mc);


  if ( (solution!=0) && (groundTruth!=0) && (initialSolution!=0) )
  {
    if (
          ( bvh_copyMotionFrameToMotionBuffer(mc,initialSolution,fIDSource) ) &&
          ( bvh_copyMotionFrameToMotionBuffer(mc,solution,fIDSource) ) &&
          ( bvh_copyMotionFrameToMotionBuffer(mc,groundTruth,fIDTarget) )
       )
      {
       initialSolution->motion[0]=0;
       initialSolution->motion[1]=0;
       initialSolution->motion[2]=distance;


       solution->motion[0]=0;
       solution->motion[1]=0;
       solution->motion[2]=distance;


       groundTruth->motion[0]=0;
       groundTruth->motion[1]=0;
       groundTruth->motion[2]=distance;


        if ( bvh_loadTransformForMotionBuffer(mc,groundTruth->motion,&bvhTargetTransform) )
         {
            #if DISCARD_POSITIONAL_COMPONENT
            bvh_removeTranslationFromTransform(
                                                mc,
                                                &bvhTargetTransform
                                              );
            #endif // DISCARD_POSITIONAL_COMPONENT


            float error2D = approximateTargetFromMotionBuffer(
                                                              mc,
                                                              &renderer,
                                                              solution,
                                                              0,
                                                              &bvhTargetTransform,
                                                              &initialMAE,
                                                              &finalMAE
                                                             );

            fprintf(stderr,"Final 2D Distance is %0.2f\n",error2D);
            result=1;


            //-------------------------------------------------------------------------------------------------------------
            //compareMotionBuffers("The problem we want to solve compared to the initial state",initialSolution,groundTruth);
            //compareMotionBuffers("The solution we proposed compared to ground truth",solution,groundTruth);
            //-------------------------------------------------------------------------------------------------------------
            compareTwoMotionBuffers(mc,"Improvement",initialSolution,solution,groundTruth);

            fprintf(stderr,"MAE went from %0.2f to %0.2f \n",initialMAE,finalMAE);
         }
      }
    freeSolutionBuffer(solution);
    freeSolutionBuffer(initialSolution);
    freeSolutionBuffer(groundTruth);
  }

 return result;
}



























































//https://www.gamasutra.com/blogs/LuisBermudez/20170804/303066/3_Simple_Steps_to_Implement_Inverse_Kinematics.php
//https://groups.csail.mit.edu/drl/journal_club/papers/033005/buss-2004.pdf
//https://simtk-confluence.stanford.edu/display/OpenSim/How+Inverse+Kinematics+Works
int mirrorBVHThroughIK(
                       struct BVH_MotionCapture * mc,
                       struct BVH_Transform * bvhTransform,
                       unsigned int fID,
                       struct simpleRenderer * renderer,
                       BVHJointID jIDA,
                       BVHJointID jIDB
                      )
{
  fprintf(stderr,"NOT IMPLEMENTED YET..");
  //Todo mirror 2D points in 2D and then perform IK..
  return 0;
}




int bvh_MirrorJointsThroughIK(
                               struct BVH_MotionCapture * mc,
                               const char * jointNameA,
                               const char * jointNameB
                             )
{
  BVHJointID jIDA,jIDB;

  if (
       (!bvh_getJointIDFromJointNameNocase(mc,jointNameA,&jIDA)) ||
       (!bvh_getJointIDFromJointNameNocase(mc,jointNameB,&jIDB))
     )
  {
    fprintf(stderr,"bvh_MirrorJointsThroughIK error resolving joints (%s,%s) \n",jointNameA,jointNameB);fprintf(stderr,"Full list of joints is : \n");
    unsigned int jID=0;
     for (jID=0; jID<mc->jointHierarchySize; jID++)
      {
        fprintf(stderr,"   joint %u = %s\n",jID,mc->jointHierarchy[jID].jointName);
      }
    return 0;
  }


 struct BVH_Transform bvhTransform={0};
 struct simpleRenderer renderer={0};
 simpleRendererDefaults(
                        &renderer,
                        1920, 1080, 582.18394,   582.52915 // https://gopro.com/help/articles/Question_Answer/HERO4-Field-of-View-FOV-Information
                       );
 simpleRendererInitialize(&renderer);

 BVHFrameID fID=0;
 for (fID=0; fID<mc->numberOfFrames; fID++)
         {
            mirrorBVHThroughIK(
                                mc,
                                &bvhTransform,
                                fID,
                                &renderer,
                                jIDA,
                                jIDB
                               );
         }


 return 1;
}
