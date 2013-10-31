# MACHINE DEPENDENT SETTINGS
qtver=5.1.1
qtdir=/Users/$USER/Qt/$qtver/$qtver/clang_64
qtbin=$qtdir/bin
build_dir=/Users/$USER/src/build

BUILD_APP=YES
MAKE_DISK_IMG=YES

app_dir=$build_dir/spl/release

# VERSION STRING
ver_str=$(python inc_version.py -a -f ../spl/main.cpp -v kDcVersionString)

if [ $BUILD_APP == 'YES' ]; then
  # Update the Info.plist version so it matches main.cpp
  python inc_version.py -f ./osx/Info.plist -v string -s $ver_str

  src=$(pwd)/..

  # Careful
  rm -Rf $build_dir
  mkdir -p $build_dir

  pushd .
  cd $build_dir
  $qtbin/qmake $src/spl.pro -r -spec macx-clang CONFIG+=x86_64
  make
  popd
fi

if [ ! -e $app_dir/spl.app ]
  then
    echo ERROR - could not find $app_dir/spl.app
    exit -1
fi

# copy the icon and Info plist into the app bundle
cp ./osx/spl.icns $app_dir/spl.app/Contents/Resources/
cp ./osx/Info.plist $app_dir/spl.app/Contents/

# FIX for 5.1.0 build
if [ $qtver == '5.1.0' ]; then
  # fix the double slash problem - see stackoverflow http://stackoverflow.com/questions/17475788/qt-5-1-and-mac-bug-making-macdeployqt-not-working-properly
 ./osx/fix_dbl_slash.sh $qtdir $app_dir/spl.app
fi

# run the Qt provided deploy tool, this setups up the shared libs and frameworks
$qtbin/macdeployqt $app_dir/spl.app

# At this point, the program should be a standalone mac application.
if [ $MAKE_DISK_IMG == 'YES' ]; then 

# Punt - It's easy to make a disk image, however, if you want a background image and some nice icons, some scripting is
# needed.  I use dmgcanvas ($19, 3rd party tool) to create the disk image - I found that it's easier to 'punt' here.
# The last time I tried to create a nice looking disk image (dmg) using scripts and such, it was like playing wac-a-mole.
# I could not find the right 'formula' to get the image looking correct on all version of OSX - in a timely manner.
dmgcanvas osx/disk_image.dmgCanvas $build_dir/strymon_lib_setup_${ver_str}.dmg -v "Strymon Librarian v${ver_str}"

if [ ! -f $build_dir/strymon_lib_setup_${ver_str}.dmg ]
  then
    echo Error creating disk image 
fi

if [ -f post_build.sh ]
  then
    . post_build.sh $build_dir/strymon_lib_setup_${ver_str}.dmg
fi
fi # NO_DISK_IMG
