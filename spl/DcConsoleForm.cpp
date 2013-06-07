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
#include "dcconsoleform.h"
#include "ui_dcconsoleform.h"
#include <QScrollBar>
#include <QObject>
#include <QEvent>
#include <QKeyEvent>
#include <QStringBuilder>
#include <QDebug>
#include <QDir>
#include <QSettings>
 #include <QDateTime>

struct parseError
{
    QString msg;
    parseError(const QString& str) : msg(str) {}
};

//-------------------------------------------------------------------------
QDataStream & operator >> ( QDataStream& stream, DcFnSymDef& symDef )
{
    stream >> symDef.name;
    stream >> symDef.args;
    stream >> symDef.doc;
    stream >> symDef.expr;
    return stream;
}

//-------------------------------------------------------------------------
QDataStream & operator<<( QDataStream& stream, const DcFnSymDef& symDef )
{
    stream << symDef.name;
    stream << symDef.args;
    stream << symDef.doc;
    stream << symDef.expr;
    return stream;
}

//-------------------------------------------------------------------------
DcConsoleForm::DcConsoleForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DcConsoleForm),
    stream(new ConStream())
{
    ui->setupUi(this);

    _updateTimer.setInterval(0);
    _clearOutput = false;
    QObject::connect(&_updateTimer,SIGNAL(timeout()),this,SLOT(refreshTextOutput()));

     _depth = 300;
     
    
     ui->lineEdit->installEventFilter(this);
     ui->textEdit->installEventFilter(this);

     _noClrOnReturnOnce = false;
     _appendOk = false;
     clear();
     
     loadHistory();

     addCmd("echo",this,SLOT(cmd_echo(DcConArgs)),"echo str & more", "Display arguments");
     addCmd("ver",this,SLOT(cmd_ver(DcConArgs)),"ver","Print program version");
     addCmd("aboutQt",this,SLOT(cmd_aboutQt(DcConArgs)),"aboutQt","Display the Qt library 'about' dialog");
     addCmd("con.html",this,SLOT(cmd_conHtml(DcConArgs)),"con.html on?","If arg evaluates true, enable html in the console display, otherwise disable html");
     addCmd("con.depth",this,SLOT(cmd_depth(DcConArgs)),"con.depth & d","Gets or sets display buffer depth in units of lines");
     addCmd("help",this,SLOT(cmd_help(DcConArgs)),"help","Display command help text");
     addCmd("defsv",this,SLOT(cmd_defSave(DcConArgs)),"defsv fname","Saves the command def's to the file fname");
     addCmd("defld",this,SLOT(cmd_defLoad(DcConArgs)),"defld fname","Load the command def's from the file fname");
     addCmd("lsdef",this,SLOT(cmd_lsDef(DcConArgs)),"lsdef","Displays the currently defined commands");
     addCmd("def",this,SLOT(cmd_def(DcConArgs)),"def symbol expr","Creates and binds the symbol to expr");
     addCmd("undef",this,SLOT(cmd_undef(DcConArgs)),"undef symbol","undefines given symbol");
     addCmd("append",this,SLOT(cmd_append(DcConArgs)),"append val","Appends val to the end of the current command-line");
     addCmd("history",this,SLOT(cmd_history(DcConArgs)),"history","display command history");
     addCmd("cls",this,SLOT(cmd_clear(DcConArgs)),"cls","Clears the display");
     addCmd("exit",this,SLOT(cmd_exit(DcConArgs)),"exit","Quit application");
     addCmd("doc",this,SLOT(cmd_doc(DcConArgs)),"doc symbol","Display the doc string for the given symbol");
     addCmd("ts",this,SLOT(cmd_ts(DcConArgs)),"ts","Displays time stamp");

}

//-------------------------------------------------------------------------
DcConsoleForm::~DcConsoleForm()
{
    saveHistory();
    delete ui;
}

