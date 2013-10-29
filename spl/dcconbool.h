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
/*!
 \file dcconbool.h
 \brief A support class for console commands that set/get boolean values.
--------------------------------------------------------------------------*/
#ifndef DCCONBOOL_H
#define DCCONBOOL_H

#include <QObject>

class DcConBool : public QObject
{
    Q_OBJECT
public:
    explicit DcConBool(QObject *parent = 0);
    
    bool isEnabled() { return _enabled; }
signals:
    
public slots:
    void enabled(bool e);
    void toggle();

private:
    bool _enabled;


    
};

#endif // DCCONBOOL_H
