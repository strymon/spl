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
#include "DcState.h"
#include <QDebug>
#include <QStringList>

QStringList* gStateTrace = 0;

#define VERBOSE_STATE_TRACE 0

//-------------------------------------------------------------------------
void DcState::SetTraceStringList( QStringList* sl )
{
    gStateTrace = sl;
}
    
//-------------------------------------------------------------------------
void DcState::trace( QString str )
{
    if(gStateTrace)
    {
        gStateTrace->append(str);
    }
}


DcState::DcState( const QString& name, QState* parent )
    : QState( parent ),
    _name( name ),
    _prefix()
{
}

DcState::DcState( const QString& name, const QString& prefix, QState* parent )
    : QState( parent ),
    _name( name ),
    _prefix( prefix )
{
}


DcState::DcState( const QString& name, ChildMode childMode, QState *parent )
    : QState(childMode,parent),
    _name( name ),
    _prefix()
{
}

DcState::DcState( const QString& name, const QString& prefix, ChildMode childMode, QState *parent )
    : QState(childMode,parent),
    _name( name ),
    _prefix( prefix )
{
}

void DcState::onEntry( QEvent* e )
{
    Q_UNUSED( e );

    // Print out the state we are entering and it's parents
    QString state = _name;
    DcState* parent = dynamic_cast<DcState*>( parentState() );
    while ( parent != 0 )
    {
        state = parent->name() + "->" + state;
        parent = dynamic_cast<DcState*>( parent->parentState() );
    }

    DcState::trace(_prefix + " Enter: " + state);

#if (VERBOSE_STATE_TRACE == 1)
    qDebug() << _prefix << "Entering state:" << state;
#endif // VERBOSE_STATE_TRACE
}

void DcState::onExit( QEvent* e )
{
    Q_UNUSED( e );

    // Print out the state we are exiting and it's parents
    QString state = _name;
    DcState* parent = dynamic_cast<DcState*>( parentState() );
    while ( parent != 0 )
    {
        state = parent->name() + "->" + state;
        parent = dynamic_cast<DcState*>( parent->parentState() );
    }
    DcState::trace(_prefix + " Leave: " + state);

#if (VERBOSE_STATE_TRACE == 1)
        qDebug() << _prefix << "Leaving state:" << state;
#endif
}