//-------------------------------------------------------------------------
void DcConsoleForm::refreshTextOutput()
{
    _updateTimer.stop();

    if(_clearOutput)
    {
        stream->buffer.clear();
        _textOutputLines.clear();
        _clearOutput = false;
        
        for (int line = 0; line < _depth ; line++)
        {
        	QString s;
            s.sprintf("\n");
            _textOutputLines.append(s);
        }
    }
    else
    {
    }
    
    if(this->isVisible())
    {
        ui->textEdit->clear();
        
        QString o = _textOutputLines.join("");
        
        if(_con_html)
        {
            ui->textEdit->setHtml(o.mid(0,o.length()));
        }
        else
        {
            ui->textEdit->setPlainText(o.mid(0,o.length()-1));
        }
        
        QScrollBar *sb = ui->textEdit->verticalScrollBar();
        sb->setValue(sb->maximum() - 1);
    }
}

//-------------------------------------------------------------------------
void DcConsoleForm::loadHistory()
{
    QSettings settings;
    _con_html = settings.value("console/html",false).toBool();
    _depth = settings.value("console/depth",14).toInt();

    int size = settings.beginReadArray("console/history");
    for (int i = 0; i < size; ++i) 
    {
        settings.setArrayIndex(i);
        _history.append(settings.value("cmd").toString());
    }
    settings.endArray();
    _historyIndex = _history.count();
}

//-------------------------------------------------------------------------
void DcConsoleForm::saveHistory()
{
    QSettings settings;

    settings.setValue("console/html",_con_html);
    settings.setValue("console/depth",_depth);

    // First, only save the newest max_history commands
    int hist_size = _history.size();
    int max_history = settings.value("console/max_history", 20).toInt();
    int hist_offset = 0;
    if(hist_size > max_history)
    {
        hist_offset = hist_size - max_history;
        hist_size = max_history;
    }
    // Write the history to the settings file
    settings.beginWriteArray("console/history");
    for (int i = 0; i < hist_size; ++i)
    {
        settings.setArrayIndex(i);
        settings.setValue("cmd",_history.at(i+hist_offset));
    }
    settings.endArray();
}

//-------------------------------------------------------------------------
void DcConsoleForm::on_lineEdit_returnPressed()
{
    QString str = ui->lineEdit->text();

    executeCmdStr(str);
    
    // Update the command line history
    _history.append(str);
    _historyIndex = _history.size();
    
    if(!_noClrOnReturnOnce)
    {
        ui->lineEdit->clear();
        _noClrOnReturnOnce = false;
    }

}

//-------------------------------------------------------------------------
bool DcConsoleForm::eventFilter(QObject* obj, QEvent *e)
{
    if(obj == ui->lineEdit)
    {
        if (e->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
            if((keyEvent->key() == Qt::Key_Control) )
            {
                _appendOk = true;
                _curLineEditText = ui->lineEdit->text();
            }
            else if (keyEvent->key() == Qt::Key_Up)
            {
                handleUpKeyPress();
                e->accept();
                return true;
            }
            else if(keyEvent->key() == Qt::Key_Down)
            {
                handleDownKeyPress();
                e->accept();
                return true;

            }
//             else if (keyEvent->key() == Qt::Key_QuoteLeft)
//             {
//                 if(!isVisible())
//                 {
//                     setVisible(true);
//                     //showDbgOutput();
//                     ui->lineEdit->setFocus();
//                 }
//                 else
//                 {
//                     setVisible(false);
//                 }
//                 e->accept();
//                 return true;
//             }
        }
        else if (e->type() == QEvent::KeyRelease)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
            if((keyEvent->key() == Qt::Key_Control) )
            {
                _appendOk = false;
            }
        }
    }
    else if(obj == ui->textEdit)
    {
   
        
        if (e->type() == QEvent::KeyRelease) //e->type() == QEvent::KeyPress)
        {
                QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
                if(keyEvent->key() == Qt::Key_Down)
                {
                    ui->lineEdit->setFocus();
                    e->accept();
                    return true;
                    
                }        
            
//             QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
//             ui->lineEdit->setText(ui->lineEdit->text() + keyEvent->text());

        }
 
     }

    return false;
}
//-------------------------------------------------------------------------
void DcConsoleForm::setVisible( bool visible )
{
    QWidget::setVisible(visible);

    if(visible)
    {
        this->requestRefresh();
        ui->lineEdit->setFocus();
    }
}

