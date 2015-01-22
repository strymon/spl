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

#include "DcMidiData.h"
#include <QStringList>
#include <QDebug>


//-------------------------------------------------------------------------
DcMidiData::DcMidiData()
    : _ts(0), _srcDevice(0)
{
}

//-------------------------------------------------------------------------
DcMidiData::DcMidiData(const DcMidiData &other)
    : _ts(0), _srcDevice(0)
{
    _ts = other._ts;
    _srcDevice = other._srcDevice;
    _data = other._data;
}

DcMidiData::~DcMidiData()
{
}

//-------------------------------------------------------------------------
DcMidiData::DcMidiData(const std::vector<unsigned char>& vector,DcMidiIn* source /* = 0*/)
    : _ts(0), _srcDevice(source)
{
    if(vector.size() > 0)
    {
        _data.append(reinterpret_cast<const char*>(&vector[0]), (int)vector.size());
    }
}

//-------------------------------------------------------------------------
DcMidiData::DcMidiData(const qint32 data)
    : _ts(0), _srcDevice(0)
{
    appendNum(data);
}

DcMidiData::DcMidiData(const QByteArray& ba)
    : _ts(0), _srcDevice(0)
{
    _data = ba;
}

DcMidiData::DcMidiData(const QString& st)
    : _ts(0), _srcDevice(0)
{
    QString s = st;
    s.replace("\n","");
    const std::string str = s.toStdString();
    setText(str.c_str());

}

//-------------------------------------------------------------------------
DcMidiData::DcMidiData(const char* data)
    : _ts(0), _srcDevice(0)

{
    setText(data);
}

//-------------------------------------------------------------------------
void DcMidiData::copyToStdVec( std::vector<unsigned char>& vec, int offset /* = 0*/, int len /*= -1 */) const
{
    vec.clear();
    int sz = len == -1 ? _data.size() : len;
    sz = qMin(_data.size()-offset, sz );
    int last = offset + sz;

    for (int i = offset; i < last ; i++)
    {
        vec.push_back(_data.at(i));
    }
}


//-------------------------------------------------------------------------
QList<DcMidiData> DcMidiData::split( int maxSz ) const
{
    QList<DcMidiData> rtlst;
    if(0 == maxSz)
    {
        return rtlst;
    }

    int len = this->length();
    if(len > maxSz)
    {
        int chunkcnt = len / maxSz;
        int remSize= len % maxSz;

        int offset = 0;
        for (int i = 0; i < chunkcnt ; i++)
        {
            
            rtlst.append(DcMidiData(this->mid(offset,maxSz)));
            offset += maxSz;
        }

        if(remSize)
        {
            rtlst.append(DcMidiData(this->mid(offset,remSize)));
        }

    }


    return rtlst;
}


