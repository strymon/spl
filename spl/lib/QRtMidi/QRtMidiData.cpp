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

#include "QRtMidi/QRtMidiData.h"
#include <QStringList>
#include <QDebug>
//#include "DcMidiDevDefs.h"


//-------------------------------------------------------------------------
QRtMidiData::QRtMidiData()
{
}

//-------------------------------------------------------------------------
QRtMidiData::QRtMidiData(const QRtMidiData &other)
{
    _data = other._data;
}

QRtMidiData::~QRtMidiData()
{
}

//-------------------------------------------------------------------------
QRtMidiData::QRtMidiData(const std::vector<unsigned char>& vector)
{
    if(vector.size() > 0)
    {
        _data.append(reinterpret_cast<const char*>(&vector[0]), vector.size());
    }
}

//-------------------------------------------------------------------------
QRtMidiData::QRtMidiData(const qint32 data)
{
    appendNum(data);
}

//-------------------------------------------------------------------------
void QRtMidiData::copyToStdVec( std::vector<unsigned char>& vec ) const
{
    vec.clear();
    int sz = _data.size();
    for (int i = 0; i < sz ; i++)
    {
        vec.push_back(_data.at(i));
    }
}

//-------------------------------------------------------------------------
std::vector<unsigned char> QRtMidiData::toStdVector()
{
    std::vector<unsigned char> vec;
    copyToStdVec(vec);
    return vec;
}

QRtMidiData::QRtMidiData(const QByteArray& ba)
{
    _data = ba;
}

QRtMidiData::QRtMidiData(const QString& st)
{
    const std::string str = st.toStdString();
    setText(str.c_str());
    
}

//-------------------------------------------------------------------------
QRtMidiData::QRtMidiData(const char* data)
{
    setText(data);
}

// Set data with variadic function arguments
// vv    - single MIDI byte
// v14   - 14bit MIDI param
// mstr  - a c string of MIDI bytes e.g. "00 01 55"
// cstr  - a 7bit ascii c-string e.g. "Hello"
// Format specifers must be sperated by a space
// Literal MIDI bytes must be in hex
// Examples:
// -----------------------------------------
// setData("F0 lst F7","01 02 03") => F0 01 02 03 F7
// "F0 12 str F7","Hello" => F0 12 48 65 6C 6C 6F F7
// "F0 vv vv vv F7",1,2,16 => F0 01 02 10 F7
// "F0 63 v14 F7", 0x222 => "F0 63 02 22 F7"
// ----------------------------------------

//-------------------------------------------------------------------------
/*
void QRtMidiData::setData( const QString& fmt, ... )
{
    QByteArray ba = fmt.toStdString().c_str();
    clear();

    va_list args;
    va_start(args, fmt);
    vSetData(ba.data(), args);
}*/

void QRtMidiData::setData(const char* fmt, ...)
{
    clear();

    va_list args;
    va_start(args, fmt);
    vSetData(fmt, args);
    va_end(args);

}

//-------------------------------------------------------------------------
void QRtMidiData::setText( const char* data )
{
    QString str = QString(QByteArray(data));

    str = str.trimmed();
    int len = str.length();

    if(len < 2)
        return;

    QStringList strList;

    if( str.count(" ") || str.count(","))
    {
        strList = str.split(QRegExp("[ ,]+"));
    }
    else
    {
        // At this point, 'str' holds a string like this: 0x1234 or F06677AA
        // Check for the '0x' string, then the non-delim/non-0x string.
        // Assume all strings passed in as char* must be hex

        if(is0xHexStr(str))
        {
            strList.append(str);
        }
        else
        {
            // For a legal hex midi string without spaces, it must be
            // divisible by 2
            if((str.length()%2) != 0)
            {
                qDebug() << "DcMidiData::DcMidiData(const char*,int) - invalid hex string format: " << str;
            }
            else
            {
                for (int idx = 0; idx < str.length() ; idx++)
                {
                    QString tstr;
                    tstr.append(str[idx++]);
                    tstr.append(str[idx]);
                    strList.append(tstr);
                }
            }
        }
    }
    // Convert the string to hex
    foreach(QString s, strList)
    {
        if(s.isEmpty())
            continue;

        bool ok = true;
        int x = s.toInt(&ok,16);
        if(!ok)
        {
            qDebug() << "DcMidiData::DcMidiData(const char*,int) - error converting hex string: " << str;
        }
        else
        {
            if(!appendNum(x))
                break;
        }
    }
}



