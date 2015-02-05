#!/bin/bash

STARTDIR=`pwd`
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

#BINARIES_THAT_NEED_LIBS="grabber viewer grabber_mux grabber_segment acquisitionBroadcast editor redist openni1_acquisition_shared_library openni2_acquisition_shared_library libfreenect_acquisition_shared_library "


ONIFOLDER64="x64-Release"
ONIFOLDER="x86-Release"
MACHINE_TYPE=`uname -m`
if [ ${MACHINE_TYPE} == 'x86_64' ]; then
echo "Will pick 64bit binaries"
ONIFOLDER=$ONIFOLDER64
else
echo "Will pick 32bit binaries"
#CUDA_VER SHOULD ALREADY BE SET TO $CUDA_VER32
fi





BINARIES_THAT_NEED_LIBS="`../scripts/binariesThatNeedLibs.sh`"

if [ -d libfreenect2 ]
then
echo "Freenect appears to already exist .."
else
  echo "Do you want to download freenect 2 ( kinect 2 support ) ? " 
  echo
  echo -n " (Y/N)?"
  read answer
  if test "$answer" != "N" -a "$answer" != "n";
  then
     git clone git://github.com/OpenKinect/libfreenect2.git
     cd libfreenect2
     sudo depends/install_ubuntu.sh
     cd examples/protonect
     cmake .
     make
     cd ../../../

     #should be at 3dparty dir
     cd 3dparty   
  fi
fi











if [ -d libfreenect ]
then
echo "Freenect appears to already exist .."
else
  echo "Do you want to download freenect ? " 
  echo
  echo -n " (Y/N)?"
  read answer
  if test "$answer" != "N" -a "$answer" != "n";
  then
     sudo apt-get install libxmu-dev libxi-dev freeglut3-dev libusb-dev  libusb-1.0-0-dev  
     git clone git://github.com/OpenKinect/libfreenect.git
     cd libfreenect
     mkdir build
     cd build
     cmake .. -DBUILD_OPENNI2_DRIVER=ON
     make
     cd ../../ 


     #should be at 3dparty dir
     cd ..  
     #should be at root dir
     for f in $BINARIES_THAT_NEED_LIBS
           do  
             if [ -d $f ]
              then
               cd $f 
               ln -s ../3dparty/libfreenect/build/lib/libfreenect.so
               ln -s ../3dparty/libfreenect/build/lib/libfreenect_sync.so 
               cd ..
             else
              echo "Could not create links for $f"
             fi
           done

     #should be at 3dparty dir
     cd 3dparty   
  
  fi
fi











#wget http://www.openni.org/wp-content/uploads/2012/12/OpenNI-Bin-Dev-Linux-Arm-v1.5.4.0.tar.zip
#unzip OpenNI-Bin-Dev-Linux-Arm-v1.5.4.0.tar.zip
#rm OpenNI-Bin-Dev-Linux-Arm-v1.5.4.0.tar.zip
#tar xvjf OpenNI-Bin-Dev-Linux-Arm-v1.5.4.0.tar.bz2 
#--
#ln -s OpenNI-Bin-Dev-Linux-Arm-v1.5.4.0/ OpenNI
#mkdir -p Platform/Linux/Bin/
#cd Platform/Linux/Bin/
#ln -s ../../../Lib/ x64-Release
#cd ../../../../


