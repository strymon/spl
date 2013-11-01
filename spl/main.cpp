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
#include "DcPresetLib.h"
#include <QtWidgets/QApplication>
#include "QRtMidi/QRtMidiData.h"
#include <QDebug>
#include <QtGlobal>
#include "DcConArgs.h"


static const char* kDcVersionString = "0.9.3.8";




void muteMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);
    Q_UNUSED(msg);
}

// QByteArray localMsg = msg.toLocal8Bit();
// switch (type) 
// {
// case QtDebugMsg:
//     // fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
//     break;
// case QtWarningMsg:
//     // fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
//     break;
// case QtCriticalMsg:
//     //fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
//     break;
// case QtFatalMsg:
//     //fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
//     abort();
// }


int main(int argc, char *argv[])
{
    qInstallMessageHandler (muteMessageOutput);

    QApplication a(argc, argv);

    qRegisterMetaType<QRtMidiData>();
    qRegisterMetaType<DcConArgs>();


    a.setOrganizationName("Strymon");
    a.setOrganizationDomain("damagecontrolusa.com");
    a.setApplicationName("spl");
    a.setApplicationDisplayName("Strymon Librarian");
    a.setApplicationVersion(kDcVersionString);



    DcPresetLib w;
    w.show();
    return a.exec();
}
