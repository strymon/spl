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
#include "RenameDialog.h"
#include <QIcon>

//-------------------------------------------------------------------------
RenameDialog::RenameDialog(QWidget *parent /*= 0*/ )
{
    ui.setupUi(this);
    
    this->setWindowTitle("Preset Rename");
    setWindowFlags( Qt::Dialog | Qt::WindowTitleHint );

    int pwh = (parent->size().width()/2);
    int x = (parent->pos().x() + (pwh - size().width()));
    int y = parent->pos().y() + parent->size().height()/2;
    move(x,y);
     this->setWindowIcon(QIcon(":/images/res/dcpm_256x256x32.png"));
}

//-------------------------------------------------------------------------
void RenameDialog::accept()
{
      QDialog::accept();
}

//-------------------------------------------------------------------------
void RenameDialog::reject()
{
    QDialog::reject();
}

//-------------------------------------------------------------------------
void RenameDialog::setName( QString n )
{
    ui.lineEdit->setText(n);
    ui.lineEdit->selectAll();
}

//-------------------------------------------------------------------------
QString RenameDialog::getName()
{
    QString n = ui.lineEdit->text().toUpper(); 
    return n;

}
