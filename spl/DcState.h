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
#ifndef DCSTATE_H
#define DCSTATE_H

#include <QState>


class DcState : public QState
{
    Q_OBJECT
public:
    static void SetTraceStringList(QStringList* sl);    
    static void trace(QString str);

    explicit DcState( const QString& name, QState* parent = 0 );
    explicit DcState( const QString& name, const QString& prefix, QState* parent = 0 );
    explicit DcState( const QString& name, ChildMode childMode, QState* parent = 0);
    explicit DcState( const QString& name, const QString& prefix, QState::ChildMode childMode, QState* parent = 0 );
    
    QString name() const { return _name; }
    QString prefix() const { return _prefix; }
    
    

    public slots:
        void setName( const QString& name ) { _name = name; }
        void setPrefix( const QString& prefix ) { _prefix = prefix; }

protected:
    virtual void onEntry( QEvent* e );
    virtual void onExit( QEvent* e );

protected:
    QString _name;
    QString _prefix;
};

#endif
