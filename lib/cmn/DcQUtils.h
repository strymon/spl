#pragma once
/*-------------------------------------------------------------------------
	    Copyright 2013 Damage Control Engineering, LLC

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*------------------------------------------------------------------------*/
#include <QString>

class DcQUtils
{
public:
	DcQUtils();
	~DcQUtils();
    /** Given a version string in format MM.mm.inc.bld e.g. 0.2.3.4 -> 0x00020304
     *  @param str
     *  @return int
     */
     static int verStrToDec( const QString& str );
    static QString getTimeStamp();
    static QString getOsVersion();
protected:
	
private:
};


