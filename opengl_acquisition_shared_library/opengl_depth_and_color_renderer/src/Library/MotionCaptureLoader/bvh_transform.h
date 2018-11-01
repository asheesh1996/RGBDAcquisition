#ifndef BVH_TRANSFORM_H_INCLUDED
#define BVH_TRANSFORM_H_INCLUDED

#include "bvh_loader.h"

/*
   0  1  2  3
   4  5  6  7
   8  9 10 11
  12 13 14 15
*/

struct BVH_TransformedJoint
{
  double localToWorldTransformation[16];
  double chainTransformation[16];
  double dynamicTranslation[16];
  double dynamicRotation[16];

  //Position as X,Y,Z
  //-----------------
  double pos[4];
};


struct BVH_Transform
{
  struct BVH_TransformedJoint joint[MAX_BVH_JOINT_HIERARCHY_SIZE];
};

void create4x4RotationBVH(double * matrix,int rotationType,double degreesX,double degreesY,double degreesZ);


int bvh_loadTransformForFrame(
                               struct BVH_MotionCapture * bvhMotion ,
                               BVHFrameID fID ,
                               struct BVH_Transform * bvhTransform
                             );

#endif // BVH_TRANSFORM_H_INCLUDED
