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

#include "ui_MoveDialog.h"

class MoveDialog: public QDialog
{
    Q_OBJECT

public:
    MoveDialog(QWidget *parent = 0);

    void setTargetPreset(QString name);
    void setDestinations(QStringList destNames);

    int getDestIdx();
    int getSrcIdx();
    bool swapClicked() const { return _swap; }

public slots:
    void on_swapButton_clicked();
    void on_replaceButton_clicked();


protected:
    void showEvent(QShowEvent *e);


private:
    int _srcIdx;
    Ui_MoveDialog ui;
    bool _swap;
    
    
};