//-------------------------------------------------------------------------
QString QRtMidiData::toString() const
{
    QString s = QString(_data.toHex()).toUpper();
    return s;
}

QString QRtMidiData::toString(const char ch) const
{
    QString s = QString(_data.toHex()).toUpper();
    int len = s.length();
    for (int i = 2; i < len ; i+=2)
    {
        s.insert(i++,ch);
        len++;
    }
    
    return s;
}

/*
List<QByteArray> QRtMidiData::toMidiByteList() const
{
    QList<QByteArray> lst;
    QString s = toString();
    while(!s.isEmpty())
    {
        if(s.length() >= 2)
        {
            bool ok = true;
            unsigned char val = s.mid(0,2).toInt(&ok,16);
            QByteArray ba;
            ba.Nu
            if(!ok)
                break;
            else
                lst.append(Q)


        }
    }
    QStringList lst = s.split(
    return s;
}*/
//-------------------------------------------------------------------------
bool QRtMidiData::is0xHexStr( QString &str )
{
    if(str.length() < 3)
        return false;
    
    return str.mid(0,2).contains("0x",Qt::CaseInsensitive);
}

//-------------------------------------------------------------------------
bool QRtMidiData::appendNum(quint32 x )
{
    bool result = false; 
    QByteArray ba = numHexToByteArray(x);
    if(!ba.isEmpty())
    {
        _data.append(ba);
        result = true;
    }
    
    return result;
}

//-------------------------------------------------------------------------
// contains supports "don't care" bytes, specified as 'XX'
// e.g. "F0 00 XX 22" or "F0 00 xx 22"
bool QRtMidiData::contains( const char* val) const
{
    QByteArray ba = val;
    if(ba.lastIndexOf('x') > 0 || ba.lastIndexOf('X') > 0)
    {
        // Remove spaces, force uppercase
        ba = ba.replace(" ","").toUpper();
        
        int len = ba.length();
        QByteArray thisData = _data.toHex().toUpper();
        for (int idx = 0; idx < len ; idx++)
        {

            if(ba[idx] == 'X')
                continue;

            if(ba[idx] != thisData[idx])
            {
                return false;
            }
        }
        return true;
    }
    else
    {
        QRtMidiData md(val); 
        return _data.contains(md._data);
    }
}

//-------------------------------------------------------------------------
bool QRtMidiData::contains( const QByteArray& ba) const
{
    return _data.contains(ba);
}

bool QRtMidiData::contains(const QRtMidiData& md) const
{
    return _data.contains(md.toByteArray());
}

//-------------------------------------------------------------------------
unsigned char QRtMidiData::sumOfSection( int start, int len)
{
    int accum = 0;
    QByteArray ba = _data.mid(start,len);
    for (int idx = 0; idx < ba.size() ; idx++)
    {
    	accum += (unsigned char) (0x7F&ba[idx]);

    }
    return (0x7F&accum);
}

//-------------------------------------------------------------------------
QRtMidiData QRtMidiData::left(int len ) const
{
    if (len >= _data.size())
        return *this;
    if (len < 0)
        len = 0;
    return QRtMidiData(QByteArray(_data.data(), len));
}

//-------------------------------------------------------------------------
QByteArray QRtMidiData::mid( int pos, int len /*= -1 */ ) const
{
    return _data.mid(pos,len);
}

//-------------------------------------------------------------------------
QByteArray QRtMidiData::replace( int pos, int len, const QByteArray & after )
{
    return _data.replace(pos,len,after);
}

//-------------------------------------------------------------------------
int QRtMidiData::get14bit(int offset, int defaultValue /*= 0*/) const
{
    int val = defaultValue;
    
    if( offset <= _data.length())
    {
        val = _data.at(offset+1) + (_data.at(offset) << 7);
    }
    
    return val;
}

bool QRtMidiData::set14bit(int offset, int val)
{
    bool rtval = false;

    if( (offset+1) <= _data.length())
    {
        _data[offset] = getHi(val);
        _data[offset+1] = getLo(val);
        rtval = true;
    }

    return rtval;
}

//-------------------------------------------------------------------------
void QRtMidiData::clear()
{
    _data.clear();
}

