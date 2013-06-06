#include "DcConArgs.h"
#include "QRtMidi/QRtMidiData.h"
//-------------------------------------------------------------------------
DcConArgs::DcConArgs( const DcConArgs &other )
{

    _args = other._args;
    _cmdName = other._cmdName;
    _meta = other._meta;
    _query = other._query;
}

//-------------------------------------------------------------------------
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

//-------------------------------------------------------------------------
QString DcConArgs::strJoin( int offset /*= 1*/, int len /*= 0*/ )
{
    QString st;
    len += count();
    for (int i = offset; i < len ; i++)
    {
        st.append(_args.at(i).toString());
    }
    QRtMidiData md(st);
    return st;
    
}

//-------------------------------------------------------------------------
QString DcConArgs::strScan()
{
    QString st;
    int len = count();
    int start = 0, cnt = 0;
    int i = 1;

    for (i; i < len ; i++)
    {
        if("\"" == _args.at(i).toString())
        {
            start = i++;
            break;
        }
    }

    for (i; i < len ; i++)
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

//-------------------------------------------------------------------------
QString DcConArgs::toString()
{
    QString st;
    int len = count();
    for (int i = 0; i < len ; i++)
    {
        st.append(_args.at(i).toString() + " ");
    }
    return st.trimmed();    
}

//-------------------------------------------------------------------------
QVariant DcConArgs::first()
{
    return _args.at(1);
}

//-------------------------------------------------------------------------
QVariant DcConArgs::second()
{
    return _args.at(2);
}
//-------------------------------------------------------------------------
QVariant DcConArgs::third()
{
    return _args.at(3);
}

//-------------------------------------------------------------------------
QString DcConArgs::meta( QString key,const char* def/*=""*/ )
{
    if(_meta.contains(key))
    {
        return _meta[key].toString();
    }
    return QString(def);
}

//-------------------------------------------------------------------------
void DcConArgs::setMeta( QString key,QVariant val )
{
    _meta[key] = val;
}

//-------------------------------------------------------------------------
void DcConArgs::setCmdName( QString name )
{
    _cmdName = name;
}

//-------------------------------------------------------------------------
void DcConArgs::clear()
{
    _cmdName.clear();
    _meta.clear();
    _query = false;
    _args.clear();
}

//-------------------------------------------------------------------------
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

//-------------------------------------------------------------------------
void DcConArgs::parseCmdLine( const QString &str )
{
    QStringList lst =  str.split(" ",QString::SkipEmptyParts);
    for (int i = 0; i < lst.count() ; i++)
    {
        _args.append(lst.at(i));
    }
}






