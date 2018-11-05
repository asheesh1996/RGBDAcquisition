#ifndef SIMPLERENDERER_H_INCLUDED
#define SIMPLERENDERER_H_INCLUDED


struct simpleRenderer
{

  float fx;
  float fy;
  float skew;
  float cx;
  float cy;
  float near;
  float far;
  float width;
  float height;

  float objectOffsetPosition[4];
  float objectOffsetRotation[4];


  float projectionMatrix[16];
  float viewMatrix[16];
  float modelMatrix[16];
  float modelViewMatrix[16];
  int   viewport[4];
};


int simpleRendererRender(
                         struct simpleRenderer * sr ,
                         float * position3D,
                         float * center3D,
                         float * output2DX,
                         float * output2DY
                        );

int simpleRendererInitialize(struct simpleRenderer * sr);

#endif // SIMPLERENDERER_H_INCLUDED
