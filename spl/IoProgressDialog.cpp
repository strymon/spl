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
#include "IoProgressDialog.h"
#include <QTimer>



//-------------------------------------------------------------------------
IoProgressDialog::IoProgressDialog( QWidget *parent /*= 0*/ )
    :QDialog(parent)
{
    ui.setupUi(this);
    ui.progressBar->setFormat("%v/%m");

    Qt::WindowFlags flags;

#ifdef Q_OS_OSX
    flags = Qt::Sheet;
#else
    flags = Qt::SplashScreen;
    this->setWindowOpacity(0.8);
#endif

    flags ^= Qt::NoDropShadowWindowHint;
    setWindowFlags(flags);

    setWindowModality(Qt::WindowModal);
    setModal(true);
     this->setWindowIcon(QIcon(":/images/res/dcpm_256x256x32.png"));
}

//-------------------------------------------------------------------------
void IoProgressDialog::setProgress( int v )
{
   ui.progressBar->setValue(v);
}

//-------------------------------------------------------------------------
void IoProgressDialog::reject()
{
    _hasCancled = true;
    QDialog::reject();
}

//-------------------------------------------------------------------------
void IoProgressDialog::inc()
{
    int v = ui.progressBar->value() + 1;
    ui.progressBar->setValue(v);
}

//-------------------------------------------------------------------------
bool IoProgressDialog::cancled()
{
    return _hasCancled;
}

//-------------------------------------------------------------------------
void IoProgressDialog::reset()
{
    setProgress(0);
    
    _hasCancled = false;
    ui.msgLabel->clear();
}
//-------------------------------------------------------------------------
void IoProgressDialog::setError( QString msg )
{
    ui.msgLabel->setText(msg);
}

//-------------------------------------------------------------------------
void IoProgressDialog::setMax( int max )
{
      ui.progressBar->setMinimum(0);
      ui.progressBar->setMaximum(max);
}

//-------------------------------------------------------------------------
void IoProgressDialog::setSizeAndPosition(QWidget* p)
{
    int w = p->size().width()*0.9;
    int xadj = (p->size().width()-w)/2;
    resize(w,148);
    move(p->pos().x()+xadj,p->pos().y() + p->size().height()/2);

}

//-------------------------------------------------------------------------
void IoProgressDialog::showEvent( QShowEvent *e )
{
    Q_UNUSED(e);
    ui.pushButton->setFocus();

}
