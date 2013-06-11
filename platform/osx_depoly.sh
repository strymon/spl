# MACHINE DEPENDENT SETTINGS
qtbin=~/Qt5.0.2/5.0.2/clang_64/bin
dest=/Users/john/proj/dc/build/release

# VERSION STRING
ver_str=$(python inc_version.py -a -f ../spl/main.cpp -v kDcVersionString)

# Update the Info.plist version so it matches main.cpp
python inc_version.py -f ./osx/Info.plist -v string -s $ver_str

src=$(pwd)/../spl

rm -Rf $dest
mkdir -p $dest
mkdir -p ../build

pushd .
cd $dest
$qtbin/qmake $src/spl.pro -r -spec macx-clang CONFIG+=x86_64
make
popd

if [ ! -e $dest/spl.app ]
  then
    return
fi

# copy the icon and Info plist into the app bundle
cp ./osx/spl.icns $dest/spl.app/Contents/Resources/
cp ./osx/Info.plist $dest/spl.app/Contents/

# run the Qt provided deploy tool, this setups up the shared libs and frameworks
$qtbin/macdeployqt $dest/spl.app

# At this point, the program should be a standalone mac application.

# Punt - It's easy to make a disk image, however, if you want a background image and some nice icons, some scripting is
# needed.  I use dmgcanvas ($19, 3rd party tool) to create the disk image - I found that it's easier to 'punt' here.
# The last time I tried to create a nice looking disk image (dmg) using scripts and such, it was like playing wac-a-mole.
# I could not find the right 'formula' to get the image looking correct on all version of OSX - in a timely manner.
dmgcanvas osx/disk_image.dmgCanvas ../build/strymon_lib_setup_${ver_str}.dmg -v "Strymon Librarian v${ver_str}"

if [ ! -f ../build/strymon_lib_setup_${ver_str}.dmg ]
  then
    echo Error creating disk image 
fi

if [ -f post_build.sh ]
  then
    . post_build.sh ../build/strymon_lib_setup_${ver_str}.dmg
fi