if [ -d OpenNI ]
then
echo "OpenNI1 appears to already exist .."
else
  echo "Do you want to download OpenNI1 ? " 
  echo
  echo -n " (Y/N)?"
  read answer
  if test "$answer" != "N" -a "$answer" != "n";
  then
     sudo apt-get install libxmu-dev libxi-dev freeglut3-dev libusb-dev  libusb-1.0-0-dev  
     git clone git://github.com/OpenNI/OpenNI.git 
     cd OpenNI/Platform/Linux/CreateRedist
     ./RedistMaker
     
     cd ../Redist
     #sudo ./install.sh 
 
     cd ../../../../


     #should be at 3dparty dir
     cd ..  
     #should be at root dir
     for f in $BINARIES_THAT_NEED_LIBS
           do  
             if [ -d $f ]
              then
               cd $f 
               ln -s ../3dparty/OpenNI/Platform/Linux/Bin/$ONIFOLDER/libOpenNI.so
               ln -s ../3dparty/OpenNI/Platform/Linux/Bin/$ONIFOLDER/libOpenNI.jni.so  
               ln -s ../3dparty/OpenNI/Platform/Linux/Bin/$ONIFOLDER/libnimCodecs.so 
               ln -s ../3dparty/OpenNI/Platform/Linux/Bin/$ONIFOLDER/libnimMockNodes.so 
               ln -s ../3dparty/OpenNI/Platform/Linux/Bin/$ONIFOLDER/libnimRecorder.so 
               cd ..
             else
              echo "Could not create links for $f"
             fi
           done

     #should be at 3dparty dir
     cd 3dparty   

  fi
fi

#wget http://www.openni.org/wp-content/uploads/2013/01/OpenNI-Linux-Arm-2.1.0.tar.zip
#unzip OpenNI-Linux-Arm-2.1.0.tar.zip 
#rm OpenNI-Linux-Arm-2.1.0.tar.zip 
#tar xvjf OpenNI-Linux-Arm-2.1.0.tar.bz2 
#--
#ln -s OpenNI-2.1.0-arm/ OpenNI2
#mkdir Bin 
#cd Bin
#ln -s ../Redist/ x64-Release
#cd ..

if [ -d OpenNI2 ]
then
echo "OpenNI2 appears to already exist .."
else
  echo "Do you want to download OpenNI2 ? " 
  echo
  echo -n " (Y/N)?"
  read answer
  if test "$answer" != "N" -a "$answer" != "n";
  then
     sudo apt-get install libxmu-dev libxi-dev freeglut3-dev libusb-dev  libusb-1.0-0-dev  libudev-dev
     git clone git://github.com/OpenNI/OpenNI2.git
     cd OpenNI2
     make 

     cd Bin/$ONIFOLDER/OpenNI2/Drivers/
     #ln -s ../../../../../libfreenect/build/lib/OpenNI2-FreenectDriver/libFreenectDriver.so   
     ln -s $DIR/libfreenect/build/lib/OpenNI2-FreenectDriver/libFreenectDriver.so
     cd ../../../../

     cd ..
     
     #should be at 3dparty dir
     cd ..  
     #should be at root dir
     for f in $BINARIES_THAT_NEED_LIBS
           do  
             if [ -d $f ]
              then
               cd $f  
               ln -s ../3dparty/OpenNI2/Bin/$ONIFOLDER/OpenNI2/  
               ln -s ../3dparty/OpenNI2/Config/OpenNI.ini   
               ln -s ../3dparty/OpenNI2/Config/PS1080.ini 
               ln -s ../3dparty/OpenNI2/Bin/$ONIFOLDER/libOpenNI2.so 
               cd ..
             else
              echo "Could not create links for $f"
             fi
           done

     #should be at 3dparty dir
     cd 3dparty   

  fi
fi



if [ -d caffe ]
then
echo "Caffe appears to already exist .."
else
  echo "Do you want to download Caffe ? " 
  echo
  echo -n " (Y/N)?"
  read answer
  if test "$answer" != "N" -a "$answer" != "n";
  then 
      sudo apt-get install libprotobuf-dev libleveldb-dev libsnappy-dev libopencv-dev libboost-all-dev libhdf5-serial-dev libgflags-dev libgoogle-glog-dev liblmdb-dev protobuf-compiler 
      git clone https://github.com/BVLC/caffe.git
      cd caffe

      #CMakeFile of caffe is bad :P 
      #mkdir build
      #cd build
      #cmake ..
      
      cp  Makefile.config.example Makefile.config
      make  
  fi
fi













cd "$STARTDIR"

exit 0