//-------------------------------------------------------------------------
void DcConsoleForm::handleUpKeyPress()
{
    if (_history.count())
    {
        QString command = getCurrentCommand();
        do
        {
            if (_historyIndex)
            {
                _historyIndex--;
            }
            else
            {
                break;
            }
        } while(_history[_historyIndex] == command);

        replaceCurrentCommand(_history[_historyIndex]);
    }
}

//-------------------------------------------------------------------------
void DcConsoleForm::handleDownKeyPress()
{
    if (_history.count())
    {
        QString command = getCurrentCommand();
        do
        {
            if (++_historyIndex >= _history.size())
            {
                _historyIndex = _history.size() - 1;
                break;
            }
        } while(_history[_historyIndex] == command);
        replaceCurrentCommand(_history[_historyIndex]);
    }
}

//-------------------------------------------------------------------------
void DcConsoleForm::replaceCurrentCommand( QString cmd )
{
    ui->lineEdit->setText(cmd);
}

//-------------------------------------------------------------------------
QString DcConsoleForm::getCurrentCommand()
{
    return ui->lineEdit->text();
}

//-------------------------------------------------------------------------
void DcConsoleForm::on_lineEdit_textEdited(const QString & text)
{
     if(text.contains("`"))
     {
        QString str = text;   
        str.replace(QString("`"),QString(""));
         ui->lineEdit->setText(str);

        toggleVisible();
     }
}

//-------------------------------------------------------------------------
void DcConsoleForm::toggleVisible()
{
    
    if(isVisible())
    {
        if(ui->lineEdit->hasFocus())
        {
            setVisible(false);
        }
        else
        {
           ui->lineEdit->setFocus();
        }
    }
    else
    {
        setVisible(true);
    }

}

//-------------------------------------------------------------------------
void DcConsoleForm::flush()
{
    refreshTextOutput();
    QApplication::processEvents();
}

//-------------------------------------------------------------------------
bool DcConsoleForm::addCmd(QString name, const QObject *receiver, const char *member, QString helpString)
{
    bool rtval = false;
    if (receiver && member) 
    {
        if (!name.isEmpty()) 
        {
            const char* bracketPosition = strchr(member, '(');
            if (!bracketPosition || !(member[0] >= '0' && member[0] <= '2')) 
            {
                qWarning("DcConsoleForm::addCmd: Invalid member specification");
            }
            else
            {
                // Get the receiver member argument count
                QByteArray meth(bracketPosition);

                // Clean up the method params
                meth = meth.trimmed();
                meth.replace(" ","");
                
                // Check the argument count
                int argcnt = meth.split(',').count() -1;
                if(argcnt > 1)
                {
                    qWarning("DcConsoleForm::addCmd: Invalid member specification - too many arguments");
                    argcnt = -1; // don't support args over 1
                }
                else if(meth.contains("()"))
                {
                    argcnt = 0;
                }        
                else
                {
                     argcnt = 1;
                }
                if(argcnt != -1)
                {
                    QByteArray methodName(member+1, bracketPosition - 1 - member); // extract method name
                    _cmdHash.insertMulti(name,DcConItem(receiver,methodName,helpString,argcnt));
                    rtval = true;
                }
            }
        }
    }
    return rtval;
}

//-------------------------------------------------------------------------
bool DcConsoleForm::addCmd( QString name, const QObject *receiver, 
                           const char *member, QString useage,QString helpString )
{
    Q_UNUSED(useage);
    // TODO: add the useage data
    return addCmd(name, receiver,member,helpString);
}

//-------------------------------------------------------------------------
QStringList DcConsoleForm::tokenize(QString& sexp)
{
    sexp.replace("["," [ ");
    sexp.replace("]"," ] ");
    sexp.replace("("," ( ");
    sexp.replace(")"," ) ");
    sexp.replace("{"," { ");
    sexp.replace("}"," } ");
    sexp.replace("\""," \" ");
    sexp.replace("'"," ' ");
    QStringList tokens =   sexp.split(' ',QString::SkipEmptyParts);
    return tokens;
}

