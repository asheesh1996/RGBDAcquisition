#include <stdio.h>
#include <stdlib.h>
#include "automaticPlaneSegmentation.h"
#include "imageProcessing.h"
#include "../tools/Quasirandomness/quasirandomness.h"

#define MEMPLACE1(x,y,width) ( y * ( width ) + x )
#define GET_RANDOM_DIM(width,bounds) (bounds+rand()%(width-1-bounds))
#define FLOATISZERO(f) ( (f<0.00001) && (f>-0.00001) )

unsigned int minimumAcceptedDepths = 830;
unsigned int maximumAcceptedDepths = 3000;

#define NeighborhoodNormalCombos 4
#define ResultNormals 64
#define MaxTriesPerPoint 100

#define SINGLE_TEST 0

unsigned int neighborhoodHalfWidth = 6;
unsigned int neighborhoodHalfHeight = 6;
enum NEIGHBORHOOD_OF_POINTS
{
   NH_TOPLEFT = 0 ,
   NH_TOPRIGHT ,
   NH_CENTER,
   NH_BOTLEFT,
   NH_BOTRIGHT,
   //---------------
   NH_TOTAL_NEIGHBORS
};

enum pShorthand
{
  X=0,Y,Z
};



struct TriplePoint
{
 float coord[3];
};

struct normalArray
{
    unsigned int originX,originY;
    struct TriplePoint point;
    float normal[3];
};


inline float getDepthValue(unsigned short * source , unsigned int x, unsigned int y , unsigned int width)
{
  return (float) source[MEMPLACE1(x,y,width)];
}


inline int pointAndNormalAreZero(float * point , float * normal)
{
  if (
             (
             (! FLOATISZERO(normal[0]) ) ||
             (! FLOATISZERO(normal[1]) ) ||
             (! FLOATISZERO(normal[2]) )
             ) &&
             (
             (! FLOATISZERO(point[0]) ) ||
             (! FLOATISZERO(point[1]) ) ||
             (! FLOATISZERO(point[2]) )
             )
      )
             {
                  return 0;
             }
  return 1;
}

