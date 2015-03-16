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
#ifndef DCCONSOLEFORM_H
#define DCCONSOLEFORM_H

#include <QWidget>
#include <QTextStream>
#include <QString>
#include <QTimer>
#include <QMultiHash>
#include "DcConArgs.h"
#include "DcConItem.h"

namespace Ui {
class DcConsoleForm;
}


struct DcFnSymDef
{
    DcFnSymDef() {}
    QString name;
    QStringList args;
    QString expr;
    QString doc;
};

QDataStream & operator<< (QDataStream& stream, const DcFnSymDef& symDef);
QDataStream & operator>> (QDataStream& stream, DcFnSymDef& symDef);

class DcConsoleForm : public QWidget
{
    Q_OBJECT

private:
    Ui::DcConsoleForm *ui;
    
    

    struct ConStream 
    {
        ConStream() : ts(&buffer, QIODevice::WriteOnly), space(false) {}
        QTextStream ts;
        QString buffer;
        bool space;
    } *stream;

   const static quint32 kDefSymMagicNumber = 0xD000C000; // sym def file id
public:
    
    explicit DcConsoleForm(QWidget *parent = 0);
    ~DcConsoleForm();

    /*!
      toggle the console form visibility
    */ 
    void toggleVisible();


    /*!
     * \brief getSbWidth will return the width of the console scroll bar
     * \return
     */
    int initGuiElements();

    void incCounterDisplay(int value);
    void clearCounterDisplay();
   
    /*!
      set true, and the console input will except key presses, set false
      to prevent input and display the text 'busy...'
    */ 
    void setInputReady( bool en );

    /*!
      clear the read only symbol definitions
    */ 

    void clearRoSymDefs();
    /*!
      Sets the base path used for file IO
    */ 
    void setBaseDir( QString& basePath );

    // Register a command handler
    
    // addCmd - binds the name to the reciver/SLOT pair and associates a help string
    // reciver slot/member functions can take zero or one argument.  When one argument
    // is specified it shall be DcConArgs.
    bool addCmd(QString name, const QObject *receiver, const char *member, QString helpString);
    bool addCmd(QString name, const QObject *receiver, const char *member, QString useage,QString helpString);

    void loadHistory();
    void saveHistory();


    // Cause the text output to display
    void requestRefresh();

    
    /*!
      Adds a name and definition to the sym definition list.
    */ 
    void addSymDef(QString name, QString d);
    /*!
      Add a read only name/definition to the ro sym hash
    */ 
    void addRoSymDef(QString name, QString d);
    void addRoSymDef(QString name, int val);
    /*!
      Execute given command string
    */ 
    void execCmd(const QString cmd,bool blocking = false);

    // Text output
    void flush();
     
    void autoCmd(QString cmd);

    // Inspired by QDebug, implement stram operators for console text output.
    bool autoInsertSpaces() const { return stream->space; }
    void setAutoInsertSpaces(bool b) { stream->space = b; }
    inline DcConsoleForm &space() { stream->space = true; stream->ts << ' '; return *this; }
    inline DcConsoleForm &nospace() { stream->space = false; return *this; }
    inline DcConsoleForm &maybeSpace() { if (stream->space) {stream->ts << ' '; requestRefresh();} return *this; }
    inline DcConsoleForm &operator<<(QChar t) { stream->ts << t; requestRefresh(); return maybeSpace(); }
    inline DcConsoleForm &operator<<(bool t) { stream->ts << (t ? "true" : "false"); requestRefresh(); return maybeSpace(); }
    inline DcConsoleForm &operator<<(char t) { stream->ts << t; requestRefresh(); return maybeSpace(); }
    inline DcConsoleForm &operator<<(signed short t) { stream->ts << t; requestRefresh(); return maybeSpace(); }
    inline DcConsoleForm &operator<<(unsigned short t) { stream->ts << t; requestRefresh(); return maybeSpace(); }
    inline DcConsoleForm &operator<<(signed int t) { stream->ts << t; requestRefresh(); return maybeSpace(); }
    inline DcConsoleForm &operator<<(unsigned int t) { stream->ts << t; requestRefresh(); return maybeSpace(); }
    inline DcConsoleForm &operator<<(signed long t) { stream->ts << t; requestRefresh(); return maybeSpace(); }
    inline DcConsoleForm &operator<<(unsigned long t) { stream->ts << t; requestRefresh(); return maybeSpace(); }
    inline DcConsoleForm &operator<<(qint64 t)
    { stream->ts << QString::number(t); requestRefresh(); return maybeSpace(); }
    inline DcConsoleForm &operator<<(quint64 t)
    { stream->ts << QString::number(t); requestRefresh(); return maybeSpace(); }
    inline DcConsoleForm &operator<<(float t) { stream->ts << t; requestRefresh(); return maybeSpace(); }
    inline DcConsoleForm &operator<<(double t) { stream->ts << t; requestRefresh(); return maybeSpace(); }
    inline DcConsoleForm &operator<<(const char* t) { stream->ts << QString::fromUtf8(t); requestRefresh(); return maybeSpace(); }
    inline DcConsoleForm &operator<<(const QString & t) { stream->ts << t; requestRefresh(); return maybeSpace(); }
    inline DcConsoleForm &operator<<(const QStringRef & t) { requestRefresh(); return operator<<(t.toString()); }
    inline DcConsoleForm &operator<<(QLatin1String t) { stream->ts << t; requestRefresh(); return maybeSpace(); }
    inline DcConsoleForm &operator<<(const QByteArray & t) { stream->ts  << t; requestRefresh(); return maybeSpace(); }
    inline DcConsoleForm &operator<<(const void * t) { stream->ts << t; requestRefresh(); return maybeSpace(); }
    inline DcConsoleForm &operator<<(QTextStreamFunction f) {
        stream->ts << f;
        requestRefresh(); return *this;}
    inline DcConsoleForm &operator<<(QTextStreamManipulator m)
    { stream->ts << m; requestRefresh(); return *this; }
    