//-------------------------------------------------------------------------
bool DcConsoleForm::executeCmdStr(const QString cmdLine)
{
    bool rtval = true;
    DcConArgs args(cmdLine);
    // Expand any symbols defined with 'def'

    // Unless this is a command 'def'
    if(args.cmd() != "def" && args.cmd() != "undef")
    {
        // Lookup command and map any args on the command line if needed.
        if (isDefined(args.cmd()))
        {
            DcFnSymDef fnDef = _fnSym.value(args.cmd());
            
            // If def has args
            if(fnDef.args.count())
            {
                Q_ASSERT(fnDef.name == args.cmd());

                if(fnDef.args.count() > args.argCount())
                {
                    *this << args.cmd() << " - error, argument count miss match\n";
                    return false;
                }
                
                // Loop over the expression in the symbol definition and for each
                // defined argument in 'fnDef.args', replace the arg name in the expression
                // with the value specified on the command line
                QString expr = fnDef.expr;
                int i = 1; 
                foreach(QString a, fnDef.args)
                {
                    expr.replace(a,args.at(i++).toString());
                }
                
                // Replace contents of args with the updated expression
                args.clear();
                args.parseCmdLine(expr);
                args.setMeta("doc",fnDef.doc);
            }
        }
        
        applySymbols(args,_roSym);
        
        // Convert the fnSyms into a map applySymbols can understand.
        QMap<QString,QString> m;
        foreach(QString k, _fnSym.keys())
        {
            m.insert(k,_fnSym.value(k).expr);	
        }
        
        applySymbols(args,m);
    }

    if(!execCmd(args))
    {
        print(args);   
        rtval = false;
    }

    return rtval;
}


//-------------------------------------------------------------------------
bool DcConsoleForm::execCmd( DcConArgs &args )
{
    bool rtval = false;

    // check the user defined commands
    if (_cmdHash.contains(args.cmd()))
    {
        DcConItem conItem = _cmdHash.value(args.cmd());

        // See if the arg counts match, if not, look for a better match
        if((conItem.methodArgCnt==0 && args.argCount() != 0) || (conItem.methodArgCnt==1 && args.argCount()==0))
        {
            QMultiHash<QString, DcConItem>::iterator i = _cmdHash.find(args.cmd());
            while (i != _cmdHash.end() && i.key() == args.cmd()) 
            {
                if(i.value().methodArgCnt == args.argCount())
                {
                    conItem = i.value();
                    break;
                }
                ++i;
            }        
        }
        rtval = true;

        if(conItem.methodArgCnt == 1)
        {
            // Assume that all one argument commands take a DcConAgrs param
            QMetaObject::invokeMethod(const_cast<QObject *>(conItem.reciver), 
                conItem.methodName.constData(), Qt::QueuedConnection,Q_ARG(DcConArgs,args));
        }
        else if(conItem.methodArgCnt == 0)
        {
            QMetaObject::invokeMethod(const_cast<QObject *>(conItem.reciver), 
                conItem.methodName.constData(), Qt::QueuedConnection);
        }
        else
        {
            rtval = false;
        }
    }	return rtval;
}

//-------------------------------------------------------------------------
void DcConsoleForm::cmd_clear( DcConArgs args )
{
    Q_UNUSED(args);
    clear();
}

//-------------------------------------------------------------------------
void DcConsoleForm::cmd_help( DcConArgs args )
{
    Q_UNUSED(args);

    QString s;
    QTextStream txtStrm(&s);
    if(false == _con_html)
    {
        txtStrm.setFieldWidth(20);
        txtStrm.setFieldAlignment(QTextStream::AlignLeft);
        txtStrm << "Command Name";
        txtStrm << " | Help Text";
        *this << s << "\n";
        s.clear();
        *this << "-------------------- | ----------------------------\n";
        
        // Print the user command help
        QHashIterator<QString, DcConItem> i(_cmdHash);
        while (i.hasNext()) 
        {
            i.next();
            txtStrm.setPadChar('.');
            txtStrm << i.key().trimmed();
            txtStrm.setPadChar(' ');
            txtStrm << " | " + i.value().helpString;
            *this << s << "\n";
            s.clear();
        }
    }
    else
    {
       s.clear();
        txtStrm <<  "<table border='0' cellspacing='0' cellpadding='0' bgcolor='silver' >";
        txtStrm << "<tr>";
        txtStrm << "<td><b>Command Name   </b></td>";
        txtStrm << "<td><b>Help Text</b></td>";
        txtStrm << "</tr>";
        
        QHashIterator<QString, DcConItem> i(_cmdHash);
        while (i.hasNext()) 
        {
            i.next();
            txtStrm << "<tr><td>" << i.key().trimmed() << "</td>";
            txtStrm << "<td>" << i.value().helpString << "</td></tr>";
        }
        txtStrm << "</table>\n";
        *this << s;
    }
}

