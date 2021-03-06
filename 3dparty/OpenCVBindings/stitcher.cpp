#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <stdio.h>
int myBlendImages( cv::Mat & out,
                   cv::Mat & left ,
                   cv::Mat & right ,
                   int blend )
{
   unsigned char lR=0,lG=0,lB=0,rR=0,rG=0,rB=0;
   unsigned int leftBlank,rightBlank;
   unsigned int x=0,y=0,offset=0,width=left.size().width;

   for ( x=0; x<width; x++ )
   {
    for ( y=0; y<left.size().height; y++ )
    {
      offset=y*width*3+x*3;
      lR=left.data[offset+0];  lG=left.data[offset+1];  lB=left.data[offset+2];
      rR=right.data[offset+0]; rG=right.data[offset+1]; rB=right.data[offset+2];

       if ( (  lR==0 ) && ( lG==0 ) && ( lB == 0 )  )  { leftBlank=1; }  else { leftBlank=0; }
       if ( (  rR==0 ) && ( rG==0 ) && ( rB == 0 )  )  { rightBlank=1; } else { rightBlank=0; }

       if ( (  leftBlank ) && ( rightBlank ) )  { } else
       if ( (  !leftBlank ) && ( rightBlank ) )
       {
         out.data[offset+0]=lR;
         out.data[offset+1]=lG;
         out.data[offset+2]=lB;
       } else
       if ( (  leftBlank ) && ( !rightBlank ) )
       {
         out.data[offset+0]=rR;
         out.data[offset+1]=rG;
         out.data[offset+2]=rB;
       } else
       {
         if (blend)
          {
           out.data[offset+0]=(lR+rR)/2;
           out.data[offset+1]=(lG+rG)/2;
           out.data[offset+2]=(lB+rB)/2;
          } else
          {//If not blend right is copied on top
           out.data[offset+0]=rR;
           out.data[offset+1]=rG;
           out.data[offset+2]=rB;
          }
       }
    }
   }
 return 1;
}


int stitchAffineMatch(
                       const char * filenameOutput ,
                       unsigned int border,
                       cv::Mat & left ,
                       cv::Mat & right ,
                       cv::Mat & warp_mat
                     )
{
    fprintf(stderr,"Stitching Affine Transformation Match : ");
    unsigned int borderX = left.size().width/2;
    unsigned int borderY = border;

    warp_mat.at<double>(0,2) += borderX;
    warp_mat.at<double>(1,2) += borderY;

    cv::Size sz = cv::Size(/*left.size().width +*/ right.size().width  + borderX + border , /*left.size().height +*/ right.size().height + borderY + border );
    cv::Mat matchingImageLeft = cv::Mat::zeros(sz, CV_8UC3);
    cv::Mat matchingImageRight = cv::Mat::zeros(sz, CV_8UC3);
    cv::Mat matchingImageBlended = cv::Mat::zeros(sz, CV_8UC3);


    // Draw camera frame
    cv::Mat roi1 = cv::Mat(matchingImageRight, cv::Rect(borderX, borderY, right.size().width, right.size().height));
    right.copyTo(roi1);


   /// Apply the Affine Transform just found to the src image
   cv::Mat warp_dst = cv::Mat::zeros( left.rows, left.cols, left.type() );
   cv::warpAffine( left, matchingImageLeft /* warp_dst */ , warp_mat, /*warp_dst*/ matchingImageLeft.size() );

   //double alpha = 0.5 , beta = ( 1.0 - alpha );
   //addWeighted(   matchingImageLeft /*warp_dst*/  , alpha, matchingImageRight, beta, 0.0, matchingImageBlended);
   myBlendImages(matchingImageBlended , matchingImageLeft , matchingImageRight , 1 );

   cv::imwrite( filenameOutput  ,  matchingImageBlended /* warp_dst */);
   fprintf(stderr,"done. \n");
   return 1;
}




int stitchHomographyMatch(
                          const char * filenameOutput ,
                          unsigned int border,
                          cv::Mat & left ,
                          cv::Mat & right ,
                          cv::Mat & warp_mat
                         )
{
    fprintf(stderr,"Stitching Homography Match : ");
    unsigned int borderX = left.size().width;
    unsigned int borderY = 0;// border;

    cv::Mat translation = cv::Mat::zeros(3,3,CV_64FC1);
    cv::Mat translatedWarpMat = cv::Mat::zeros(3,3,CV_64FC1);


    translation.at<double>(0,2) = borderX;
    translation.at<double>(1,2) = borderY;
    translation.at<double>(0,0) =  1.0;
    translation.at<double>(1,1) =  1.0;
    translation.at<double>(2,2) =  1.0;

    translatedWarpMat = translation * warp_mat;


    cv::Size sz = cv::Size( /*left.size().width +*/ right.size().width  + borderX + border , /*left.size().height +*/ right.size().height + borderY + border );
    cv::Mat matchingImageLeft = cv::Mat::zeros(sz, CV_8UC3);
    cv::Mat matchingImageRight = cv::Mat::zeros(sz, CV_8UC3);
    cv::Mat matchingImageBlended = cv::Mat::zeros(sz, CV_8UC3);


    // Draw camera frame
    cv::Mat roi1 = cv::Mat(matchingImageRight, cv::Rect(borderX, borderY, right.size().width, right.size().height));
    right.copyTo(roi1);


   /// Apply the Affine Transform just found to the src image
   cv::Mat warp_dst = cv::Mat::zeros( left.rows, left.cols, left.type() );
   cv::warpPerspective( left, matchingImageLeft , translatedWarpMat , matchingImageLeft.size() );

   myBlendImages(matchingImageBlended , matchingImageLeft , matchingImageRight , 1);

   cv::imwrite( filenameOutput  ,  matchingImageBlended );
   fprintf(stderr,"done. \n");
   return 1;
}