//-------------------------------------------------------------------------
std::vector<unsigned char> DcMidiData::toStdVector(int offset /*=0*/, int len /*= -1*/)
{
    std::vector<unsigned char> vec;
    copyToStdVec(vec,offset,len);
    return vec;
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

void DcMidiData::setData(const char* data, ...)
{
    clear();

    va_list args;
    va_start(args, data);
    vSetData(data, args);
    va_end(args);

}

//-------------------------------------------------------------------------
void DcMidiData::setText( const char* data )
{
    clear();

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
QString DcMidiData::toString() const
{
    QString s = QString(_data.toHex()).toUpper();
    return s;
}

//-------------------------------------------------------------------------
QString DcMidiData::toString(const char ch) const
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

//-------------------------------------------------------------------------
bool DcMidiData::is0xHexStr( QString &str )
{
    if(str.length() < 3)
        return false;
    
    return str.mid(0,2).contains("0x",Qt::CaseInsensitive);
}

//-------------------------------------------------------------------------
bool DcMidiData::appendNum(quint32 x )
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
bool DcMidiData::match( const char* val,bool must_start_with /* = false */) const
{
    QString tst = val;
    tst = tst.replace(" ","").toUpper();
    tst = tst.replace("XX","..");

    QString thisData = this->toString();
    QRegExp rx(tst);
    bool rtval;
    int pos = rx.indexIn(thisData);

    if(must_start_with)
    {
       rtval = (pos == 0);
    }
    else
    {

        rtval = (pos > -1);
    }

    return rtval;
}

//-------------------------------------------------------------------------
bool DcMidiData::match( QRegExp rx,bool must_start_with /*= false*/) const
{
    QString thisData = this->toString();
    bool rtval;
    int pos = rx.indexIn(thisData);

    if(must_start_with)
    {
        rtval = (pos == 0);
    }
    else
    {

        rtval = (pos > -1);
    }

    return rtval;
}
//-------------------------------------------------------------------------
bool DcMidiData::contains( const char* hex_string) const
{
    return this->match(hex_string);
}

//-------------------------------------------------------------------------
bool DcMidiData::contains( const QByteArray& ba) const
{
    return _data.contains(ba);
}

bool DcMidiData::contains(const DcMidiData& md) const
{
    return _data.contains(md.toByteArray());
}

bool DcMidiData::startsWith(const char *hex_string) const
{
    return this->match(hex_string,true);
}

bool DcMidiData::startsWith(const QByteArray& ba) const
{
    return this->match(ba,true);
}

bool DcMidiData::startsWith(const DcMidiData &md) const
{
    return this->match(md.toByteArray(),true);
}

//-------------------------------------------------------------------------
unsigned char DcMidiData::sumOfSection( int start, int len) const
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
DcMidiData DcMidiData::left(int len ) const
{
    if (len >= _data.size())
        return *this;
    if (len < 0)
        len = 0;
    return DcMidiData(QByteArray(_data.data(), len));
}

//-------------------------------------------------------------------------
QByteArray DcMidiData::mid( int pos, int len /*= -1 */ ) const
{
    return _data.mid(pos,len);
}

//-------------------------------------------------------------------------
QByteArray DcMidiData::replace( int pos, int len, const QByteArray & after )
{
    return _data.replace(pos,len,after);
}

//-------------------------------------------------------------------------
int DcMidiData::get14bit(int offset, int defaultValue /*= 0*/) const
{
    int val = defaultValue;
    
    if( offset <= _data.length())
    {
        val = _data.at(offset+1) + (_data.at(offset) << 7);
    }
    
    return val;
}

//-------------------------------------------------------------------------
bool DcMidiData::set14bit(int offset, int val)
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
void DcMidiData::clear()
{
    _ts = 0;
    _srcDevice = 0;
    _data.clear();
}

//-------------------------------------------------------------------------
QByteArray DcMidiData::numHexToByteArray( quint32 x)
{
    QByteArray ba;

    if(x <= 0xFF)
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
void DcMidiData::vSetData( const char* fmt, va_list args )
{
    QByteArray ba = fmt;
    ba = ba.toUpper();
    QString ss = ba.data();
    QStringList lst = ss.split(' ');

    for (int idx = 0; idx < lst.length() ; idx++)
    {
        QString val = lst.at(idx);

        if (val ==  QLatin1String("VV")) 
        {
            int i = va_arg(args, int);
            QString ts;
            ts.sprintf("%02X",i);
            if(!appendNum(ts.toInt(0,16)))
                Q_ASSERT(false /*appendNum failed*/);
        } 
        else if (val == QLatin1String("P14")) 
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
        else if (val == QLatin1String("MSTR"))
        {
            char *strVal = va_arg(args, char *);
            DcMidiData m(strVal);
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
        else if (val == QLatin1String("CSTR"))
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
int DcMidiData::toInt( int offset, int cnt, int defaultValue /*= 0 */ ) const
{
   int val = defaultValue;
   if( offset + cnt > _data.length())
   {
       return val;
   }

   if(1 == cnt )
   {
        val = (int)_data.at(offset);
   }
   else if ( 2 == cnt )
   {
       val = (int)_data.at(offset) << 7;
       val += (int)_data.at(offset+1);
   }
   else if ( 3 == cnt )
   {
       val = (int)_data.at(offset)      << 16;
       val += (int)_data.at(offset + 1) << 8;
       val += (int)_data.at(offset + 2) ;
   }
   else if( 4 == cnt )
   {
       val = (int)_data.at(offset) << 24;
       val += (int)_data.at(offset + 1) << 16;
       val += (int)_data.at(offset + 2) << 8;
       val += (int)_data.at(offset + 3);
   }

   return val;
}

//-------------------------------------------------------------------------
QString DcMidiData::toAsciiString( int offset, int len )
{
    QString rtval;
    if(this->length() >= offset+len)
    {
        QByteArray ba = this->mid(offset,len);

        for (int mdx = 0; mdx < ba.length() ; mdx++)
        {
            char cval = ba[mdx];
            rtval += QLatin1Char(cval);
        }
    }

    return rtval;
}