//-------------------------------------------------------------------------
void DcConsoleForm::cmd_depth( DcConArgs args )
{
    if(args.noArgs())
    {
        // Display the depth
        *this << "depth is " << _depth << " lines\n";
    }
    else if(args.oneArg())
    {
        _depth = args.at(1,_depth).toInt();
        // clear the scrip so depth change takes affect
        clear();
        // Display the depth
        *this << "depth=" << _depth << " lines\n";
    }
    else
    {
        *this << args.meta("doc") << "\n";
    }

}

//-------------------------------------------------------------------------
void DcConsoleForm::setInputReady( bool en )
{
    if(!en)
    {
        ui->lineEdit->setText("busy...");
    }
    else
    {
        ui->lineEdit->clear();
    }
    
    ui->lineEdit->setEnabled(en);
}

//-------------------------------------------------------------------------
void DcConsoleForm::cmd_def( DcConArgs args )
{

    if(args.oneArg())
    {
        cmd_undef(args);
        return;
    }

    if(!checkArgCnt(args,2))
    {
        return;
    }
    
    DcFnSymDef symDef = parseFn(args.toString());
    if (_roSym.contains(symDef.name))
    {
        *this << "can not redefined symbol: " << symDef.name << "\n";
    }
    _fnSym.insert(symDef.name,symDef);
}

//-------------------------------------------------------------------------
DcFnSymDef DcConsoleForm::parseFn(QString sexp)
{
    QStringList tokens = tokenize(sexp);
    DcFnSymDef symDef;

    if(tokens.length() < 3)
        return DcFnSymDef();

    if(tokens.takeFirst() == "def")
    {
        symDef.name = tokens.takeFirst();

        QString t = tokens.takeFirst();
        if(t == "\"")
        {
            while(tokens.length() && tokens.at(0) != "\"")
            {
                t = tokens.takeFirst();
                symDef.doc.append(t + " ");
            }

            if(tokens.length() < 2)
                throw parseError("error in expression");
            t = tokens.takeFirst();
            Q_ASSERT(t == "\"");
            symDef.doc = symDef.doc.trimmed();
             t = tokens.takeFirst();
        }
        
        if(t == "[")
        {
            while(tokens.length() && tokens.at(0) != "]")
            {
                t = tokens.takeFirst();
                if(t == "[")
                    throw parseError("misplaced '[' found, expected token or ']'");
                symDef.args.append(t);
            }

            if(tokens.length() < 2)
                throw parseError("error in expression");
            t = tokens.takeFirst();
            Q_ASSERT(t == "]");
        }
        symDef.expr = tokens.join(' ');
    }
    return symDef;
}
//-------------------------------------------------------------------------
bool DcConsoleForm::checkArgCnt( DcConArgs &args, int minCnt /*=0*/ )
{
    // See if this is a request for help  
    if(args.oneArg())
    {
        if("help" == args.first())
        {
            *this << "help";
        }
    }
    
    if(args.argCount() < minCnt)
    {
        *this << "invalid arg count < " << minCnt << " \n";
        return false;
    }
    return true;
}

//-------------------------------------------------------------------------
void DcConsoleForm::addSymDef( QString name, QString d )
{
    DcFnSymDef sym;
    sym.name = name;
    sym.expr = d;
    _fnSym.insert(name,sym);
//    _cmdSym.insert(name, d);
}

//-------------------------------------------------------------------------
void DcConsoleForm::addRoSymDef( QString name, QString d )
{
    _roSym.insert(name, d);
}


//-------------------------------------------------------------------------
void DcConsoleForm::cmd_defSave( DcConArgs args )
{
    if(!checkArgCnt(args,1))
    {
        return;
    }
    
    QString fullFName = QDir::toNativeSeparators(_basePath + args.at(1).toString());
    QFile file(fullFName);
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);

    // Write a header with a "magic number" and a version
    out << (quint32)DcConsoleForm::kDefSymMagicNumber;
    out << (qint32)1;
    out.setVersion(QDataStream::Qt_4_9);
    out << _fnSym;
    
}

