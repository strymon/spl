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
#include "DcStateMachineHelpers.h"

// Code example yanked from the QT project documentation, Thanks!!
// http://qt-project.org/doc/qt-4.8/statemachine-api.html


DcCustomTransition::DcCustomTransition( QEvent::Type eventType, QState* sourceState )
    : QAbstractTransition( sourceState ),
    m_eventTypes()
{
    m_eventTypes.append( eventType );
}

DcCustomTransition::DcCustomTransition( const QList<QEvent::Type>& eventTypes, QState* sourceState )
    : QAbstractTransition( sourceState ),
    m_eventTypes( eventTypes )
{
}

bool DcCustomTransition::eventTest( QEvent* e )
{
    QList<QEvent::Type>::const_iterator itr;
    for ( itr = m_eventTypes.constBegin(); itr != m_eventTypes.constEnd(); ++itr ) 
    {
        if ( e->type() == *itr ) 
        {
            return true;
        }
    }

    return false;
}

void DcCustomTransition::onTransition( QEvent* e )
{
    Q_UNUSED( e );
}