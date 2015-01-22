# DcMidi

#### Description
This is a Qt based wrapper for rtmidi.

#### Creating MSVC Projects
##### Assumptions
* The project source is located in .\DcMidi
* The desired build directory is .\build
* You have launch a command prompt using ...\msvc2010\bin\qtenv2.bat
* The current working directory is .\build

##### Creating the library project
qmake -spec win32-msvc2010 -tp vc ..\DcMidi

##### Creating the test project
qmake -spec win32-msvc2010 -r -tp vc ..\DcMidi\tests

#### About Rt MIDI
[The RtMidi Tutorial] (http://www.music.mcgill.ca/~gary/rtmidi)
[Documentation] (http://www.music.mcgill.ca/~gary/rtmidi/classRtMidi.html)
[Version 2.0.1](http://www.music.mcgill.ca/~gary/rtmidi/release/rtmidi-2.0.1.tar.gz)
> RtMidi: realtime MIDI i/o C++ classes
> Copyright (c) 2003-2012 Gary P. Scavone

### DcMidi License
Copyright © 2013 Damage Control Engineering, LLC
Licensed under the [Apache License, Version 2.0](https://www.apache.org/licenses/LICENSE-2.0.html)


### Mailing List

For any questions, comments or patches, use the strymon-dev google group here:

https://groups.google.com/group/strymon-dev

