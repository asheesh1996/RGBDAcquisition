#ifndef BVH_TO_SVG_H_INCLUDED
#define BVH_TO_SVG_H_INCLUDED


#include "../bvh_loader.h"
#include "../bvh_transform.h"
#include "../../../../../../tools/AmMatrix/simpleRenderer.h"

#ifdef __cplusplus
extern "C"
{
#endif

int dumpBVHToSVGFrame(
                      const char * svgFilename,
                      struct BVH_MotionCapture * mc,
                      struct BVH_Transform * bvhTransform,
                      unsigned int fID,
                      struct simpleRenderer * renderer
                     );

#ifdef __cplusplus
}
#endif

#endif // BVH_TO_SVG_H_INCLUDED
