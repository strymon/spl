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
#include <QDropEvent>
#include <QMimeData>
#include <QWidget>
#include <QDebug>

DcImgLabel::DcImgLabel(QWidget *parent) :
QLabel(parent),enterCnt(0)
{

    setMouseTracking( true );
}

void DcImgLabel::setNormalImgName( const QString& resPath )
{
    _normalImagename = resPath;
    if( !underMouse())
    {
        setPixmap( QPixmap( _normalImagename ) );
    }
}

void DcImgLabel::setHoverImgName( const QString& resPath )
{
    _hoverImageName = resPath;
}

void DcImgLabel::mousePressEvent( QMouseEvent *ev )
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


void DcImgLabel::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        _orgW = this->pixmap()->width();
        _orgH = this->pixmap()->height();
        this->setPixmap(this->pixmap()->scaled(_orgW-2,_orgH-2));
        event->acceptProposedAction();
    }
    else if(event->mimeData()->hasFormat("application/x-qt-windows-mime;value=\"FileName\""))
    {
        event->acceptProposedAction();
    }
}

void DcImgLabel::dropEvent(QDropEvent *event)
{
    const QMimeData *data = event->mimeData();
    Qt::DropAction action = event->dropAction();

    if (event->mimeData()->hasUrls())
    {
        setPixmap(this->pixmap()->scaled(_orgW,_orgH));
        QUrl url = QUrl::fromEncoded(data->text().toLatin1());
        event->acceptProposedAction();
        emit fileDropped(url.toLocalFile());
    }
    else  if ( data->hasFormat("application/x-qt-windows-mime;value=\"FileName\"") &&
          ( (action == Qt::MoveAction) || (action == Qt::CopyAction) ) )
      {
        setPixmap(this->pixmap()->scaled(_orgW,_orgH));
        QUrl url = QUrl::fromEncoded(data->text().toLatin1());
        
        event->acceptProposedAction();
        emit fileDropped(url.toLocalFile());
      }
}

void DcImgLabel::dragLeaveEvent(QDragLeaveEvent *event)
{
     setPixmap(this->pixmap()->scaled(_orgW,_orgH));
     event->accept();
}

void DcImgLabel::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void DcImgLabel::mouseMoveEvent( QMouseEvent * )

{
    emit mouseMoved();
}

void DcImgLabel::enterEvent( QEvent * )
{
    setPixmap( QPixmap( _hoverImageName ) );
    qDebug() << "imgLable ENTER";
    emit on_enter();
}

void DcImgLabel::leaveEvent( QEvent * )
{
    setPixmap( QPixmap( _normalImagename ) );
    qDebug() << "imgLable leave";
    emit on_leave();
}