//-------------------------------------------------------------------------
QByteArray QRtMidiData::numHexToByteArray( quint32 x)
{
    QByteArray ba;

    if(x < 0xFF)
    {
        ba.append((char)x);
    }
    else if(x < 0xFFFF)
    {
        unsigned char hi = (x<<8);
        unsigned char lo = x&0xFF;
        ba.append((char)hi);
        ba.append((char)lo);
    }
    else if(x < 0xFFFFFF)
    {
        unsigned char hi = (x>>16);
        unsigned char m = (x>>8)&0xFF;
        unsigned char lo = x&0xFF;
        ba.append((char)hi);
        ba.append((char)m);
        ba.append((char)lo);
    }
    else if(x < 0xFFFFFFFF)
    {
        unsigned char hi = (x>>24);
        unsigned char mh = (x>>16)&0xFF;
        unsigned char ml = (x>>8)&0xFF;
        unsigned char lo = x&0xFF;
        ba.append((char)hi);
        ba.append((char)mh);
        ba.append((char)ml);
        ba.append((char)lo);
    }
    else
    {
        qDebug() << "DcMidiData::numHexToByteArray(quint32) - error";
    }	
    
    return ba;
}

//-------------------------------------------------------------------------
void QRtMidiData::vSetData( const char* fmt, va_list args )
{
    QByteArray ba = fmt;
    ba = ba.toUpper();
    QString ss = ba.data();
    QStringList lst = ss.split(' ');

    for (int idx = 0; idx < lst.length() ; idx++)
    {
        QString val = lst.at(idx);

        if (val == "VV") 
        {
            int i = va_arg(args, int);
            QString ts;
            ts.sprintf("%02X",i);
            if(!appendNum(ts.toInt(0,16)))
                Q_ASSERT(false /*appendNum failed*/);
        } 
        else if (val == "P14") 
        {
            int p14 = va_arg(args, int);
            QString ts;
            ts.sprintf("%02X",getHi(p14));
            if(!appendNum(ts.toInt(0,16)))
                Q_ASSERT(false /*appendNum failed*/);            
            ts.sprintf("%02X",getLo(p14));
            if(!appendNum(ts.toInt(0,16)))
                Q_ASSERT(false /*appendNum failed*/);            
        } 
        else if (val == "MSTR")
        {
            char *strVal = va_arg(args, char *);
            QRtMidiData m(strVal);
            if(!m.isEmpty())
            {
                for (int hidx = 0; hidx < m.length() ; hidx++)
                {
                    _data.append(m.at(hidx));
                }
            }
            else
            {
                lst.removeAt(idx);
                qDebug() << "Invalid value for format( " << fmt << " ) at specifier " << val;
            }
        }
        else if (val == "CSTR")
        {
            char *strVal = va_arg(args, char *);
            QByteArray ba(strVal);
            if(!ba.isEmpty())
            {
                for (int hidx = 0; hidx < ba.length() ; hidx++)
                {
                    _data.append(ba.at(hidx));
                }
            }
            else
            {
                lst.removeAt(idx);
                qDebug() << "Invalid value for format( " << fmt << " ) at specifier " << val;
            }
        }
        else
        {
            bool ok = false;
            int v = val.toInt(&ok,16);
            if(!ok)
            {
                Q_ASSERT(false /*toInt failed*/);
            }
            else
            {
                appendNum(v);
            }

        }

    }
}

//-------------------------------------------------------------------------
int QRtMidiData::toInt( int offset, int cnt, int defaultValue /*= 0 */ ) const
{
   int val = defaultValue;

   if(cnt > 0 && cnt <= 4)
   {
       if( offset+cnt <= _data.length())
       {
           int newVal = 0;
           int sft = 0;
           for (int i = cnt - 1; i >= 0 ; i--)
           {
               newVal += _data.at(offset+i) << (8*sft++);
           }
           val = newVal;
       }
   }

    return val;

}




//-------------------------------------------------------------------------
void testRtMidiData()
{
//    static const char* tdata = "0xF01233";
//    QRtMidiData mdt1("F0F67FF7");
//    QRtMidiData mdt2("F0 F6 7F F7");

//    bool t = mdt1 == mdt2;
//    t = mdt2 == "F0F67FF7";
//    t = mdt2 == "F0F67F";
//    t = mdt1 == "F0 F6 7F F7";
//    t = mdt1 == "0xF0 0xF6 0x7F 0xF7";
//    QRtMidiData mdt3(tdata);
}
