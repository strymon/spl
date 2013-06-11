# This tool can return or increment version number strings within a text file for a 
# specific name/value format.  The tool is useful for creating automated build scripts
# that can update versing definitions in source and .plist (xml) files.
#
#  Examples of tested formats:
#  const char* myver="1.2.3.4";
#  #define MYVER "1.2.3.4"
#  <string>1.2.3.4</string>
# 
# Usage:
#    inc_version.py -v myver -f myprog.h --inc_maj
#       The above will look for 'myver' "0.0.0.0" and increment the "major" number
#       in the version string.  Version numbers are defined as Major.minor.incremental.buid
#       
import re
import string
import os
import sys

from optparse import OptionParser

# ----------------------------------------------------------------
# Returns a python regular express that will identify the name/version string pair.
# The name is specified in var_name.
# Note: Extending 
def makeRegex(var_name):
    return r'(.*\s*' + var_name + '\s*=?>?\s*\"?\s*)(\d+\.\d+\.\d+\.[a-z0-9]+)(\"?;?<?.*)'
#
# ----------------------------------------------------------------
def set_version(filename,var_name,ver_str):
    if not os.path.exists(filename):
        print "Error: File " + filename + " was not found" 
        return

    f = open(filename,'r')
    fdata = f.read().strip()

    regstr = makeRegex(var_name)
    regex = re.compile(regstr, re.MULTILINE)
    m = re.search(regstr, fdata)
    if not m or len(m.groups()) < 3:
      print "Error: " + var_name + " was not found or the version was not formated correctly"
      return
    new_ver = m.group(1) + ver_str + m.group(3)
    fdata = re.sub(regex, new_ver, fdata)
    f = open(filename,'w')
    f.write(fdata)

# ----------------------------------------------------------------
def get_version(filename,var_name):
    if not os.path.exists(filename):
      print "Error: File " + filename + " was not found" 
      return ""

    f = open(filename,'r')
    fdata = f.read().strip()

    regstr = makeRegex(var_name)
    m = re.search(regstr,fdata)
    if m:
        return m.group(2)
    print "Error: " + var_name + " was not found in " + filename 
    return "" 
# ----------------------------------------------------------------
# Increment the version string in the specified file.
# Use the 'idx' param to specified the version digit to
# increment: Version format is M.m.i.b
# idx 0 = M
#     1 = m
#     2 = i
#     3 = b
def inc_ver(filename,var_name,idx):

    ver = get_version(filename,var_name)
    if ver == "":
      print "inc_ver error: " + var_name + " did not parse"
      return

    vlst = ver.split('.')
    vdigcnt= len(vlst)
    if idx > vdigcnt:
      print "Index of version (" + str(idx) + ") is out of range in: " + ver
      return

    vlst[idx] = str(int(vlst[idx]) + 1)
    idx +=1 
    while idx < vdigcnt:
       vlst[idx] = str(0)
       idx +=1

    ver = '.'.join(vlst)
    set_version(filename,var_name,ver)

# ----------------------------------------------------------------
# MAIN
parser = OptionParser()
parser.add_option('-M', '--inc_maj', action="store_true",help='Increment major version number')
parser.add_option('-m', '--inc_min', action="store_true",help='Increment minor version number')
parser.add_option('-i', '--inc_inc', action="store_true",help='Increment incremental version number')
parser.add_option('-b', '--inc_build', action="store_true",help='Increment build version number')
parser.add_option('-a', '--auto_bld_format', action="store_true",help='format the output for use with other scripts')

parser.add_option('-s', '--version_string', help='Set the version string')

parser.add_option('-v', '--var_name', help='variable name that equates to version number')
parser.add_option('-f', '--filename', help='file name')

options, args = parser.parse_args()
errors = []
error_msg = 'No %s specified. Use option %s'

if not options.var_name:
    errors.append(error_msg % ('variable name', '-v'))
if not options.filename:
    errors.append(error_msg % ('file name', '-f'))

if errors:
    print '\n'.join(errors)
    sys.exit(1)

if options.version_string:
  set_version(options.filename,options.var_name,options.version_string)
else:
  dig_idx = -1

  if options.inc_maj:
    dig_idx = 0

  if options.inc_min:
    dig_idx = 1

  if options.inc_inc:
    dig_idx=2

  if options.inc_build:
    dig_idx=3

  if dig_idx > -1:
    inc_ver(options.filename,options.var_name,dig_idx)
  else:
    ver = get_version(options.filename,options.var_name)
    if ver:
      if options.auto_bld_format:
        print ver
      else:
        print options.filename + ":" + options.var_name + "=" + ver
    sys.exit(1)

sys.exit(0)