    bool isHtmlMode() const { return _con_html; }
    
    void append(const QString& str, int bytecnt);

public slots:
    virtual void setVisible(bool visible);
     
     
    void on_lineEdit_returnPressed();
    void on_lineEdit_textEdited(const QString &);



private slots:
    
    void refreshTextOutput();
    
    void cmd_echo(DcConArgs args);
    void cmd_depth( DcConArgs args );
    void cmd_conHtml( DcConArgs args );
    void cmd_history( DcConArgs args );
    void cmd_append( DcConArgs args );
    void cmd_help(DcConArgs args);
    void cmd_def( DcConArgs  args );
    void cmd_defSave( DcConArgs  args );
    void cmd_lsDef( DcConArgs  args );
    void cmd_undef( DcConArgs  args );
    void cmd_defLoad( DcConArgs  args );
    void cmd_clear(DcConArgs args);
    void cmd_ver(DcConArgs args);
    void cmd_doc(DcConArgs args);
    void cmd_ts(DcConArgs args);
    void cmd_aboutQt(DcConArgs args);
    void cmd_exit(DcConArgs args);

    

protected:

    // Methods for command line history
    bool eventFilter(QObject* obj, QEvent *e);
    
    void handleUpKeyPress();
    void handleDownKeyPress();

    void replaceCurrentCommand( QString cmd );
    QString getCurrentCommand();
    
    bool executeCmdStr(const QString cmdLine,bool blocking = false);
    bool execCmd( DcConArgs &args, bool direct = false);
    bool checkArgCnt( DcConArgs &args, int minCnt = 0);
    /*!
      Returns true if the command string is defined in the 
      global symbol map
    */ 
    inline bool isDefined(const QString& cmd)
    {
        return _fnSym.contains(cmd);
    }
    
    void applySymbols( DcConArgs &args,  const QMap<QString,QString>& syms);
    void print( DcConArgs &args, int offset = 0);

    void clear();

    
private:

    
    // Commands
    QMultiHash<QString,DcConItem> _cmdHash; // user commands
    QMap<QString,DcFnSymDef> _fnSym;
    
    // Read only Symbols (defined the application to inject data into the console)
    QMap<QString,QString> _roSym; 

    // Text Output
    QTimer          _updateTimer; // naive output synchronization
    bool            _clearOutput;
    int             _depth;
    QStringList     _textOutputLines;
    
    // Command history
    QStringList     _history;
    int             _historyIndex;
    
    // Internal command help string
    QStringList _internalCmdHelp;
    
    QString _basePath;
    
    bool _noClrOnReturnOnce;
    bool _appendOk;
    QString _curLineEditText;
    int _dispCount;
    bool _con_html;
    quint64 _lastTs;
    bool getNoClrOnReturnOnce() const { return _noClrOnReturnOnce; }
    void setNoClrOnReturnOnce(bool val) { _noClrOnReturnOnce = val; }
    QStringList tokenize(QString& sexp);
    DcFnSymDef parseFn(QString sexp);
    

};

#endif // DCCONSOLEFORM_H
