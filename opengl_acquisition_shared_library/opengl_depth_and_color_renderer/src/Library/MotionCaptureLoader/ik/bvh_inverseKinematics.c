/*
 * bvh_inverseKinematics.c
 *
 * This file contains an implementation of my inverse kinematics algorithm
 *
 * Ammar Qammaz, May 2020
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include <time.h>

#include "hardcodedProblems_inverseKinematics.h"
#include "bvh_inverseKinematics.h"
#include "levmar.h"

#include "../export/bvh_to_svg.h"
#include "../edit/bvh_cut_paste.h"


// --------------------------------------------
#include <errno.h>
// --------------------------------------------

#define NORMAL   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */

  

unsigned long tickBaseIK = 0;


void clear_line()
{
    fputs("\033[A\033[2K\033[A\033[2K",stdout);
    rewind(stdout);
    int i=ftruncate(1,0);
    if (i!=0)
    {
        /*fprintf(stderr,"Error with ftruncate\n");*/
    }
}

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


float getSquared3DPointDistance(float aX,float aY,float aZ,float bX,float bY,float bZ)
{
    float diffX = (float) aX-bX;
    float diffY = (float) aY-bY;
    float diffZ = (float) aZ-bZ;
    //We calculate the distance here..!
    return (diffX*diffX) + (diffY*diffY) + (diffZ*diffZ);
}


float get3DPointDistance(float aX,float aY,float aZ,float bX,float bY,float bZ)
{
    return sqrt(getSquared3DPointDistance(aX,aY,aZ,bX,bY,bZ));
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



float meanBVH2DDistance(
    struct BVH_MotionCapture * mc,
    struct simpleRenderer *renderer,
    int useAllJoints,
    BVHMotionChannelID onlyConsiderChildrenOfThisJoint,
    struct BVH_Transform * bvhSourceTransform,
    struct BVH_Transform * bvhTargetTransform,
    unsigned int verbose
)
{
    if (verbose)
    {
        fprintf(stderr,"\nmeanBVH2DDistance\n");
    }
    if (bvh_projectTo2D(mc,bvhSourceTransform,renderer,0,0))
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

            if ( (useAllJoints) || ( (isSelected) && (mc->jointHierarchy[jID].parentJoint == onlyConsiderChildrenOfThisJoint) ) )
            {
                ///Warning: When you change this please change calculateChainLoss as well!
                float sX=bvhSourceTransform->joint[jID].pos2D[0];
                float sY=bvhSourceTransform->joint[jID].pos2D[1];
                float tX=bvhTargetTransform->joint[jID].pos2D[0];
                float tY=bvhTargetTransform->joint[jID].pos2D[1];

                if (   
                         (  (sX!=0.0) || (sY!=0.0) ) && (  (tX!=0.0) || (tY!=0.0) ) 
                    )
                {
                    float this2DDistance=get2DPointDistance(sX,sY,tX,tY);
                    
                    if (verbose)
                    {
                        fprintf(stderr,"src(%0.1f,%0.1f)->tar(%0.1f,%0.1f) : ",sX,sY,tX,tY);
                        fprintf(stderr,"2D %s distance = %0.1f\n",mc->jointHierarchy[jID].jointName,this2DDistance);
                    }

                    numberOfSamples+=1;
                    sumOf2DDistances+=this2DDistance;
                }  
            }
        }
        if (verbose)
        {
            fprintf(stderr,"\n");
        }

        if (numberOfSamples>0)
        {
            return (float)  sumOf2DDistances/numberOfSamples;
        }
    } //-----------------

    return 0;
}



float meanBVH3DDistance(
    struct BVH_MotionCapture * mc,
    struct simpleRenderer *renderer,
    int useAllJoints,
    BVHMotionChannelID onlyConsiderChildrenOfThisJoint,
    float * sourceMotionBuffer,
    struct BVH_Transform * bvhSourceTransform,
    float * targetMotionBuffer,
    struct BVH_Transform * bvhTargetTransform
)
{

    if (targetMotionBuffer==0)
    {
        return NAN;
    }

    if (
        (
            performPointProjectionsForMotionBuffer(
                mc,
                bvhSourceTransform,
                sourceMotionBuffer,
                renderer,
                0,
                0
            )
        ) &&
        (
            performPointProjectionsForMotionBuffer(
                mc,
                bvhTargetTransform,
                targetMotionBuffer,
                renderer,
                0,
                0
            )
        )
    )
    {
        //-----------------
        float sumOf3DDistances=0.0;
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
                float tX=bvhTargetTransform->joint[jID].pos3D[0];
                float tY=bvhTargetTransform->joint[jID].pos3D[1];
                float tZ=bvhTargetTransform->joint[jID].pos3D[2];

                if ( (tX!=0.0) || (tY!=0.0) || (tZ!=0.0) )
                {
                    float this3DDistance=get3DPointDistance(
                                             (float) bvhSourceTransform->joint[jID].pos3D[0],
                                             (float) bvhSourceTransform->joint[jID].pos3D[1],
                                             (float) bvhSourceTransform->joint[jID].pos3D[2],
                                             (float) tX,
                                             (float) tY,
                                             (float) tZ
                                         );

                    fprintf(stderr,"src(%0.1f,%0.1f,%0.1f)->tar(%0.1f,%0.1f,%0.1f) : ",(float) bvhSourceTransform->joint[jID].pos3D[0],
                            (float) bvhSourceTransform->joint[jID].pos3D[1],
                            (float) bvhSourceTransform->joint[jID].pos3D[2],
                            (float) tX,
                            (float) tY,
                            (float) tZ);
                    fprintf(stderr," %s distance^2 = %0.1f\n",mc->jointHierarchy[jID].jointName,this3DDistance);

                    numberOfSamples+=1;
                    sumOf3DDistances+=this3DDistance;
                }
            }
        }

        if (numberOfSamples>0)
        {
            return (float)  sumOf3DDistances/numberOfSamples;
        }
    } //-----------------

    return 0.0;
}


