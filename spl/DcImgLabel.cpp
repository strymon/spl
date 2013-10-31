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
 \file DcImgLabel.cpp
 --------------------------------------------------------------------------*/
#include "DcImgLabel.h"

DcImgLabel::DcImgLabel(QWidget *parent) :
    QLabel(parent)
{
}

void DcImgLabel::mousePressEvent(QMouseEvent *ev)
{
    Q_UNUSED(ev);

    if(!this->pixmap())
        return;

    _orgW = this->pixmap()->width();
    _orgH = this->pixmap()->height();

    this->setPixmap(this->pixmap()->scaled(_orgW-2,_orgH-2));


}

void DcImgLabel::mouseReleaseEvent(QMouseEvent *ev)
{
    Q_UNUSED(ev);

    if(!this->pixmap())
        return;


    this->setPixmap(this->pixmap()->scaled(_orgW,_orgH));

    emit clicked();
}
