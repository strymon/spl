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

#include "ui_RenameDialog.h"

class RenameDialog : public QDialog
{
    Q_OBJECT

public:
    RenameDialog(QWidget *parent = 0);
    
    QString getName();


public slots:
    void accept();
    void reject();
    void setName(QString n);
    
protected:


private:

    Ui_RenameDialog ui;
};