int updateProblemSolutionToAllChains(struct ikProblem * problem,struct MotionBuffer * updatedSolution)
{
    if (updatedSolution==0)                     { return 0; }
    if (problem->currentSolution==0)  { return 0; }
    if (problem->initialSolution==0)     { return 0; }
    
    //Actual copy
    if (!copyMotionBuffer(problem->currentSolution,updatedSolution) )  { return 0; }
    if (!copyMotionBuffer(problem->initialSolution,updatedSolution) )      { return 0; }

    for (unsigned int chainID=0; chainID<problem->numberOfChains; chainID++)
    {
        if (!copyMotionBuffer(problem->chain[chainID].currentSolution,updatedSolution))
        {
            return 0;
        }
    }
    return 1;
}

int cleanProblem(struct ikProblem * problem)
{
    freeMotionBuffer(problem->previousSolution);
    freeMotionBuffer(problem->initialSolution);
    freeMotionBuffer(problem->currentSolution);

    for (unsigned int chainID=0; chainID<problem->numberOfChains; chainID++)
    {
        freeMotionBuffer(problem->chain[chainID].currentSolution);
        //Terminate all threads..  
        problem->chain[chainID].terminate=1;
        problem->chain[chainID].threadIsSpawned=0;
    }
   
    return 1;
}


