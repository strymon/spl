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
// Code example yanked from the QT project documentation, Thanks!!
// http://qt-project.org/doc/qt-4.8/statemachine-api.html

#include <QState>
#include <QAbstractTransition>
#include <QEvent>

#include "lib/QRtMidi/QRtMidiData.h"

template<int offset>
class DcSimpleEvent : public QEvent
{
public:
    DcSimpleEvent()
        : QEvent( QEvent::Type( QEvent::User + offset ) )
    {}
    const static QEvent::Type TYPE = QEvent::Type(QEvent::User + offset);
};

class DcCustomTransition : public QAbstractTransition
{
public:
    // The simple single event type trigger case...
    explicit DcCustomTransition( QEvent::Type eventType = QEvent::None, QState* sourceState = 0 );

    // The generalized multi event types trigger case...
    explicit DcCustomTransition( const QList<QEvent::Type>& eventTypes, QState* sourceState = 0 );

    virtual bool eventTest( QEvent* e );
    virtual void onTransition( QEvent* e );

    // The simple single event type trigger case...
    void setEventType( const QEvent::Type& eventType ) 
    { 
        m_eventTypes[ 0 ] = eventType; 
    }
    
    QEvent::Type eventType() const 
    { 
        return m_eventTypes.at( 0 ); 
    }

    // The generalized multi event types trigger case...
    void setEventTypes( const QList<QEvent::Type> eventTypes ) 
    { 
        m_eventTypes = eventTypes; 
    }
    
    QList<QEvent::Type> eventTypes() const 
    { 
        return m_eventTypes; 
    }

private:
    QList<QEvent::Type> m_eventTypes;
};

