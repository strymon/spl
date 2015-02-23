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

#include <QDialog>

#include "ui_IoProgressDialog.h"

class IoProgressDialog : public QDialog
{
    Q_OBJECT

public:
    IoProgressDialog(QWidget *parent = 0);

    void setSizeAndPosition(QWidget* p);


public slots:
    void reject();
    void setProgress(int v);
    void inc();
    bool cancled();
    void reset();
    void setMax(int max);
    void setError( QString msg );
    void setMessage( QString msg );
    void setFormat(QString fmt);
    void setNoCancel( bool noCan );
    void useOkButton( bool useOk );
    void setIoHealth( int badnessLvl );
    

protected:
    void showEvent(QShowEvent *e);
    
private:

    Ui_IoProgressDialog ui;
    bool _hasCancled;
    QWidget* _parent;


};


