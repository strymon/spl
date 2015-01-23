#include "DcDropLabel.h"
#include <QDropEvent>
#include <QMimeData>
#include <QWidget>
#include <QDebug>

DcDropLabel::DcDropLabel(QWidget *parent) :
    QLabel(parent)
{

}

DcDropLabel::~DcDropLabel()
{

}

void DcDropLabel::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasFormat("application/x-qt-windows-mime;value=\"FileName\""))
       {
           event->acceptProposedAction();
       }
}

void DcDropLabel::dropEvent(QDropEvent *event)
{
    const QMimeData *data = event->mimeData();
      Qt::DropAction action = event->dropAction();

      if ( data->hasFormat("application/x-qt-windows-mime;value=\"FileName\"") &&
          ( (action == Qt::MoveAction) || (action == Qt::CopyAction) ) )
      {

           QUrl url = QUrl::fromEncoded(data->text().toLatin1());
           qDebug() << "FileName: " + url.fileName() << "\n";
           setText(url.toLocalFile());
      }
}

