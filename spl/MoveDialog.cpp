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
#include "MoveDialog.h"

//-------------------------------------------------------------------------
MoveDialog::MoveDialog( QWidget *parent /*= 0*/ )
{
    ui.setupUi(this);
    
    move(parent->pos());
    this->setWindowTitle("Preset Move");
    setWindowFlags( Qt::Dialog | Qt::WindowTitleHint );
    this->setWindowIcon(QIcon(":/images/res/dcpm_256x256x32.png"));

}

void MoveDialog::showEvent(QShowEvent *e)
{
    Q_UNUSED(e);
    _srcIdx = 0;
    _swap = false;
}

//-------------------------------------------------------------------------
void MoveDialog::setTargetPreset( QString name )
{
    ui.lineEdit->setText(name);
}

//-------------------------------------------------------------------------
void MoveDialog::setDestinations( QStringList destNames )
{
    ui.destNameComboBox->addItems(destNames);    
}

//-------------------------------------------------------------------------
int MoveDialog::getDestIdx()
{
    return ui.destNameComboBox->currentIndex();
}

//-------------------------------------------------------------------------
int MoveDialog::getSrcIdx()
{
    return _srcIdx;
}

//-------------------------------------------------------------------------
void MoveDialog::on_swapButton_clicked()
{
    _swap=true;
}

//-------------------------------------------------------------------------
void MoveDialog::on_replaceButton_clicked()
{
    _swap = false;
}
