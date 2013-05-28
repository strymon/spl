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
#include "DcListWidget.h"

//-------------------------------------------------------------------------
void DcListWidget::dragEnterEvent( QDragEnterEvent *e )
{
 /*   if (QListWidget *lb = qobject_cast<QListWidget*>(e->source())) 
    {
        

    }*/

    qDebug() << e->source()->objectName();
    qDebug() << "Enter: " << e->pos() << this->itemAt(e->pos())->text();
    e->accept();
}

//-------------------------------------------------------------------------
void DcListWidget::dragLeaveEvent( QDragLeaveEvent *event )
{
    
        
        event->accept();

}


//-------------------------------------------------------------------------
void DcListWidget::dropEvent( QDropEvent *e )
{
    qDebug() << this->selectedItems()[0]->text() << "Replaces " << this->itemAt(e->pos())->text();
    e->accept();
}


//-------------------------------------------------------------------------
void DcListWidget::dragMoveEvent( QDragMoveEvent *e )
{
    qDebug() << "dragMoveEvent";

     QListWidgetItem* i = this->itemAt(e->pos());
     if(i)
     {
         qDebug() << i->text();
     }
     e->accept();
    
    //         if (e->source() != this) 
    //         {
    //             e->accept();
    //         } else 
    //         {
    //             e->ignore();
    //         }
}