//-------------------------------------------------------------------------
// Method saves commands defined at runtime using the 'def' command to the 
// specified file.
void DcConsoleForm::cmd_defLoad( DcConArgs  args )
{

    if(!checkArgCnt(args,1))
    {
        return;
    }

    QString fullFName = QDir::toNativeSeparators(_basePath + args.at(1).toString());

    QFile file(fullFName);
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);

    // Read and check the header
    quint32 magic;
    in >> magic;
    if (magic != kDefSymMagicNumber)
    {
        *this << "invalid file signature\n";
        return;
    }

    // Read the version
    qint32 version;
    in >> version;
    if (version < 1)
    {
        *this << "file version too old\n";
        return;
    }
    if (version > 1)
    {
        *this << "file version too old\n";        
        return;
    }

    if (version == 1)
    {
        in.setVersion(QDataStream::Qt_4_9);
    }

    // Read the data
    in >> _fnSym;

}

//-------------------------------------------------------------------------
// Method loads commands defined at runtime using the 'def' command to the 
// specified file.
void DcConsoleForm::cmd_lsDef( DcConArgs  args )
{
    Q_UNUSED(args);
    QString s;
    QTextStream txtStrm(&s);

    txtStrm.setFieldWidth(20);
    txtStrm.setFieldAlignment(QTextStream::AlignLeft);
    txtStrm << "Name";
    txtStrm << " | Definition";
    *this << s << "\n";
    s.clear();
    *this << "-------------------- | ----------------------------\n";
    
    // Print the user defined command/symbols

    QMapIterator<QString, DcFnSymDef> i(_fnSym);
    while (i.hasNext()) 
    {
        i.next();
        txtStrm.setPadChar('.');
        txtStrm << i.key().trimmed();
        txtStrm.setPadChar(' ');
        txtStrm << " | " + i.value().doc;
        *this << s << "\n";
        s.clear();
    }
    QMapIterator<QString, QString> x(_roSym);
    while (x.hasNext()) 
    {
        x.next();
        txtStrm.setPadChar('.');
        txtStrm << x.key().trimmed();
        txtStrm.setPadChar(' ');
        txtStrm << " | " + x.value();
        *this << s << "\n";
        s.clear();
    }
}

//-------------------------------------------------------------------------
void DcConsoleForm::setBaseDir( QString& basePath )
{
    _basePath  = QDir::toNativeSeparators(basePath + "console/");
    QDir().mkpath(_basePath);
}

//-------------------------------------------------------------------------
void DcConsoleForm::execCmd( const QString cmd )
{
    executeCmdStr(cmd);
}

//-------------------------------------------------------------------------
void DcConsoleForm::applySymbols( DcConArgs &args, const QMap<QString,QString>& syms )
{
    if(syms.count() <= 0)
        return;

    if(args.count() <= 0 )
        return;
    
    QString lastExpand;
    bool expanded = false;
    do
    {
        expanded = false;
        for (int i = 0; i < args.count() ; i++)
        {
            if (syms.contains(args.at(i).toString()))
            {
                if(lastExpand == syms.value(args.at(i).toString()))
                    continue;

                args[i] = syms.value(args.at(i).toString());
                lastExpand = args[i].toString();
                expanded = true;
            }
        }

        if(expanded)
        {
            DcConArgs args2(args.toString());
            args = args2;
        }
    } while (expanded);
}

//-------------------------------------------------------------------------
void DcConsoleForm::clearRoSymDefs()
{
    _roSym.clear();
}

//-------------------------------------------------------------------------
void DcConsoleForm::print( DcConArgs &args, int offset /*= 0*/)
{
    QString etext;
    for (int i = offset; i < args.count() ; i++)
    {
        etext += args.at(i).toString() + " ";	
    }
    *this << etext.trimmed() << "\n";
}

//-------------------------------------------------------------------------
void DcConsoleForm::cmd_append( DcConArgs args )
{
    if(!_appendOk)
        return;
    
    
    if(!checkArgCnt(args,1))
    {
    	return;
    }
	QString val = args.at(1).toString();
    // setNoClrOnReturnOnce(true);
    ui->lineEdit->setText(_curLineEditText + " " + val);
    
}