int decideNormalAround3DPoint(unsigned short * source , struct calibration * calib , unsigned int x , unsigned int y  , unsigned int width , unsigned int height , float * point , float * normal )
{
  //We get a position x,y and we want to decide the normal of this position
  //In order to do so , we have to find some neighboring points calculate the resulting normals
  //and take a mean approximation of the point , then we return

   //---------------------------------------------------
   //        *0                                   *1
   //
   //                  *2 Central Point *
   //
   //        *3                                   *4
   //---------------------------------------------------
   //        Calculated normals ( triangles will be )
   //        0 , 2 , 1 -> Normal 0
   //        0 , 3 , 2 -> Normal 1
   //        2 , 3 , 4 -> Normal 2
   //        1 , 2 , 4 -> Normal 3
   //---------------------------------------------------
   unsigned int normalSeries[NeighborhoodNormalCombos][3] = { { 0, 2, 1 }, { 0, 3, 2 }, { 2, 3, 4 } , { 1, 2, 4 }  };

   struct TriplePoint neighbors[NH_TOTAL_NEIGHBORS]={0};
   struct normalArray neighborNormals[NeighborhoodNormalCombos]={0};
   unsigned int normalOK[NeighborhoodNormalCombos]={0};


   //The central point is of course the one we have !
   neighbors[NH_CENTER].coord[X]=x;
   neighbors[NH_CENTER].coord[Y]=y;
   neighbors[NH_CENTER].coord[Z]=getDepthValue(source,neighbors[NH_CENTER].coord[X],neighbors[NH_CENTER].coord[Y],width);


   if ( (x>neighborhoodHalfWidth) && (y>neighborhoodHalfHeight) )
       { neighbors[NH_TOPLEFT].coord[X]=x-neighborhoodHalfWidth; neighbors[NH_TOPLEFT].coord[Y]=y-neighborhoodHalfHeight;
         neighbors[NH_TOPLEFT].coord[Z]=getDepthValue(source,neighbors[NH_TOPLEFT].coord[X],neighbors[NH_TOPLEFT].coord[Y],width); }

   if ( (x+neighborhoodHalfWidth>width) && (y>neighborhoodHalfHeight) )
      { neighbors[NH_TOPRIGHT].coord[X]=x+neighborhoodHalfWidth; neighbors[NH_TOPRIGHT].coord[Y]=y-neighborhoodHalfHeight;
        neighbors[NH_TOPRIGHT].coord[Z]=getDepthValue(source,neighbors[NH_TOPRIGHT].coord[X],neighbors[NH_TOPRIGHT].coord[Y],width); }

   if ( (x>neighborhoodHalfWidth) && (y+neighborhoodHalfHeight>height) )
       { neighbors[NH_BOTLEFT].coord[X]=x-neighborhoodHalfWidth; neighbors[NH_BOTLEFT].coord[Y]=y+neighborhoodHalfHeight;
         neighbors[NH_BOTLEFT].coord[Z]=getDepthValue(source,neighbors[NH_BOTLEFT].coord[X],neighbors[NH_BOTLEFT].coord[Y],width); }

   if ( (x+neighborhoodHalfWidth>width) && (y+neighborhoodHalfHeight>height) )
      { neighbors[NH_BOTRIGHT].coord[X]=x+neighborhoodHalfWidth; neighbors[NH_BOTRIGHT].coord[Y]=y+neighborhoodHalfHeight;
        neighbors[NH_BOTRIGHT].coord[Z]=getDepthValue(source,neighbors[NH_BOTRIGHT].coord[X],neighbors[NH_BOTRIGHT].coord[Y],width); }



   //Project Points to get real 3D coordinates
   unsigned int i=0;
   unsigned int x2D,y2D,d3D;
   for (i=0; i<NH_TOTAL_NEIGHBORS; i++)
   {
      if (
           (neighbors[i].coord[X]!=0) ||
           (neighbors[i].coord[Y]!=0) ||
           (neighbors[i].coord[Z]!=0)
         )
        {
        x2D = (unsigned int) neighbors[i].coord[X];
        y2D = (unsigned int) neighbors[i].coord[Y];
        d3D = (unsigned int) neighbors[i].coord[Z];

        if (! transform2DProjectedPointTo3DPoint( calib , x2D , y2D  , d3D ,
                                                  &neighbors[i].coord[X] ,
                                                  &neighbors[i].coord[Y] ,
                                                  &neighbors[i].coord[Z]
                                                 )
           )
           {
               fprintf(stderr,"Could not calculate point %u for center %u,%u\n",i,x,y);
           }
        }
   }


   point[0]=neighbors[NH_CENTER].coord[X];
   point[1]=neighbors[NH_CENTER].coord[Y];
   point[2]=neighbors[NH_CENTER].coord[Z];

   //We have an array of normals ready , lets populate the normals now
   for (i=0; i<NeighborhoodNormalCombos; i++)
   {
      if ( //If all three points have a Depth ( thus only Z is checked )
           ( FLOATISZERO(neighbors[normalSeries[i][0]].coord[Z]) ) &&
           ( FLOATISZERO(neighbors[normalSeries[i][1]].coord[Z]) ) &&
           ( FLOATISZERO(neighbors[normalSeries[i][2]].coord[Z]) )
         )
          {
            //This is a zero normal , we dont consider it
            normalOK[i]=0;
          } else
          { //Find the normal
                 crossProductFrom3Points( neighbors[normalSeries[i][0]].coord ,
                                          neighbors[normalSeries[i][1]].coord ,
                                          neighbors[normalSeries[i][2]].coord ,
                                          neighborNormals[i].normal);

               //Mark the normal as found
               if ( ! pointAndNormalAreZero(point,neighborNormals[i].normal) ) {  normalOK[i]=1; }

          }
   }

   unsigned int samples = 0;
   normal[0]=0; normal[1]=0; normal[2]=0;
   for (i=0; i<NeighborhoodNormalCombos; i++)
   {
     if (normalOK[i])
     {
       ++samples;
       #if SINGLE_TEST
        normal[0]=neighborNormals[i].normal[0];
        normal[1]=neighborNormals[i].normal[1];
        normal[2]=neighborNormals[i].normal[2];
        #else
       normal[0]+=neighborNormals[i].normal[0];
       normal[1]+=neighborNormals[i].normal[1];
       normal[2]+=neighborNormals[i].normal[2];
       #endif // SINGLE_TEST
     }
   }

   if (samples>0)
   {

     #if SINGLE_TEST

     #else
      normal[0]/=samples;
      normal[1]/=samples;
      normal[2]/=samples;
     #endif // SINGLE_TEST

     //

     if ( ! pointAndNormalAreZero(point,normal) )
        {
          fprintf(stderr,"Should returned 0.02 -0.83 -0.56 , returned %f %f %f\n",normal[0],normal[1],normal[2]);

          return 1;
        }
   }

   fprintf(stderr,"decideNormalAround3DPoint( %u , %u ) failed with normal %f %f %f \n",x,y,normal[0],normal[1],normal[2]);
   return 0;
}





