#!/bin/bash - 
# Assume this is executed from <root>/platform, so the project root is $pwd/..
EXPECTED_ARGS=1
E_BADARGS=65

if [ $# -ne $EXPECTED_ARGS ]
then
  echo "Usage: `basename $0` {tag}"
  exit $E_BADARGS
fi

ver_str=$1
is_dirty=$(git status -s -u no --porcelain | wc -l)

if [ $is_dirty -gt 1 ]
then
  echo You can only tag if the working directory is clean
else
  echo It is ok to tag 
  python inc_version.py -s $ver_str -f ../spl/main.cpp -v kDcVersionString
  python inc_version.py -f ./osx/Info.plist -v string -s $ver_str
  git commit -a -m"version bump to $ver_str"
  git tag -a $ver_str -m"version $ver_str"
fi


