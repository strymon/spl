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
    :QDialog(parent),_parent(parent)
{
    ui.setupUi(this);

    setSizeAndPosition( _parent);
    Qt::WindowFlags flags;
    
#ifdef Q_OS_OSX
    flags = Qt::Sheet;
#else
    // flags = Qt::SplashScreen;
    flags = Qt::SplashScreen;
    ui.progressBar->setFormat("%v/%m");
    ui.progressBar->setTextVisible(true);
#endif
    this->setWindowOpacity(0.8);
    flags ^= Qt::NoDropShadowWindowHint;
    setWindowFlags(flags);

    setWindowModality(Qt::WindowModal);
    setModal(true);
     this->setWindowIcon(QIcon(":/images/res/dcpm_256x256x32.png"));

     setIoHealth( 0 );
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
// void IoProgressDialog::inc()
// {
//     int v = ui.progressBar->value() + 1;
//     ui.progressBar->setValue(v);
// }

void IoProgressDialog::inc()
{
    int p = ui.progressBar->value();
    ui.progressBar->setValue( p + 1 );
    if( p > ui.progressBar->maximum() )
    {
        ui.progressBar->setMaximum( p + 1 );
    }
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
    ui.progressBar->setTextVisible(false);
    ui.progressBar->reset();
    adjustPosition(_parent);
    ui.pushButton->setText( "Cancel" );
    ui.label->setText("");

    _hasCancled = false;
    ui.msgLabel->clear();
    ui.pushButton->show();
    setIoHealth( 0 );
}
//-------------------------------------------------------------------------
void IoProgressDialog::setError( QString msg )
{
    ui.msgLabel->setStyleSheet("color: rgb(255, 0, 0)");
    ui.msgLabel->setText(msg);
}

//-------------------------------------------------------------------------
void IoProgressDialog::setMax( int max )
{
      ui.progressBar->setMinimum(0);
      ui.progressBar->setMaximum(max);
      ui.progressBar->setTextVisible(true);
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

void IoProgressDialog::setLable( const QString & str )
{
    ui.label->setText( str);
}

//-------------------------------------------------------------------------
void IoProgressDialog::setMessage( QString msg )
{
    ui.msgLabel->setStyleSheet("color: rgb(0,0,0)");
    ui.msgLabel->setText(msg);
}

//-------------------------------------------------------------------------
void IoProgressDialog::setFormat( QString fmt )
{
    ui.progressBar->setFormat(fmt);
}

//-------------------------------------------------------------------------
void IoProgressDialog::setNoCancel( bool noCan )
{
    noCan ? ui.pushButton->hide() : ui.pushButton->show();
}

void IoProgressDialog::useOkButton( bool useOk)
{
    useOk ? ui.pushButton->setText( "Ok" ) : ui.pushButton->setText( "Cancel" );
}


void IoProgressDialog::setIoHealth( int badnessLvl )
{
    if(badnessLvl == 0)
    {
        ui.ioStatusLable->setText( "                " );
        ui.ioStatusLable->setStyleSheet( "border-radius: 4px;" );
    }
    else if( badnessLvl == 1 )
    {
        ui.ioStatusLable->setText( "  rate reduced  " );
        ui.ioStatusLable->setStyleSheet( "background-color: rgba(255, 165, 0, 230);border-radius: 4px;" );
    }
    else if( badnessLvl == 2 )
    {
        ui.ioStatusLable->setText( "  workaround mode  " );
        ui.ioStatusLable->setStyleSheet( "background-color: rgba(255, 0, 0, 230);border-radius: 44px;" );
    }
}
