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

*-------------------------------------------------------------------------*/
#pragma once

    struct DcConItem
    {
        DcConItem() : reciver(0),methodArgCnt(0) {}
        DcConItem(const QObject* o, const QByteArray& ba, const QString& h,int argcnt) : reciver(o), methodName(ba), helpString(h),methodArgCnt(argcnt) {}
        const QObject* reciver;
        QByteArray methodName;
        QString helpString;
        int methodArgCnt;
    };

    inline bool operator==(const DcConItem &e1, const DcConItem &e2)
    {
        return e1.methodName == e2.methodName
            && e1.reciver == e2.reciver
            && e1.helpString == e2.helpString 
            && e1.methodArgCnt == e2.methodArgCnt;
    }

//    inline uint qHash(const DcConItem &key, uint seed)
//    {
//        return (uint)(qHash(key.helpString, seed) + qHash(key.methodName, seed)) + (uint)key.reciver;
//    }

