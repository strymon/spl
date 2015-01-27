#include "DcConArgs.h"
#include "DcMidi/DcMidiData.h"

DcConArgs::DcConArgs( const DcConArgs &other )
{

    _args = other._args;
    _cmdName = other._cmdName;
    _meta = other._meta;
    _query = other._query;
}

QString DcConArgs::hexJoin( int offset /*= 1*/, int len /*= 0*/ )
{
    QString st;
    len += count();
    for (int i = offset; i < len ; i++)
    {
        st.append(_args.at(i).toString() + " ");
    }
    return st.trimmed();
}

QString DcConArgs::decJoin( int offset /*= 1*/, int len /*= 0*/ )
{
    QString st;
    len += count();
    for (int i = offset; i < len ; i++)
    {
        QString strArg = _args.at(i).toString();
        if(strArg.contains("0x"))
        {
            if(strArg.length() > 2)
            {

                st.append(strArg.mid(2) + " ");
            }
        }
        else
        {
            bool ok;
            int val = (unsigned char)_args.at(i).toInt(&ok);
            if(ok)
            {
                QString s;
                s.sprintf("%02X ",val);
                st.append(s);
            }
        }
    }
    return st.trimmed();
}

QString DcConArgs::strJoin( int offset /*= 1*/, int len /*= 0*/ )
{
    QString st;
    len += count();
    for (int i = offset; i < len ; i++)
    {
        st.append(_args.at(i).toString());
    }
    DcMidiData md(st);
    return st;
    
}

QString DcConArgs::strScan()
{
    QString st;
    int len = count();
    int start = 0, cnt = 0;
    int i = 1;

    for (; i < len ; i++)
    {
        if("\"" == _args.at(i).toString())
        {
            start = i++;
            break;
        }
    }

    for (; i < len ; i++)
    {
        if("\"" != _args.at(i).toString())
        {
            cnt++;
        }
        else
        {
           st = strJoin(start,cnt);
           break;
        }
    }
    return st;
}

QString DcConArgs::toString()
{
    QString st;
    int len = count();
    for (int i = 0; i < len ; i++)
    {
        QString t = _args.at(i).toString();
        if(t.contains(" "))
        {
            st.append("\"" + _args.at(i).toString() + "\"");
        }
        else
        {
            st.append( _args.at( i ).toString() + " " );
        }
    }
    return st.trimmed();    
}

QStringList DcConArgs::argsAsStringList()
{
    QStringList rtval;

    for (int idx = 1; idx < _args.count() ; idx++)
    {
    	rtval << _args.at(idx).toString();
    }
    return rtval;
}

QVariant DcConArgs::first(QVariant def)
{
    if(_args.count() > 1)
        return _args.at(1);
    return def;
}

QVariant DcConArgs::second(QVariant def)
{
    if(_args.count() > 2)
        return _args.at(2);

    return def;
    
}

QVariant DcConArgs::third(QVariant def)
{
    if(_args.count() > 3)
        return _args.at(3);

    return def;
}

QString DcConArgs::meta( QString key,const char* def/*=""*/ )
{
    if(_meta.contains(key))
    {
        return _meta[key].toString();
    }
    return QString(def);
}

void DcConArgs::setMeta( QString key,QVariant val )
{
    _meta[key] = val;
}

void DcConArgs::setCmdName( QString name )
{
    _cmdName = name;
}

void DcConArgs::clear()
{
    _cmdName.clear();
    _meta.clear();
    _query = false;
    _args.clear();
}

bool DcConArgs::queryCmd( QString name, QString useage, QString doc )
{
    if(this->isQuery())
    {
        this->setMeta("doc",doc);
        this->setMeta("useage",useage);
        this->setCmdName(name);
        return true;
    }

    return false;
}

void DcConArgs::parseCmdLine( const QString &cmd )
{
    // Credit: http://www.qtcentre.org/threads/37304-Split-strings-using-QStringList-split()-but-ignore-quotes

    if(cmd.isEmpty())
        return;

    bool inside = (cmd.at(0) == '\"'); //true if the first character is "
    QStringList tmpList = cmd.split(QRegExp("\""), QString::SkipEmptyParts); // Split by " and make sure you don't have an empty string at the beginning
    foreach (QString s, tmpList)
    {
        if (inside)
        {
            // If 's' is inside quotes ...
            _args.append(s); // ... get the whole string
        } else
        {
            // If 's' is outside quotes ...
            QStringList lst = s.split(" ", QString::SkipEmptyParts); // ... get the spitted string
            for (int i = 0; i < lst.count() ; i++)
            {
                _args.append(lst.at(i));
            }
        }
        inside = !inside;
    }   
}