int automaticPlaneSegmentation(unsigned short * source , unsigned int width , unsigned int height , float offset, struct SegmentationFeaturesDepth * segConf , struct calibration * calib)
{
    fprintf(stderr,"doing automaticPlaneSegmentation( using RANSAC with %u points )\n",ResultNormals);

    unsigned int boundDistance=10;
    unsigned int x,y,depth;
    unsigned int bestNormal = 0;

    if (ResultNormals==0) { fprintf(stderr,"No Normals allowed cannot do automatic plane segmentation \n"); return 0; }

    struct normalArray result[ResultNormals]={0};
    float resultScore[ResultNormals]={0};


    struct TriplePoint legend;
    legend.coord[X]=0.016560; legend.coord[Y]=-0.826509; legend.coord[Z]=-0.562679;

    struct quasiRandomizerContext qrc;
    initializeQuasirandomnessContext(&qrc,width,height,0);
    float rX,rY,rZ;

    unsigned int gotResult=0;
    unsigned int tries=0;
    unsigned int i=0;
    for (i=0; i<ResultNormals; i++)
    {
        tries=0; gotResult=0;
         while ( (!gotResult)  && (tries<MaxTriesPerPoint)  )
         {
          ++tries;
          getNextRandomPoint(&qrc,&rX,&rY,&rZ);

          if ( (0<=rX) && (rX<width) && (0<=rY) && (rY<height) )
          {
           x=(unsigned int) rX%width;
           y=(unsigned int) rY%height;
           if ( (0<=x) && (x<width) && (0<=y) && (y<height) )
           {
            depth=source[MEMPLACE1(x,y,width)];
            if ( (minimumAcceptedDepths<depth) && (depth<maximumAcceptedDepths) && (depth!=0) )
                         {
                           if (decideNormalAround3DPoint(source , calib , x , y  , width , height , result[i].point.coord , result[i].normal ) )
                            {
                                result[i].originX=x;
                                result[i].originY=y;
                                gotResult=1;
                             }
                         }
           }
          }
         }

        if ( pointAndNormalAreZero(result[i].point.coord,result[i].normal) )
                         { fprintf(stderr,"Produced a zero normal after %u tries , wtf \n",tries); }
    } //We now have Populated result[i].normal and result[i].point

    unsigned int z=0;
    float angle=0.0;
    for (i=0; i<ResultNormals; i++)
    {
      for (z=0; z<ResultNormals; z++)
      {
          if (z!=i)
          {
             angle=angleOfNormals(result[i].normal,result[z].normal);
             if (angle<0.0) { angle=-1 * angle; }
             resultScore[i]+=angle;
          }
      }

      //Adding angle penalty according to legend ( to try and match it
      angle=angleOfNormals(result[i].normal,legend.coord);
      if (angle<0.0) { angle=-1 * angle; }
      resultScore[i]+=angle;
      fprintf(stderr,"Point 2D %u (%u,%u) - 3D (%f,%f,%f) - Normal (%f,%f,%f) - score %f\n",i,
                                                                                      result[i].originX,result[i].originY,
                                                                                      result[i].point.coord[0],result[i].point.coord[1],result[i].point.coord[2],
                                                                                      result[i].normal[0], result[i].normal[1], result[i].normal[2] , resultScore[i] );
    }

//Normal segmentation using point 24.677238,256.603088,1019.000000 and normal 0.022093,-0.833547,-0.552007

    float MAX_SCORE = 121230.0;
    float bestScore = MAX_SCORE;
    for (i=0; i<ResultNormals; i++)
    {
      if (
           ( resultScore[i]<bestScore ) && ( !pointAndNormalAreZero(result[i].point.coord,result[i].normal) )
         )
      {
        bestNormal = i;
        bestScore = resultScore[i];
      }
    }

   if (bestScore == MAX_SCORE)
   {
     fprintf(stderr,"Failed to get an automatic plane :( \n");
     return 0;
   }

   segConf->enablePlaneSegmentation=1;
   segConf->planeNormalOffset=offset; //<- this is to ensure a good auto segmentation
   segConf->doNotGenerateNormalFrom3Points=1; // <- we have a normal and a point , we dont have 3 points

   segConf->center[0] = result[bestNormal].point.coord[0];
   segConf->center[1] = result[bestNormal].point.coord[1];
   segConf->center[2] = result[bestNormal].point.coord[2];

   segConf->normal[0] = result[bestNormal].normal[0];
   segConf->normal[1] = result[bestNormal].normal[1];
   segConf->normal[2] = result[bestNormal].normal[2];
   fprintf(stderr,"Picked result %u with score %0.2f \n",bestNormal , bestScore);



   fprintf(stderr,"Best Points are \n point %u,%u , %0.2f %0.2f %0.2f \n normal %0.2f %0.2f %0.2f , offset %f \n" ,
                         result[bestNormal].originX,result[bestNormal].originY,
                         result[bestNormal].point.coord[0] ,  result[bestNormal].point.coord[1] ,  result[bestNormal].point.coord[2] ,
                         result[bestNormal].normal[0] ,  result[bestNormal].normal[1] ,  result[bestNormal].normal[2]
                         ,offset
         );

   fprintf(stderr,"Automatic shutdown of automatic plane segmentation so it does not feed on itself on the next frame\n");
   segConf->autoPlaneSegmentation=0;
  return 1;
}