int viewProblem(struct ikProblem * problem)
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
                fprintf(stderr,"jID(%s/%u)->EndEffector ",problem->mc->jointHierarchy[jID].jointName,jID);
            }
            else
            {
                fprintf(stderr,"jID(%s/%u)->mID(%u to %u) ",
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
                                                   unsigned int chainID,
                                                   unsigned int partIDStart
                                                 )
{
    unsigned int numberOfSamples=0;
    float loss=0;
    if (chainID<problem->numberOfChains)
    {
        if (partIDStart >= problem->chain[chainID].numberOfParts)
        {
           fprintf(stderr,"Chain %u has  too few parts ( %u ) \n",chainID,problem->chain[chainID].numberOfParts);
        }
        else          
        if (
              bvh_loadTransformForMotionBuffer(
                                                                                          problem->mc,
                                                                                          problem->chain[chainID].currentSolution->motion,
                                                                                          &problem->chain[chainID].current2DProjectionTransform,
                                                                                          0//Dont populate extra structures we dont need them they just take time
                                                                                        )
            )
        {
            if  (bvh_projectTo2D(problem->mc,&problem->chain[chainID].current2DProjectionTransform,problem->renderer,0,0))
            {
                for (unsigned int partID=partIDStart; partID<problem->chain[chainID].numberOfParts; partID++)
                {
                        unsigned int jID=problem->chain[chainID].part[partID].jID;
                         
                        ///Warning: When you change this please change meanBVH2DDistance as well!
                        float sX=(float) problem->chain[chainID].current2DProjectionTransform.joint[jID].pos2D[0];
                        float sY=(float) problem->chain[chainID].current2DProjectionTransform.joint[jID].pos2D[1];
                        float tX =(float) problem->bvhTarget2DProjectionTransform->joint[jID].pos2D[0];
                        float tY =(float) problem->bvhTarget2DProjectionTransform->joint[jID].pos2D[1];
                        
                        //Only use source/target joints  that exist and are not occluded.. 
                        if ( ((sX!=0.0) || (sY!=0.0)) && ((tX!=0.0) || (tY!=0.0)) )
                        { 
                            loss+= getSquared2DPointDistance(sX,sY,tX,tY) * problem->chain[chainID].part[partID].jointImportance;
                            ++numberOfSamples;
                        }
                } //We add ever part of this chain
            } else  // We successfully projected the BVH file to 2D points..
           { fprintf(stderr,RED "Could not calculate transform projections to 2D for chain %u \n"NORMAL,chainID); }
        } else //Have a valid 2D transform
       { fprintf(stderr,RED "Could not calculate transform for chain %u is invalid\n"NORMAL,chainID); }
    } else //Have a valid chain
    { fprintf(stderr,RED "Chain %u is invalid\n"NORMAL,chainID); }
    //I have left 0/0 on purpose to cause NaNs when projection errors occur
    //----------------------------------------------------------------------------------------------------------
    if (numberOfSamples!=0) { loss = (float) loss/numberOfSamples; }  else
                                                       { loss = NAN; }
    //----------------------------------------------------------------------------------------------------------
    return loss;
}


float iteratePartLoss(
                                           struct ikProblem * problem,
                                           unsigned int iterationID,
                                           unsigned int chainID,
                                           unsigned int partID,
                                           float lr,
                                           float maximumAcceptableStartingLoss,
                                           unsigned int epochs,
                                           unsigned int tryMaintainingLocalOptima,
                                           float spring,
                                           float gradientExplosionThreshold,
                                           unsigned int verbose
                                          )
{
    unsigned long startTime = GetTickCountMicrosecondsIK();

    unsigned int mIDS[3] =
    {
        problem->chain[chainID].part[partID].mIDStart,
        problem->chain[chainID].part[partID].mIDStart+1,
        problem->chain[chainID].part[partID].mIDStart+2
    };


    float originalValues[3] = 
    {
         problem->chain[chainID].currentSolution->motion[mIDS[0]],
         problem->chain[chainID].currentSolution->motion[mIDS[1]],
         problem->chain[chainID].currentSolution->motion[mIDS[2]]
    }; 
    
    //Shorthand to access joint ID and joint Name witout having to traverse the problem 
    unsigned int jointID = problem->chain[chainID].part[partID].jID;
    const char * jointName = problem->mc->jointHierarchy[jointID].jointName;
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------
  
    //Our armature has 500 d.o.f, if we do a calculateChainLoss this will calculate each and every one of them!!
    //Obviously we want to be really fast so we can't afford this, in order to speed up computations we will need to transform all parent joints
    //until the end joint of our chain...  iterateChainLoss
    float initialLoss = calculateChainLoss(problem,chainID,partID);

    ///Having calculated all these joints from here on we only need to update this joint and its children ( and we dont care about their parents since they dont change .. )
    bvh_markJointAsUsefulAndParentsAsUselessInTransform(problem->mc,&problem->chain[chainID].current2DProjectionTransform,jointID);
   //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

 
 //Some reasons not to perform optimization is starting from NaN, starting from 0 or starting with a very high  loss
   if (initialLoss!=initialLoss)
   {
       fprintf(stderr,RED "Started with a NaN loss whill processing chain %u for joint %s \n" NORMAL,chainID,jointName);
       bvh_printNotSkippedJoints(problem->mc,&problem->chain[chainID].current2DProjectionTransform);
       return initialLoss;
   }
      else
    if (initialLoss==0.0)
    {
        //If our loss is perfect we obviously can't improve it..
        if (verbose)
               { fprintf(stderr, GREEN"\nWon't optimize %s,  already perfect\n" NORMAL,jointName); }

        return initialLoss;
    }
       else
    if (maximumAcceptableStartingLoss>0.0)
    {
        //The positional subproblem gets a pass to help the other joints..
        int isItThePositionalSubproblem = ( (partID==0) && ( (chainID==0) || chainID==1) ); 
        
        //If we are really.. really.. far from the solution we might not want to try and do IK
        //as it will improve loss but may lead to a weird incorrect pose 
        if ( (initialLoss>maximumAcceptableStartingLoss) && (!isItThePositionalSubproblem) ) //Dont do that chain
        {
            if (verbose)
                    { fprintf( stderr, RED"\nWon't optimize %s,  exceeded maximum acceptable starting loss by %0.2f%%\n" NORMAL,jointName, ((float) 100*initialLoss/maximumAcceptableStartingLoss) ); }

            return initialLoss;
        }
    } //Careful dont add another else here..
 //------------------------


    if (verbose)
          { fprintf(stderr,"\nOptimizing %s (initial loss %0.2f, iteration %u , chain %u, part %u)\n",jointName,initialLoss,iterationID,chainID,partID); }

//-------------------------------------------
//-------------------------------------------
//-------------------------------------------
//We only need to do this on the first iteration
//We dont want to constantly overwrite values with previous solutions and sabotage next iterations
if (iterationID==0)
{
    //If the previous solution is not given then it is impossible to use it
    if  ( (problem->previousSolution!=0) && (problem->previousSolution->motion!=0) )
    { 
           //We need to remember the initial solution we where given 
            float rememberInitialSolution[3]={
                                                                                  problem->chain[chainID].currentSolution->motion[mIDS[0]],
                                                                                  problem->chain[chainID].currentSolution->motion[mIDS[1]],
                                                                                  problem->chain[chainID].currentSolution->motion[mIDS[2]] 
                                                                                }; 
            
            //Maybe previous solution is closer to current?
            problem->chain[chainID].currentSolution->motion[mIDS[0]] = (float) problem->previousSolution->motion[mIDS[0]];
            problem->chain[chainID].currentSolution->motion[mIDS[1]] = (float) problem->previousSolution->motion[mIDS[1]];
            problem->chain[chainID].currentSolution->motion[mIDS[2]] = (float) problem->previousSolution->motion[mIDS[2]];
            float previousLoss = calculateChainLoss(problem,chainID,partID);
            
            if (previousLoss<initialLoss)
            {
                //Congratulations! better solution for free!
                 if (verbose)
                  { fprintf(stderr,GREEN "Previous solution for joint %s loss (%0.2f) is better than current (%0.2f) \n" NORMAL,jointName,previousLoss,initialLoss); }
                originalValues[0] = problem->chain[chainID].currentSolution->motion[mIDS[0]];
                originalValues[1] = problem->chain[chainID].currentSolution->motion[mIDS[1]];
                originalValues[2] = problem->chain[chainID].currentSolution->motion[mIDS[2]];
                //lr/=10;
                initialLoss = previousLoss;
            } else
            {
                //Previous solution is a worse solution,  let's forget about it and revert back!
               problem->chain[chainID].currentSolution->motion[mIDS[0]] = rememberInitialSolution[0];
               problem->chain[chainID].currentSolution->motion[mIDS[1]] = rememberInitialSolution[1];
               problem->chain[chainID].currentSolution->motion[mIDS[2]] = rememberInitialSolution[2]; 
            } 
    }
} 
//-------------------------------------------
//-------------------------------------------
//-------------------------------------------

    float previousValues[3] = {  originalValues[0],originalValues[1],originalValues[2] } ;
    float currentValues[3]    = {  originalValues[0],originalValues[1],originalValues[2] } ;
    float bestValues[3]          = {  originalValues[0],originalValues[1],originalValues[2] } ;

    float previousLoss[3]      = { initialLoss, initialLoss, initialLoss };
    float currentLoss[3]         = { initialLoss, initialLoss, initialLoss };
    float previousDelta[3]    = {0.0,0.0,0.0};
    float gradient[3]               = {0.0,0.0,0.0};

    float bestLoss = initialLoss;
    //float loss=initialLoss;

    //Gradual fine tuning.. On a first glance it works worse..
    //lr = lr / iterationID;

    unsigned int consecutiveBadSteps=0;
    float minimumLossDeltaFromBestToBeAcceptable = 0.0; //Just be better than best..
    unsigned int maximumConsecutiveBadEpochs=3;
    float e=0.000001;
    float d=lr; //0.0005;
    float beta = 0.9; // Momentum
    float distanceFromInitial; 


//Give an initial direction..
    float delta[3]= {d,d,d};


///--------------------------------------------------------------------------------------------------------------
///--------------------------------------------------------------------------------------------------------------
///--------------------------------------------------------------------------------------------------------------
///--------------------------------------------------------------------------------------------------------------
    if (tryMaintainingLocalOptima)
    {
       //Are we at a global optimum? ---------------------------------------------------------------------------------
       //Do we care ? ----------------------------------------------------------------------------------
        unsigned int badLosses=0;
        for (unsigned int i=0; i<3; i++)
        {
            float rememberOriginalValue =  problem->chain[chainID].currentSolution->motion[mIDS[i]];
            problem->chain[chainID].currentSolution->motion[mIDS[i]] = currentValues[i]+d;
            float lossPlusD=calculateChainLoss(problem,chainID,partID);
            problem->chain[chainID].currentSolution->motion[mIDS[i]] = currentValues[i]-d;
            float lossMinusD=calculateChainLoss(problem,chainID,partID);
            problem->chain[chainID].currentSolution->motion[mIDS[i]] = rememberOriginalValue;

            if ( (initialLoss<=lossPlusD) && (initialLoss<=lossMinusD) )
            {
                if (verbose)  { fprintf(stderr,"Initial #%u value seems to be locally optimal..!\n",i); }
                delta[i] = d;
                ++badLosses;
            }
            else if ( (lossPlusD<initialLoss) && (lossPlusD<=lossMinusD) )
            {
                if (verbose)  { fprintf(stderr,"Initial #%u needs to be positively changed..!\n",i); }
                delta[i] = d;
            }
            else if ( (lossMinusD<initialLoss) && (lossMinusD<=lossPlusD) )
            {
                if (verbose) { fprintf(stderr,"Initial #%u needs to be negatively changed..!\n",i); }
                delta[i] = -d;
            }
            else
            {
                if (verbose)
                {
                    fprintf(stderr,RED "Dont know what to do with #%u value ..\n" NORMAL,i);
                    fprintf(stderr,"-d = %0.2f,   +d = %0.2f, original = %0.2f\n",lossMinusD,lossPlusD,initialLoss);
                    delta[i] = d;
                    ++badLosses;
                }
            }
        }
        if (badLosses==3)
        {
            //We tried nudging all parameters both ways and couldn't improve anything
            //We are at a local optima and  since tryMaintainingLocalOptima is enabled
            //we will try to maintain it..!
            if (verbose) { fprintf(stderr, YELLOW "Maintaining local optimum and leaving joint with no change..!\n" NORMAL); }
              
            return initialLoss;
        }

//-------------------------------------------------------------------------------------------------------------
    }
///--------------------------------------------------------------------------------------------------------------
///--------------------------------------------------------------------------------------------------------------
///--------------------------------------------------------------------------------------------------------------
///--------------------------------------------------------------------------------------------------------------


    if (verbose)
    {
        fprintf(stderr,"  State |   loss   | rX  |  rY  |  rZ \n");
        fprintf(stderr,"Initial | %0.1f | %0.2f  |  %0.2f  |  %0.2f \n",initialLoss,originalValues[0],originalValues[1],originalValues[2]);
    }


    unsigned int executedEpochs=epochs;
    for (unsigned int currentEpoch=0; currentEpoch<epochs; currentEpoch++)
    {
        //Calculate losses
        //-------------------  -------------------  -------------------  -------------------  -------------------  -------------------  -------------------  
        problem->chain[chainID].currentSolution->motion[mIDS[0]] = currentValues[0];
        distanceFromInitial=fabs(currentValues[0] - originalValues[0]);
        currentLoss[0]=calculateChainLoss(problem,chainID,partID) + spring * distanceFromInitial * distanceFromInitial;
        problem->chain[chainID].currentSolution->motion[mIDS[0]] = previousValues[0];
        //-------------------  -------------------  -------------------  -------------------  -------------------  -------------------  -------------------  
        problem->chain[chainID].currentSolution->motion[mIDS[1]] = currentValues[1];
        distanceFromInitial=fabs(currentValues[1] - originalValues[1]);
        currentLoss[1]=calculateChainLoss(problem,chainID,partID) + spring * distanceFromInitial * distanceFromInitial;
        problem->chain[chainID].currentSolution->motion[mIDS[1]] = previousValues[1];
        //-------------------  -------------------  -------------------  -------------------  -------------------  -------------------  -------------------  
        problem->chain[chainID].currentSolution->motion[mIDS[2]] = currentValues[2];
        distanceFromInitial=fabs(currentValues[2] - originalValues[2]);
        currentLoss[2]=calculateChainLoss(problem,chainID,partID) + spring * distanceFromInitial * distanceFromInitial;
        problem->chain[chainID].currentSolution->motion[mIDS[2]] = previousValues[2];
        //-------------------  -------------------  -------------------  -------------------  -------------------  -------------------  -------------------  

        
        //We multiply by 0.5 to do a "One Half Mean Squared Error"
        //-------------------  -------------------  -------------------  -------------------  -------------------  -------------------  -------------------  
        previousDelta[0]=delta[0]; 
        gradient[0] =  (float) 0.5 * (previousLoss[0] - currentLoss[0]) / (delta[0]+e);
        delta[0] =  beta * delta[0] + (float) lr * gradient[0];
        //-------------------  -------------------  -------------------  -------------------  -------------------  -------------------  -------------------  
        previousDelta[1]=delta[1]; 
        gradient[1] =  (float) 0.5 * (previousLoss[1] - currentLoss[1]) / (delta[1]+e);
        delta[1] =  beta * delta[1] + (float) lr * gradient[1];
        //-------------------  -------------------  -------------------  -------------------  -------------------  -------------------  -------------------  
        previousDelta[2]=delta[2]; 
        gradient[2] =  (float) 0.5 * (previousLoss[2] - currentLoss[2]) / (delta[2]+e);
        delta[2] =  beta * delta[2] + (float) lr * gradient[2];
        //-------------------  -------------------  -------------------  -------------------  -------------------  -------------------  -------------------  




        
        if  ( 
                //Safeguard agains gradient explosions which we detect when we see large gradients  
                 (fabs(delta[0]>gradientExplosionThreshold)) || 
                 (fabs(delta[1]>gradientExplosionThreshold)) || 
                 (fabs(delta[2]>gradientExplosionThreshold)) ||  
                 //Safeguard against NaNs
                 (delta[0]!=delta[0]) || 
                 (delta[1]!=delta[1]) || 
                 (delta[2]!=delta[2]) 
             )
        {
            fprintf(stderr,RED "EXPLODING GRADIENT @ %s %u/%u!\n" NORMAL,jointName,currentEpoch,epochs);
            if (verbose)
            {
             fprintf(stderr,RED "previousDeltas[%0.2f,%0.2f,%0.2f]\n" NORMAL,previousDelta[0],previousDelta[1],previousDelta[2]);
             fprintf(stderr,RED "currentDeltas[%0.2f,%0.2f,%0.2f]\n" NORMAL,delta[0],delta[1],delta[2]);
             fprintf(stderr,RED "gradients[%0.2f,%0.2f,%0.2f]\n" NORMAL,gradient[0],gradient[1],gradient[2]);
             fprintf(stderr,RED "previousLoss[%0.2f,%0.2f,%0.2f]\n" NORMAL,previousLoss[0],previousLoss[1],previousLoss[2]);
             fprintf(stderr,RED "currentLoss[%0.2f,%0.2f,%0.2f]\n" NORMAL,currentLoss[0],currentLoss[1],currentLoss[2]);
             fprintf(stderr,RED "lr = %f beta = %0.2f \n" NORMAL,lr,beta);
            }
             //Just stop after an explosion..
            executedEpochs=currentEpoch;
             break;
        }
/*     else 
        { 
        if  ( 
                //Safeguard agains gradient explosions which we detect when we see large gradients  
                 (fabs(delta[0]>50)) || 
                 (fabs(delta[1]>50)) || 
                 (fabs(delta[2]>50))  
             )
        {
             fprintf(stderr,YELLOW "gradients[%0.2f,%0.2f,%0.2f]\n" NORMAL,gradient[0],gradient[1],gradient[2]);
             fprintf(stderr,YELLOW "currentDeltas[%0.2f,%0.2f,%0.2f]\n" NORMAL,delta[0],delta[1],delta[2]);
             exit(0);
        }
        }
*/
        //Remember previous loss/values 
        previousLoss[0]=currentLoss[0];
        previousLoss[1]=currentLoss[1];
        previousLoss[2]=currentLoss[2];
        //----------------------------------------------
        previousValues[0]=currentValues[0];
        previousValues[1]=currentValues[1];
        previousValues[2]=currentValues[2];
        //----------------------------------------------
        
        //We advance our current state..
        currentValues[0]+=delta[0];
        currentValues[1]+=delta[1];
        currentValues[2]+=delta[2];
        //----------------------------------------------
         
        
        //We store our new values and calculate our new loss
        problem->chain[chainID].currentSolution->motion[mIDS[0]] = currentValues[0];
        problem->chain[chainID].currentSolution->motion[mIDS[1]] = currentValues[1];
        problem->chain[chainID].currentSolution->motion[mIDS[2]] = currentValues[2];
        //----------------------------------------------
        float loss=calculateChainLoss(problem,chainID,partID);
        //----------------------------------------------

        // If loss is NaN
        if (loss!=loss)
        {
            //Immediately terminate when encountering NaN, it will be a waste of resources otherwise
            if (verbose)
                    { fprintf(stderr,RED "%07u |NaN| %0.2f  |  %0.2f  |  %0.2f \n" NORMAL,currentEpoch,currentValues[0],currentValues[1],currentValues[2]); }
            executedEpochs=currentEpoch;
            break;
        } else
        if (loss + minimumLossDeltaFromBestToBeAcceptable < bestLoss)  
        {
            //Loss has been improved..!
            bestLoss=loss;
            bestValues[0]=currentValues[0];
            bestValues[1]=currentValues[1];
            bestValues[2]=currentValues[2];
            consecutiveBadSteps=0;
            if (verbose)
                  { fprintf(stderr,"%07u | %0.1f | %0.2f(%0.2f)  |  %0.2f(%0.2f)  |  %0.2f(%0.2f) \n",currentEpoch,loss,currentValues[0],delta[0],currentValues[1],delta[1],currentValues[2],delta[2]); }
        }
        else
        { //Loss has not been improved..!
            ++consecutiveBadSteps;
            if (verbose)
                  { fprintf(stderr,YELLOW "%07u | %0.1f | %0.2f(%0.2f)  |  %0.2f(%0.2f)  |  %0.2f(%0.2f) \n" NORMAL,currentEpoch,loss,currentValues[0],delta[0],currentValues[1],delta[1],currentValues[2],delta[2]); }
        }



        if (consecutiveBadSteps>=maximumConsecutiveBadEpochs)
        {
            if (verbose)
                 { fprintf(stderr,YELLOW "Early Stopping\n" NORMAL); }
            executedEpochs=currentEpoch;
            break;
        }
    }
    unsigned long endTime = GetTickCountMicrosecondsIK();

    if (verbose)
    {
        fprintf(stderr,"Optimization for joint %s \n", jointName);
        fprintf(stderr,"Improved loss from %0.2f to %0.2f ( %0.2f%% ) in %lu microseconds \n",initialLoss,bestLoss, 100 - ( (float) 100* bestLoss/initialLoss ),endTime-startTime);
        fprintf(stderr,"Optimized values changed from %0.2f,%0.2f,%0.2f to %0.2f,%0.2f,%0.2f\n",originalValues[0],originalValues[1],originalValues[2],bestValues[0],bestValues[1],bestValues[2]);
        fprintf(stderr,"correction of %0.2f,%0.2f,%0.2f deg\n",bestValues[0]-originalValues[0],bestValues[1]-originalValues[1],bestValues[2]-originalValues[2]);
        fprintf(stderr,"correction rate of %0.2f,%0.2f,%0.2f deg\n",(bestValues[0]-originalValues[0])/executedEpochs,(bestValues[1]-originalValues[1])/executedEpochs,(bestValues[2]-originalValues[2])/executedEpochs);
    }

    //After finishing with the optimization procedure we store the best result we achieved..!
    problem->chain[chainID].currentSolution->motion[mIDS[0]] = bestValues[0];
    problem->chain[chainID].currentSolution->motion[mIDS[1]] = bestValues[1];
    problem->chain[chainID].currentSolution->motion[mIDS[2]] = bestValues[2];

    return bestLoss;
}





int iterateChainLoss(
                                         struct ikProblem * problem,
                                         unsigned int iterationID,
                                         unsigned int chainID,
                                         float lr,
                                         float maximumAcceptableStartingLoss,
                                         unsigned int epochs,
                                         unsigned int tryMaintainingLocalOptima,
                                         float spring, 
                                         float gradientExplosionThreshold,
                                         unsigned int verbose
                                       )
{
    problem->chain[chainID].status = BVH_IK_STARTED;
   
     //Make sure chain has been fully extended to root joint..
    bvh_markAllJointsAsUselessInTransform(problem->mc,&problem->chain[chainID].current2DProjectionTransform);
    for (unsigned int partID=0; partID<problem->chain[chainID].numberOfParts; partID++)
    {
      unsigned int jointID = problem->chain[chainID].part[partID].jID;
      bvh_markJointAndParentsAsUsefulInTransform(problem->mc,&problem->chain[chainID].current2DProjectionTransform,jointID);
    }
    
    for (unsigned int partID=0; partID<problem->chain[chainID].numberOfParts; partID++)
    {
        if (!problem->chain[chainID].part[partID].endEffector)
        { //If the part is  not an end effector it has parameters to change and improve
            iteratePartLoss(
                                            problem,
                                            iterationID,
                                            chainID,
                                            partID,
                                            lr,
                                            maximumAcceptableStartingLoss,
                                            epochs,
                                            tryMaintainingLocalOptima,
                                            spring, 
                                            gradientExplosionThreshold,
                                            verbose
                                        );
         }
    }

    problem->chain[chainID].status = BVH_IK_FINISHED_ITERATION;

    return 1;
}

 

//This is the regular and easy to follow serial implementation where for each iteration we go through
//each one of the chains in order.. We still mark the chain status to ensure 1:1 operation with the multithreaded
//version of the code.. 
int singleThreadedSolver(
                                         struct ikProblem * problem,
                                         struct ikConfiguration * ikConfig
                                       )
{
  for (unsigned int iterationID=0; iterationID<ikConfig->iterations; iterationID++)
    {
        for (unsigned int chainID=0; chainID<problem->numberOfChains; chainID++)
        {
            //Before we start we will make a copy of the problem->currentSolution to work on improving it..
            copyMotionBuffer(problem->chain[chainID].currentSolution,problem->currentSolution);
    
            problem->chain[chainID].currentIteration=iterationID;
            iterateChainLoss(
                                               problem,
                                               iterationID,
                                               chainID,
                                               ikConfig->learningRate,
                                               ikConfig->maximumAcceptableStartingLoss,
                                               ikConfig->epochs,
                                               ikConfig->tryMaintainingLocalOptima,
                                               ikConfig->spring, 
                                               ikConfig->gradientExplosionThreshold,
                                               ikConfig->verbose
                                             );
             
             //After we finish we update the problem->currentSolution with what our chain came up with..
            copyMotionBuffer(problem->currentSolution,problem->chain[chainID].currentSolution);
        }
    }

   for (unsigned int chainID=0; chainID<problem->numberOfChains; chainID++)
        {
           problem->chain[chainID].status = BVH_IK_FINISHED_EVERYTHING;
        }
   return 1;
}



///=====================================================================================
///=====================================================================================
///=====================================================================================
///                             Start of Multi Threaded Code
///=====================================================================================
///=====================================================================================
///=====================================================================================
 
void * iterateChainLossThread(void * ptr)
{
  //We are a thread so lets retrieve our variables..
  struct passIKContextToThread * ctx = (struct passIKContextToThread *) ptr;
   
   //Instead of doing this here we believe what pthread_create tells us..
  /// ctx->problem->chain[ctx->chainID].threadIsSpawned = 1;
  
  while (!ctx->problem->chain[ctx->chainID].terminate)
  {
     if (ctx->problem->chain[ctx->chainID].permissionToStart)
     {
         // We need a new permission to start again...
         ctx->problem->chain[ctx->chainID].permissionToStart = 0; 
         iterateChainLoss(
                                    ctx->problem,
                                    ctx->problem->chain[ctx->chainID].currentIteration,
                                    ctx->chainID,
                                    ctx->ikConfig->learningRate,
                                    ctx->ikConfig->maximumAcceptableStartingLoss,
                                    ctx->ikConfig->epochs,
                                    ctx->ikConfig->tryMaintainingLocalOptima,
                                    ctx->ikConfig->spring, 
                                    ctx->ikConfig->gradientExplosionThreshold,
                                    ctx->ikConfig->verbose
                                  );      
    }
    usleep(100);
  }
 
  ctx->problem->chain[ctx->chainID].threadIsSpawned=0;
  return 0;
}


//This is the multi threaded version of the code..!
//This is more complex than just spawning one thread per problem because we have to ensure that certain chains get processed in certain order
int multiThreadedSolver(
                                         struct ikProblem * problem,
                                         struct ikConfiguration * ikConfig
                                       )
{
  unsigned int numberOfWorkerThreads = 0;
  
  //Make sure all threads needed are created but only paying the cost of creating a thread once..!
  for (unsigned int chainID=0; chainID<problem->numberOfChains; chainID++)
        { 
              if ( problem->chain[chainID].parallel )
              {
                  if (!problem->chain[chainID].threadIsSpawned)
                  {
                      //Make sure the thread will not terminate just as it starts
                      problem->chain[chainID].terminate=0;
                      
                      //Populate context passed to the thread 
                      problem->workerContext[numberOfWorkerThreads].problem=problem;
                      problem->workerContext[numberOfWorkerThreads].ikConfig=ikConfig;
                      problem->workerContext[numberOfWorkerThreads].chainID=chainID; 
                      
                      //Create the actual thread..
                     int result = pthread_create(&problem->workerPool[numberOfWorkerThreads],0,iterateChainLossThread,(void*) &problem->workerContext[numberOfWorkerThreads]);
                     problem->chain[chainID].threadIsSpawned = (result==0);
                  }
                  ++numberOfWorkerThreads;
              }
        }

  
  //We will perform a number of iterations  each of which have to be synced in the end..
  for (unsigned int iterationID=0; iterationID<ikConfig->iterations; iterationID++)
    {
        //We go through each chain, if the chain is single threaded we do the same as the singleThreadedSolver
        //if the thread is parallel then we just ask it to start processing the current data and we then need to stop and wait to gather results..
        for (unsigned int chainID=0; chainID<problem->numberOfChains; chainID++)
        {
              //Before we start we will make a copy of the problem->currentSolution to work on improving it..
              copyMotionBuffer(problem->chain[chainID].currentSolution,problem->currentSolution);
              
              problem->chain[chainID].currentIteration=iterationID;
              if (!problem->chain[chainID].parallel )
              {  //Normal chains run normally..
                iterateChainLoss(
                                                   problem,
                                                   iterationID,
                                                   chainID,
                                                   ikConfig->learningRate,
                                                   ikConfig->maximumAcceptableStartingLoss,
                                                   ikConfig->epochs,
                                                   ikConfig->tryMaintainingLocalOptima,
                                                   ikConfig->spring, 
                                                   ikConfig->gradientExplosionThreshold,
                                                   ikConfig->verbose
                                                 );
                                                 
                  //After we finish we update the problem->currentSolution with what our chain came up with..
                 copyMotionBuffer(problem->currentSolution,problem->chain[chainID].currentSolution);
              } else
              {
                  //We just give the thread permission to start and we will process its output asynchronously later..
                  problem->chain[chainID].permissionToStart = 1;
              }
        }
        
        //At this point of the code for the particular iteration all single threaded chains have been executed
        //All parallel threads have been started and now we must wait until they are done and gather their output 
        unsigned int allThreadsAreDone=0;
        unsigned int threadsComplete=0;
        unsigned int waitTime=0;
        fprintf(stderr,"\nWaiting for threads to complete : ");
        while (!allThreadsAreDone)
        {
          //Lets check if all our chains are done and copy back their results..!  
          for (unsigned int chainID=0; chainID<problem->numberOfChains; chainID++)
            {
                 if (problem->chain[chainID].parallel )
                 {
                     if (problem->chain[chainID].status == BVH_IK_FINISHED_ITERATION)
                     {
                         ++threadsComplete;                      
                        //Yey..! This thread has finished its iteration, normally we would gather the result by using the copyMotionBuffer call
                        //however this will overwrite the solution of other stuff so we are really only interested in copyint the motion values
                        //from partIDs that do not correspond to endEffectors..
                        
                        for (unsigned int partID=0; partID<problem->chain[chainID].numberOfParts; partID++)
                           {
                                if (!problem->chain[chainID].part[partID].endEffector)
                                   {  
                                       //Shorthand to address the correct motion values
                                       unsigned int mIDS[3] ={
                                                                                       problem->chain[chainID].part[partID].mIDStart,
                                                                                       problem->chain[chainID].part[partID].mIDStart+1,
                                                                                       problem->chain[chainID].part[partID].mIDStart+2
                                                                                      };

                                       //Copy back the solution of the chain to the "official" solution for the  whole  problem
                                       problem->currentSolution->motion[mIDS[0]]=problem->chain[chainID].currentSolution->motion[mIDS[0]];
                                       problem->currentSolution->motion[mIDS[1]]=problem->chain[chainID].currentSolution->motion[mIDS[1]];
                                       problem->currentSolution->motion[mIDS[2]]=problem->chain[chainID].currentSolution->motion[mIDS[2]];
                                    } //If a part is an end effector it has no parameters to copy
                           }  //Copy every part of this chain
                           
                        problem->chain[chainID].status=BVH_IK_NOTSTARTED;
                     } // If thread has finished its iteration  
                 }// If this chain has a thread serving it
            }//Check all chains..
            
            if (numberOfWorkerThreads==threadsComplete)
            {
                allThreadsAreDone=1;
            } else
            {
                usleep(100);
            }
            
            ++waitTime;
            if (waitTime%3==0) { fprintf(stderr,"."); }
         }
    }
    
  //This should make the thread release all of its resources (?)
  //pthread_detach(pthread_self());
}

///=====================================================================================
///=====================================================================================
///=====================================================================================
///                            End of Multi Threaded Code
///=====================================================================================
///=====================================================================================
///=====================================================================================









int ensureInitialPositionIsInFrustrum(
                                                                            struct simpleRenderer *renderer,
                                                                            struct MotionBuffer * solution,
                                                                            struct MotionBuffer * previousSolution
                                                                         )
{
   float closestDistanceToCameraInCM=30; 
    
  //TODO : 
   //Ensure that  pose is not out of the bounds of camera ?
   //If it is inverse kinematics wont know what to do..
    if (solution->motion[2] > -1 * closestDistanceToCameraInCM)
    {
        fprintf(stderr,RED "Warning: Detected pose behind camera! ..\n" NORMAL);
        if ( (previousSolution!=0) && (previousSolution->motion!=0) )
        {
            if (previousSolution->motion[2] < -1 * closestDistanceToCameraInCM)
                    {
                        fprintf(stderr,GREEN "Fixed using previous frame ! ..\n" NORMAL);
                        solution->motion[0]=previousSolution->motion[0]; 
                        solution->motion[1]=previousSolution->motion[1]; 
                        /// This is the most important  --------------------------------
                        solution->motion[2]=previousSolution->motion[2]; 
                        ///-------------------------------------------------------------------------
                        solution->motion[3]=previousSolution->motion[3]; 
                        solution->motion[4]=previousSolution->motion[4]; 
                        solution->motion[5]=previousSolution->motion[5]; 
                    } 
        }

        if (solution->motion[2] > -1 * closestDistanceToCameraInCM)
        {  
                 fprintf(stderr,RED "Warning: Didnt manage to solve problem, brute forcing it ! ..\n" NORMAL);
                 solution->motion[2]=-140;
        }  
    }
     
     return 1;
}



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
                                                                                                                                )
{
    if  ( (solution==0) || (solution->motion==0) )
    {
        fprintf(stderr,RED "No initial solution provided for IK..\n" NORMAL);
        return 0;
    }

    if (ikConfig==0)
    {
        fprintf(stderr,RED "No configuration provided for IK..\n" NORMAL);
        return 0;
    }


    if (ikConfig->ikVersion != (float) IK_VERSION)
    {
        fprintf(stderr,RED "Fatal: IK Version mismatch for configuration structure (%0.2f vs %0.2f ) ..\n" NORMAL,ikConfig->ikVersion,IK_VERSION);
        exit(0);
    }


    ensureInitialPositionIsInFrustrum(renderer,solution,previousSolution);
    
    //Make sure our problem has the correct details ..
    problem->bvhTarget2DProjectionTransform =  bvhTargetTransform;  
      
    #define RELY_ON_PREVIOUS_SOLUTION_MORE_THAN_SOLUTION 0
    
    #if RELY_ON_PREVIOUS_SOLUTION_MORE_THAN_SOLUTION
      updateProblemSolutionToAllChains(problem,previousSolution); 
      if (!copyMotionBuffer(problem->previousSolution,solution) )      { return 0; }        
    #else
      updateProblemSolutionToAllChains(problem,solution);
      if (!copyMotionBuffer(problem->previousSolution,previousSolution) )      { return 0; }         
    #endif
    
     
    
    //Don't spam console..
    //viewProblem(problem);
    
    float previousMAEInPixels=1000000; //Big invalid number
    //---------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------
    struct BVH_Transform bvhCurrentTransform= {0};


    if (bvh_loadTransformForMotionBuffer(mc,problem->initialSolution->motion,&bvhCurrentTransform,0)// We don't need extra structures
    )
    {  
        //----------------------------------------------------
        if (initialMAEInPixels!=0)
        {
            *initialMAEInPixels = meanBVH2DDistance(mc,renderer,1,0,&bvhCurrentTransform,bvhTargetTransform,ikConfig->verbose);
        }
        //----------------------------------------------------


        if ( (initialMAEInMM!=0) && (groundTruth!=0) )
        {
            *initialMAEInMM = meanBVH3DDistance(mc,renderer,1,0,problem->initialSolution->motion,&bvhCurrentTransform,groundTruth->motion,bvhTargetTransform);
        }
        //----------------------------------------------------

    }

    if (ikConfig->dumpScreenshots)
    {
        dumpBVHToSVGFrame("initial.svg",mc,&bvhCurrentTransform,0,renderer);
    }
    //---------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------

     if (useMultipleThreads)
     {
      //Solve the problem using multiple threads..!
      multiThreadedSolver(problem,ikConfig);          
     } else
     {
     //Solve the problem using a single thread..!
      singleThreadedSolver(problem,ikConfig);
     }

    //Retrieve regressed solution
    copyMotionBuffer(solution,problem->currentSolution);

     float * m = problem->initialSolution->motion;
     fprintf(stderr,"IK lr = %f ,  max start loss =%0.1f , Iterations = %u , epochs = %u , spring = %0.1f \n", 
                                               ikConfig->learningRate,
                                               ikConfig->maximumAcceptableStartingLoss,
                                               ikConfig->iterations,
                                               ikConfig->epochs, 
                                               ikConfig->spring
                    );
     fprintf(stderr,"Initial Position/Location was %0.2f,%0.2f,%0.2f %0.2f,%0.2f,%0.2f\n",m[0],m[1],m[2],m[3],m[4],m[5]);

        if  ( (problem->previousSolution!=0) && (problem->previousSolution->motion!=0) )
        { 
            m = problem->previousSolution->motion;
            fprintf(stderr,"Previous Position/Location was %0.2f,%0.2f,%0.2f %0.2f,%0.2f,%0.2f\n",m[0],m[1],m[2],m[3],m[4],m[5]); 
        }
    
    m = solution->motion;
    fprintf(stderr,"Final Position/Location was %0.2f,%0.2f,%0.2f %0.2f,%0.2f,%0.2f\n",m[0],m[1],m[2],m[3],m[4],m[5]);
    //---------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------

    if (
          bvh_loadTransformForMotionBuffer(
                                                                                      mc,
                                                                                      solution->motion,
                                                                                      &bvhCurrentTransform,
                                                                                      0// dont use extra structures
                                                                                     )
      )
    {
        //----------------------------------------------------
        if (finalMAEInPixels!=0)
        {
            *finalMAEInPixels  = meanBVH2DDistance(mc,renderer,1,0,&bvhCurrentTransform,bvhTargetTransform,ikConfig->verbose);
                                                                                                 
            if (previousMAEInPixels<*finalMAEInPixels)
            {
                if (ikConfig->considerPreviousSolution)
                {
                    fprintf(stderr,RED "After all this work we where not smart enough to understand that previous solution was better all along..\n" NORMAL);
                    copyMotionBuffer(solution,previousSolution);
                }
            }
        }
        //----------------------------------------------------
        if ( (finalMAEInMM!=0) && (groundTruth!=0) )
        {
            *finalMAEInMM = meanBVH3DDistance(mc,renderer,1,0,solution->motion,&bvhCurrentTransform,groundTruth->motion,bvhTargetTransform);
        }
        //----------------------------------------------------
    }
    //---------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------

    if (ikConfig->dumpScreenshots)
    {
        dumpBVHToSVGFrame("target.svg",mc,bvhTargetTransform,1,renderer);
        dumpBVHToSVGFrame("solution.svg",mc,&bvhCurrentTransform,0,renderer);
    }



    return 1;
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
        fprintf(stderr,"bvh_MirrorJointsThroughIK error resolving joints (%s,%s) \n",jointNameA,jointNameB);
        fprintf(stderr,"Full list of joints is : \n");
        unsigned int jID=0;
        for (jID=0; jID<mc->jointHierarchySize; jID++)
        {
            fprintf(stderr,"   joint %u = %s\n",jID,mc->jointHierarchy[jID].jointName);
        }
        return 0;
    }

    struct BVH_Transform bvhTransform= {0};
    struct simpleRenderer renderer= {0};
    // https://gopro.com/help/articles/Question_Answer/HERO4-Field-of-View-FOV-Information
    simpleRendererDefaults(&renderer,1920,1080, 582.18394,582.52915);
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
