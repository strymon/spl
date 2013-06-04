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
#pragma once

#include <QTextStream>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QList>


class DcConArgs 
{

public:    

    DcConArgs() {clear();}
    DcConArgs(const DcConArgs &other);
   ~DcConArgs() {};


   bool queryCmd(QString name, QString useage, QString doc);

    /*!
      Initializes the object by parsing the given string.
      The parser simply splits the string by ' ' chars.
      Example: DcConArgs("mycmd arg1 arg2 arg3")
    */ 
    DcConArgs(const QString& str)
    {
        clear();
        parseCmdLine(str);

    }

    void parseCmdLine( const QString &str );


    /*!
      Returns argument count + 1
    */ 
    inline int count() const {return _args.count();}
    
    /*!
      Returns the command string
    */ 
    QString cmd() const {return count() ? _args.at(0).toString() : QString("");}
    /*!
      Returns the command argument count
    */ 
    inline int argCount() const {return _args.count()-1;}
    /*!
      Returns the value at idx (0 is the command string, 1 is the first argument)
      def is returned if the idx is grater than the arg count.
    */ 
    QVariant at(int idx,QVariant def) const {return (count()>idx) ? _args.at(idx) : def;}
    /*!
      Returns the value at idx, where 0 is the command string and 1 would return the first argument.
    */ 
    QVariant at(int idx) const {return (count()>idx) ? _args.at(idx) : QString("");}
    
    /*!
      Converts the arguments into a string of base 16 values (hex), for example,
      if the command line was 'mycmd 5 10 17' then hexJoin(1) would produce: "05 0A 11" 
    */
    QString hexJoin(int offset = 1, int len = 0);

    /*!
      Converts the arguments into a string stating at the given argument offset.
    */ 
    QString strJoin(int offset = 1, int len = 0);
    
    // Unused - 
    QString strScan();

    inline QVariant &operator[](int j)
    {
        return _args[j];
    }

    QString toString();

    /*!
      Returns true if no arguments are available
    */ 
    inline bool noArgs() {return argCount() == 0;}
    /*!
      Returns true if one argument is available
    */ 
    inline bool oneArg() {return argCount() == 1;}
    /*!
      Returns true if two arguments are available
    */ 
    inline bool twoArgs() {return argCount() == 2;}
    /*!
      Returns true if three arguments are available
    */ 
    inline bool threeArgs() {return argCount() == 3;}

    /*!
      Returns the first argument
    */ 
    QVariant first();
    /*!
      Returns the second argument
    */ 
    QVariant second();
    /*!
      Returns the third argument
    */ 
    QVariant third();

    inline bool firstTruthy()
    {
        if(argCount() >= 1)
        {
            QString a = first().toString().toLower();
            return a == "1" || a == "on" || a == "yes" || a == "true";
        }
        return false;
    }
    
    /*!
      Returns a QString version of the meta-data value for the given key
      For example, to get a command doc string, use 'doc'
      QString doc_string = args.meta("doc");
    */ 
    QString meta(QString key,const char* def="");
    
    /*!
      Sets the meta-data for the given key
    */ 
    void setMeta(QString key,QVariant val);
    
    
    void setCmdName( QString name );
    /*!
      Returns true if this argument object is a query.
      A query is a house keeping call by the command parser to 
      grab the command name, doc string, and usage from the command.
    */ 
    bool isQuery() {return _query;}
    void setQuery() {_query = true;}
    
    /*!
        Put object in a clean uninitialized state.
    */ 
    void clear();
    inline bool is( const char* c ) { return cmd() == c;}
    inline bool is( QString& qs ) { return cmd() == qs;}

private:
    QList<QVariant>         _args;
    QMap<QString,QVariant>  _meta;
    QString _cmdName;
    bool _query;
};

Q_DECLARE_METATYPE(DcConArgs);


//-------------------------------------------------------------------------


