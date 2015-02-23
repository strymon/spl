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

#include "DcQUtils.h"
#include <QDateTime>
#include <QTime>
#include <QSysInfo>
#include <QStringList>

int DcQUtils::verStrToDec( const QString& str )
{
    int rtval = 0;
    QStringList vlst;
    vlst = str.split( "." );
    if( vlst.length() == 4 )
    {
        rtval =  vlst[0].toInt() << 24;
        rtval += vlst[1].toInt() << 16;
        rtval += vlst[2].toInt() << 8;
        rtval += vlst[3].toInt();
    }
        
    return rtval;
}

//-------------------------------------------------------------------------
QString DcQUtils::getTimeStamp()
{
    return QTime::currentTime().toString("hh:mm:ss:zzz");
}

QString DcQUtils::getOsVersion()
{
#ifdef Q_OS_WIN
    switch (QSysInfo::WindowsVersion)
    {
        case QSysInfo::WV_2000: return "Windows 2000";
        case QSysInfo::WV_XP: return "Windows XP";
        case QSysInfo::WV_2003: return "Windows Server 2003";
        case QSysInfo::WV_VISTA: return "Windows Vista";
        case QSysInfo::WV_WINDOWS7: return "Windows 7";
        case QSysInfo::WV_WINDOWS8: return "Windows 8";
        case QSysInfo::WV_WINDOWS8_1: return "Windows 8.1";

        default: 
            return "Unknown Windows";
    }
#elif defined Q_OS_MAC
    int foo = (int)QSysInfo::MacintoshVersion;

    switch (QSysInfo::MacintoshVersion)
    {
        case QSysInfo::MV_Unknown: return "Unknown Mac";
        case QSysInfo::MV_10_0: return "OS X 10.0";
        case QSysInfo::MV_10_1: return "OS X 10.1";
        case QSysInfo::MV_10_2: return "OS X 10.2";
        case QSysInfo::MV_10_3: return "OS X 10.3";
        case QSysInfo::MV_10_4: return "OS X 10.4";
        case QSysInfo::MV_10_5: return "OS X 10.5";
        case QSysInfo::MV_10_6: return "OS X 10.6";
        case QSysInfo::MV_10_7: return "OS X 10.7";
        case QSysInfo::MV_10_8: return "OS X 10.8";
        case QSysInfo::MV_10_9: return "OS X 10.9";
        case QSysInfo::MV_10_10: return "OS X 10.10";

        default: 
            return "Unknown Mac";
    }
#else
    return "*nix";
#endif
}
