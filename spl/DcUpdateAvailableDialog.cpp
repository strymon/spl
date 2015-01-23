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
 \file DcUpdateAvailableDialog.cpp
 \brief The dialog class supporting software update
--------------------------------------------------------------------------*/
#include "DcUpdateAvailableDialog.h"
#include "ui_DcUpdateAvailableDialog.h"

DcUpdateAvailableDialog::DcUpdateAvailableDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DcUpdateAvailableDialog)
{
    ui->setupUi(this);
    ui->installedVerFrame->hide();
    ui->latestVerFrame->hide();
    ui->installButton->hide();
    ui->reinitPresetCheckBox->hide();


    ui->releaseNotesLable->hide();
    ui->reinitPresetCheckBox->setChecked(false);

}

DcUpdateAvailableDialog::~DcUpdateAvailableDialog()
{
    delete ui;
}

void DcUpdateAvailableDialog::setLatest(QString str,QString iconpath,bool hasPresets /*=false*/)
{
    ui->latestVerFrame->show();
    ui->latestVerLable->setText("<font size= \"3\" color=\"#35DB35\"> <b>" + str + "</b></font>");
    ui->iconLable->setPixmap(QPixmap(iconpath));
    ui->installButton->show();

    if(hasPresets)
        ui->reinitPresetCheckBox->show();

    renderCurrentVer();
}

void DcUpdateAvailableDialog::renderCurrentVer()
{
    if(_curVer.isEmpty())
        return;

    ui->installedVerFrame->show();

    QString str;

    if(ui->latestVerFrame->isHidden())
    {
        str = "<font size= \"3\" color=\"#35DB35\"> <b>" + _curVer + "</b></font>";
    }
    else
    {
        str = "<font size= \"3\" color=\"#FDB71B\"> <b>" + _curVer + "</b></font>";
        ui->headerLabel->setText("Firmware Update Available");
        ui->releaseNotesLable->setOpenExternalLinks(true);
        ui->releaseNotesLable->show();
    }

    ui->installedVerLable->setText(str);
}

void DcUpdateAvailableDialog::setCurrent(QString str)
{
    _curVer = str;
    renderCurrentVer();
}

void DcUpdateAvailableDialog::on_installButton_clicked()
{
    accept();
}

void DcUpdateAvailableDialog::setChangeLogUrl( QString urlstring )
{
    QString html  = "<html><head/><body><p><a href=\"" + urlstring + "\"><span style=\" text-decoration: underline; color:#0000ff;\">read release notes</span></a></p></body></html>";
    ui->releaseNotesLable->setText(html);
}

bool DcUpdateAvailableDialog::okToInstallPresets()
{
    return ui->reinitPresetCheckBox->checkState() == Qt::Checked;
}
