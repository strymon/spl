#!/usr/bin/env bash 

# Default location of Qt - this is Machine/Developer dependent
qtdir=/Users/$USER/qt/Qt5.4.0/5.4/clang_64

# The Apple assigned cert name was found in my keychain after 
# following the details in Apple Dev document: 'Code Signing Task'
app_cert_name="Developer ID Application: DAMAGE CONTROL ENGINEERING LLC (K52VK3YS5F)"
installer_cert_name="Developer ID Installer: "
OPT_CODE_SIGN=NO
# Default Script Modes 
OPT_BUILD_APP=NO
OPT_DISK_IMAGE=NO
OPT_PREP_DEPLOY=NO
OPT_DEBUG_INFO_DUMP=NO

optspec=":hbipgcd:"
while getopts "$optspec" optchar; do
    case "${optchar}" in
        h)
            echo "usage: $0 [-d <qt path>] [-i] [-b] [-p] [-c] [-g] [-h]" >&2
            echo "    -d Set Qt location" >&2
            echo " BEHAVIOR OPTIONS"
            echo "    -b Build application" >&2
            echo "    -p Prep application Bundle for deployment" >&2
            echo "       -c The Prep application phase will codesign the app bundel"
            echo "    -i Create disk image (dmg)" >&2
            echo " DEBUG OPTIONS"
            echo "     -g Display debugging info and exit" >&2
            echo 
            echo " -h Help Doc" >&2
            exit 2
            ;;
        d)
            qtdir=${OPTARG}
            ;;
        c)
            OPT_CODE_SIGN=YES
            ;;
        i)
            OPT_DISK_IMAGE=YES
            ;;
        p)
            OPT_PREP_DEPLOY=YES
            ;;
        b)
            OPT_BUILD_APP=YES
            ;;
        g)
          OPT_DEBUG_INFO_DUMP=YES
          ;;
        *)
            if [ "$OPTERR" != 1 ] || [ "${optspec:0:1}" = ":" ]; then
                echo "Invalid argument: '-${OPTARG}'" >&2
            fi
            exit -1
            ;;
    esac
done

# Always put the build output in the <root project path>../build
scriptpath=$(dirname "$(cd "$(dirname "$0")"; pwd)/$(basename "$0")")
build_dir=$(python -c "import os.path; print os.path.abspath('$scriptpath/../../build')")
proj_root=$(python -c "import os.path; print os.path.abspath('$scriptpath/..')")

# Vars to control output
app_dir=$build_dir/spl/release
qtbin=$qtdir/bin


# VERSION STRING
ver_str=$(python inc_version.py -a -f ../spl/main.cpp -v kDcVersionString)

if [ $OPT_DEBUG_INFO_DUMP == "YES" ]; then
  echo OSX DEPOLYMENT DEBUG INFO
  echo -------------------------------
  echo SCRIPT OPTIONS:
  echo "   Make disk image: $OPT_DISK_IMAGE"
  echo "   Build Application: $OPT_BUILD_APP"
  echo "   Deployment Prep: $OPT_PREP_DEPLOY"
  echo
  echo SETUP: 
  echo "   Project Root:$proj_root "
  echo "   Build Path: $build_dir"
  echo "   App Output: $app_dir"
  echo "   QTDIR: $qtdir"
  echo "   QMAKE Invocation: $qtbin/qmake $proj_root/spl.pro -r -spec macx-clang CONFIG+=x86_64"

  if [ $OPT_CODE_SIGN == "YES" ]; then
     echo "   Code Sign Enabled"
     echo "   App Cert: $app_cert_name"
     echo "   Installer Cert: $installer_cert_name"
  fi
  echo 
  echo PROJECT:
  echo "    Extracted Version: $ver_str"
  echo
  echo TESTS:
  if [ -e $proj_root/spl.pro ];then
     echo "    Verify Project Root: SUCCESS"
  else
     echo "    Verify Project Root: ERROR $proj_root/spl.pro not found"
  fi

  if [ -e $qtbin/qmake ];then
     echo "    Verify QT Insall: SUCCESS"
  else
     echo "   Verify QT Insall: ERROR - not found"
  fi
  exit 0
fi

if [ $OPT_BUILD_APP == 'YES' ]; then

  # Careful
  rm -Rf $build_dir
  mkdir -p $build_dir

  pushd .
  cd $build_dir
  $qtbin/qmake $proj_root/spl.pro -r -spec macx-clang CONFIG+=x86_64
  make -j 8
  popd
fi

if [ $OPT_PREP_DEPLOY == 'YES' ]; then 

  if [ ! -e $app_dir/spl.app ]; then
      echo ERROR - could not find $app_dir/spl.app
      exit -1
  fi

  if [ -e $build_dir/spl.app ]; then
    rm -Rf $build_dir/spl.app
  fi

  cp -Rf $app_dir/spl.app $build_dir

  # copy the icon and Info plist into the app bundle
  cp $scriptpath/osx/spl.icns $build_dir/spl.app/Contents/Resources/
  cp $scriptpath/osx/Info.plist $build_dir/spl.app/Contents/

  # run the Qt provided deploy tool, this setups up the shared libs and frameworks
  if [ $OPT_CODE_SIGN == 'YES' ]; then
    $qtbin/macdeployqt $build_dir/spl.app -codesign="$app_cert_name"
  else
    $qtbin/macdeployqt $build_dir/spl.app 
  fi
fi

# At this point, the program should be a standalone mac application.
if [ $OPT_DISK_IMAGE == 'YES' ]; then 

  if [ ! -e $build_dir/spl.app ]
    then
      echo "ERROR - could not find $build_dir/spl.app, was it preped for deply (-p option)"
      exit -1
  fi

# Create a disk image using this 3rd party tool.
dmgcanvas osx/disk_image.dmgCanvas $build_dir/strymon_lib_setup_${ver_str}.dmg -v "Strymon Librarian v${ver_str}"

if [ ! -f $build_dir/strymon_lib_setup_${ver_str}.dmg ]
  then
    echo Error creating disk image 
fi

if [ -f post_build.sh ]
  then
    . post_build.sh $build_dir/strymon_lib_setup_${ver_str}.dmg
fi
fi # OPT_DISK_IMAGE