//-------------------------------------------------------------------------
void DcConsoleForm::cmd_history( DcConArgs args )
{
    Q_UNUSED(args);
    for (int i = 0; i < _history.size(); ++i) 
    {
        *this << _history.at(i) << "\n";
    }
}

//-------------------------------------------------------------------------
void DcConsoleForm::cmd_undef( DcConArgs args )
{
    QString sym = args.at(1).toString();
    if (_roSym.contains(sym))
    {
        *this << "immutable, can not undef: " << sym << "\n";
    }
    else
    {
        _fnSym.remove(sym);
    }
}


//-------------------------------------------------------------------------
void DcConsoleForm::cmd_conHtml( DcConArgs args )
{
    if(checkArgCnt(args,1))
    {
        if(args.firstTruthy())
        {
            _con_html = true;
            *this << "ok\n";
        }
        else
        {
            _con_html = false;
            *this << "ok\n";
        }
    }
}


//-------------------------------------------------------------------------
void DcConsoleForm::requestRefresh()
{
    QString &buf=stream->buffer;
    // Console only outputs if a newline is found
    if(!stream->buffer.contains("\n"))
    {
        return;
    }

    // -------------------------------------------
    // FILTER OUTPUT

    // Messages can be filtered from the display.
    if(stream->buffer.contains("F8\n"))
    {
        stream->buffer.clear();
        return;
    }


    // Example, filter Active Sense MIDI messages
    if(stream->buffer.contains("FE\n"))
    {
        stream->buffer.clear();
        return;
    }

    // -----------------------------------------
    // TRANSFORM OUTPUT
    if(_con_html)
    {
        // only prepend the html break if there's not already an
        // html tag of some kind.
        if( ! buf.contains(QRegExp("(<.+|/>)")))
        {
            buf.prepend("<br>");
        }
        
        // Always replace the newlines
        buf.replace('\n',"");

        // Special MIDI case
        if(buf.contains("IN:"))
        {
            buf.replace("IN: ","<font color = \"#1F9319\">");
            buf.append("</font>");
        }

        // Special MIDI OUT case
        if(buf.contains("OUT:"))
        {
            buf.replace("OUT: ","<font color = \"#C9223E\">");
            buf.append("</font>");
        }
    }

    _textOutputLines.append(stream->buffer);
    stream->buffer.clear();

    if(_textOutputLines.count()>_depth)
    {
        _textOutputLines.removeFirst();
    }
    _updateTimer.start();
}

//-------------------------------------------------------------------------
void DcConsoleForm::cmd_exit(DcConArgs args)
{
    Q_UNUSED(args);
    QApplication::quit();
}

//-------------------------------------------------------------------------
void DcConsoleForm::clear()
{
    _clearOutput = true;
    refreshTextOutput();
}

//-------------------------------------------------------------------------
void DcConsoleForm::cmd_ver( DcConArgs args )
{
    Q_UNUSED(args);
    *this << QApplication::applicationVersion() << "\n";
}

//-------------------------------------------------------------------------
void DcConsoleForm::cmd_echo(DcConArgs args )
{
    print(args,1);
}

//-------------------------------------------------------------------------
void DcConsoleForm::cmd_aboutQt( DcConArgs args )
{
    Q_UNUSED(args);
    QApplication::aboutQt();
}

//-------------------------------------------------------------------------
void DcConsoleForm::cmd_doc( DcConArgs args )
{
    if(args.argCount())
    {
        QString c = args.first().toString();
        if (_fnSym.contains(c))
        {
            DcFnSymDef fnDef = _fnSym.value(c);
            args.setMeta("doc",fnDef.doc);
        }
        else if(_cmdHash.contains(c))
        {
             DcConItem conItem = _cmdHash.value(c);
             args.setMeta("doc",conItem.helpString);
        }
    
        *this << c << " : " << args.meta("doc") << "\n";
    }
}

//-------------------------------------------------------------------------
void DcConsoleForm::cmd_ts( DcConArgs args )
{
    Q_UNUSED(args);
    quint64 now = QDateTime::currentMSecsSinceEpoch();
    quint64 delta = now - _lastTs;
    _lastTs = now;
    *this << delta << "ms\n";
}
