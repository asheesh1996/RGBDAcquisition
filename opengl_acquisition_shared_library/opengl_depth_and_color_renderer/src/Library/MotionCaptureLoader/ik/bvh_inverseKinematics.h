#ifndef BVH_INVERSEKINEMATICS_H_INCLUDED
#define BVH_INVERSEKINEMATICS_H_INCLUDED


#include "../bvh_loader.h"
#include "../export/bvh_export.h"

#include <pthread.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define IK_VERSION 0.23

#define MAXIMUM_CHAINS 15
#define MAXIMUM_PARTS_OF_CHAIN 15



enum bvhIKSolutionStatus
{
  BVH_IK_NOTSTARTED=0,
  BVH_IK_STARTED,
  BVH_IK_FINISHED_ITERATION,
  BVH_IK_FINISHED_EVERYTHING, 
  //--------------------
  BVH_IK_STATES
};


struct ikChainParts
{
 unsigned int partParent;
 BVHJointID jID;
 float jointImportance;
 BVHMotionChannelID mIDStart;
 BVHMotionChannelID mIDEnd;
 char evaluated;
 char bigChanges;
 char endEffector;
};
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------

struct ikChain
{
  //Thread information  
  // --------------------------------------------------------------------------
  unsigned char parallel;  
  unsigned char currentIteration;
  unsigned char status; // enum bvhIKSolutionStatus
  unsigned char permissionToStart;  
  unsigned char terminate;  
  unsigned char threadIsSpawned;  
  // --------------------------------------------------------------------------
    
    
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



struct passIKContextToThread
{
    struct ikProblem * problem;
    struct ikConfiguration * ikConfig; 
    unsigned int chainID;
};


struct ikProblem
{
 //BVH file that reflects our problem
 struct BVH_MotionCapture * mc;
 //Renderer that handles 3D projections
 struct simpleRenderer *renderer;


 //Previous solution
 struct MotionBuffer * previousSolution;
 //Initial solution
 struct MotionBuffer * initialSolution;
 //Current solution
 struct MotionBuffer * currentSolution;

 //2D Projections Targeted
 struct BVH_Transform * bvhTarget2DProjectionTransform;

 //Chain of subproblems that need to be solved
 unsigned int numberOfChains;
 unsigned int numberOfGroups;
 unsigned int numberOfJobs;

 struct ikChain chain[MAXIMUM_CHAINS];

 //Thread storage..
  pthread_t workerPool[MAXIMUM_CHAINS];
  struct passIKContextToThread workerContext[MAXIMUM_CHAINS];
};
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------



struct ikConfiguration
{
  float maximumAcceptableStartingLoss;
  float learningRate;
  unsigned int iterations;
  unsigned int epochs;
  unsigned int considerPreviousSolution;
  unsigned int tryMaintainingLocalOptima;
  float spring; 
  float gradientExplosionThreshold;
  unsigned int dumpScreenshots;
  unsigned int verbose;
  float ikVersion;
};


int cleanProblem(struct ikProblem * problem);


unsigned long GetTickCountMicrosecondsIK();

//Temporary call that allows outside control..
int approximateBodyFromMotionBufferUsingInverseKinematics(
                                         struct BVH_MotionCapture * mc,
                                         struct simpleRenderer *renderer,
                                         struct ikProblem * problem,
                                         struct ikConfiguration * ikConfig,
                                         //---------------------------------
                                         struct MotionBuffer * previousSolution,
                                         struct MotionBuffer * solution,
                                         struct MotionBuffer * groundTruth,
                                         //---------------------------------
                                         struct BVH_Transform * bvhTargetTransform,
                                         //---------------------------------
                                         unsigned int useMultipleThreads,
                                         //---------------------------------
                                         float * initialMAEInPixels,
                                         float * finalMAEInPixels,
                                         float * initialMAEInMM,
                                         float * finalMAEInMM
                                        );



int bvh_MirrorJointsThroughIK(
                               struct BVH_MotionCapture * mc,
                               const char * jointNameA,
                               const char * jointNameB
                             );

#ifdef __cplusplus
}
#endif

#endif
