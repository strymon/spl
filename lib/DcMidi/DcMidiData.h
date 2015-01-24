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
#ifndef DcMidiData_h__
#define DcMidiData_h__

#include <QDebug>
#include <QMetaType>
#include <QByteArray>
#include <QDateTime>

#include <vector>


// Example TimeLine Preset Fetch: F0 00 01 55 12 01 63 hi lo F7
// If p is the 14 bit preset id, then:
// p = (hi<<7)&7F + lo&7F
//     and
// hi=(p>>7)&7F and lo=p&7F
#define getHi(P) ((P>>7)&0x7F)
#define getLo(P) (P&0x7F)
#define mk14 (H,L) ((H)<<7)|((L)&7F)


class DcMidiByteRef;
class DcMidiIn;

// Midi data container class that can be used with buffered signal and slots
// That is, signal and slots connecting different threads.
// 
// Note: The meta-type must be registered before the first use of the type.
//       This can be done by calling qRegisterMetaType<DcMidiData>(); in main, 
//       before the first use.
class DcMidiData
{

public:

    DcMidiData();
    
    DcMidiData(const DcMidiData &other);
    
    ~DcMidiData();

    DcMidiData(const char* data);
    
    DcMidiData(const QByteArray& ba);
    
    DcMidiData(const QString& st);
    
    DcMidiData(const qint32 data);

    DcMidiData(const std::vector<unsigned char>& vector,DcMidiIn* source = 0);

    // Append the given integer
    bool appendNum(quint32 x );

    static QByteArray numHexToByteArray( quint32 x);


    // Returns the data as a base16 QString
    QString toString() const;
    QString toString(const char ch) const;

    QString toAsciiString(int offset, int len);

    QList<DcMidiData> split(int maxSz) const;

    // Returns a std::vector
    std::vector<unsigned char> toStdVector(int offset = 0, int len = -1);

    // Returns true if the midi data matches the given string
    bool match(const char* hex_string,bool must_start_with = false) const;
    
    // Returns true if the midi data matches the given ReqExp.
    bool match( QRegExp rx,bool must_start_with = false) const;

    // Return true if the hex string is contained in the MIDI data
    // wild-card values are specified with 'XX'
    // e.g. If the object equals 'F05544F7' match("55XXF7") would
    // return true.
    bool contains(const char* hex_string) const;
    bool contains(const QByteArray&) const;
    bool contains(const DcMidiData& md) const;

    // Same as contains, but the match must begin at the begining of
    // the MIDI data
    // e.g. If the object equals 'F05544F7' match("55XXF7") returns false,
    // but match("F055XX") would return true
    bool startsWith(const char* hex_string) const;
    bool startsWith(const QByteArray&) const;
    bool startsWith(const DcMidiData& md) const;

    // Return the midi byte at idx
    inline unsigned char at(int idx) const;

    // Returns the byte count
    inline int length() const;

    // Return the sum of each byte between start and end
    unsigned char sumOfSection(int start, int len) const;

    // Returns the nth midi byte from the end
    inline unsigned char fromEnd(int nth_from_end);
    
    // Returns true if empty
    inline bool isEmpty() const;

    // Return the sub range specified
    inline DcMidiData left(int len ) const;
    
    const char* data() const { return _data.data();}

    QByteArray mid ( int pos, int len = -1 ) const;

    QByteArray replace  ( int pos, int len, const QByteArray & after );

    // Copies the midi data into the given std::vector
    void copyToStdVec( std::vector<unsigned char>& vec, int offset = 0, int len = -1 ) const;
    
    // Returns the 14 bit MIDI value located at the given offset, if the offset
    // is out of range, the defualtValue is returned
    int get14bit(int offset, int defaultValue = 0 ) const;
    
    // Set the 2 byte 14 bit MIDI value at the given byte offset, 
    // if the offset is out of range, false is returned.
    bool set14bit(int offset, int val);

    // Convert the specified range to an integer.  Each MIDI byte
    // is accumulated as an 8bit value.
    int toInt(int offset, int cnt, int defaultValue = 0 ) const;



/*

    DcMidiData &prepend(char c)
    {
        _data.prepend(c);
        return *this;
    }

    DcMidiData &prepend(const char *s)
    {
        _data.prepend(s);
        return *this;
    }

    DcMidiData &prepend(const QByteArray &a)
    {
        _data.prepend(a);
        return *this;
    }
 */

    DcMidiData &append(char c)
    {
        _data.append(c);
        return *this;
    }

    DcMidiData &append(const QString& s)
    {

        append(DcMidiData(s));
        return *this;
    }

    DcMidiData &append(const char *s)
    {
        
        append(DcMidiData(s));
        return *this;
    }
    
    DcMidiData &append(const DcMidiData &a)
    {
        _data.append(a.toByteArray());
        return *this;
    }


    bool operator==(const DcMidiData &d1) const
    { 
        return d1._data == _data; 
    }

    bool operator!=(const DcMidiData &d1)
    { 
        return d1._data != _data; 
    }


    bool operator==(const QByteArray &a)
    { 
        return a == _data; 
    }

     bool operator==(const char* midistr)
     {
         DcMidiData t = DcMidiData(midistr);
         return _data == t._data;
     }

     //char operator[](int i) const;
     
//      inline char operator[] (int i);
//      inline char operator[] (unsigned int i);
     char operator[](int i) const;
     char operator[](uint i) const;

     DcMidiByteRef operator[](int i);
     DcMidiByteRef operator[](uint i);

//      DcMidiData &operator+=(char c);
//      DcMidiData &operator+=(const char *s);
//      DcMidiData &operator+=(const DcMidiData &a
//      inline DcMidiData &DcMidiData::operator+=(char c)
//      { return append(c); }
// 
//      inline DcMidiData &DcMidiData::operator+=(const char *s)
//      { return append(s); }
     
//      inline DcMidiData &DcMidiData::operator+=(const QByteArray &a)
//      { return append(a); }
//      
//      inline DcMidiData &DcMidiData::operator+=(const DcMidiData &a)
//      { 
//          return a.append(a); 
//      }
// 
// 
// 
//      DcMidiData operator+(const QByteArray &ba)
//       { 
//           _data += ba;
//           return *this;
//       }

//      DcMidiData &operator+=(const DcMidiData &a)
//      {
//          this->append(a);
//          return *this;
//      }
// 
//      DcMidiData &operator+=(const QByteArray &ba)
//      {
//          this->_data.append(ba);
//          return *this;
//      }

//      DcMidiData operator+(const DcMidiData &md)
//      { 
//          _data += md._data;
//          return *this;
//      }
//      
//      DcMidiData operator+(const char* s)
//      { 
//          append(s);
//          return *this;
//      }

     //      inline const DcMidiData operator+(const DcMidiData &a1, const char *a2)
//      { return DcMidiData(a1) += a2; }
//      inline const DcMidiData operator+(const DcMidiData &a1, char a2)
//      { return DcMidiData(a1) += a2; }
//      inline const DcMidiData operator+(const char *a1, const DcMidiData &a2)
//      { return QByteArray(a1) += a2; }
//      inline const DcMidiData operator+(char a1, const DcMidiData &a2)
//      { return DcMidiData(&a1, 1) += a2; }
//
     QByteArray toByteArray() const { return _data; }
     QByteArray toByteArray(const char ch) const
     {
         QString str = toString(ch);
         const std::string stdstr = str.toStdString();
         return QByteArray(stdstr.c_str());
     }

     void clear();
      
     void setData(const char* fmt, ...);
     void setText( const char* data );
     //void setData(const QString& fmt, ...);

     qint64 getTimeStamp() const { return _ts; }

     qint64 setTimeStamp(quint64 ts = -1) 
     {
         qint64 cur = _ts;

         if((quint64)-1 == ts)
         {
             _ts = QDateTime::currentMSecsSinceEpoch();
         }
         else
         {
             _ts = ts;
         }
         
         return cur;
     }
     
     DcMidiIn* getSrcDevice() { return _srcDevice; }
     
     void setSrcDevice(DcMidiIn* val) { _srcDevice = val; }

private:
     friend class DcMidiByteRef;
    void vSetData( const char* fmt, va_list args );
    // Returns true if the string starts with '0x'
    bool is0xHexStr( QString &str );
    
    QByteArray _data;
    qint64 _ts;
    DcMidiIn* _srcDevice;
};



class  DcMidiByteRef 
{
    DcMidiData &a;
    int i;
    inline DcMidiByteRef(DcMidiData &md, int idx)
        : a(md),i(idx) {}

    friend class DcMidiData;

public:



    inline operator char() const
    { 
        return i < a.length() ? a._data.data()[i] : char(0); }


    inline DcMidiByteRef &operator=(char c)
    { 
        a._data.data()[i] = c;  
        return *this; 
    }

    inline DcMidiByteRef &operator=(const DcMidiByteRef &c)
    { 
        a._data.data()[i] = c.a._data.data()[c.i];
        return *this; 
    }

    inline bool operator==(char c) const
    { return a._data.data()[i] == c; }
    inline bool operator!=(char c) const
    { return a._data.data()[i] != c; }
    inline bool operator>(char c) const
    { return a._data.data()[i] > c; }
    inline bool operator>=(char c) const
    { return a._data.data()[i] >= c; }
    inline bool operator<(char c) const
    { return a._data.data()[i] < c; }
    inline bool operator<=(char c) const
    { return a._data.data()[i] <= c; }
};

inline char DcMidiData::operator[](int i) const
{ 
    return data()[i];
}

inline char DcMidiData::operator[](unsigned int i) const
{ 
    return data()[i];
}

inline DcMidiByteRef DcMidiData::operator[](int i)
{ 
    Q_ASSERT(i >= 0); 
    return DcMidiByteRef(*this, i); 
}

inline DcMidiByteRef DcMidiData::operator[](uint i)
{ 
    return DcMidiByteRef(*this, i); 
}


inline const DcMidiData  operator+(const DcMidiData  &md1, const DcMidiData  &md2)
{ 
    return DcMidiData(md1.toByteArray() + md2.toByteArray()); 
}

inline const DcMidiData  operator+(const DcMidiData  &a1, const char* c)
{ 
    return DcMidiData(a1 + DcMidiData(c)); 
}

inline const DcMidiData  operator+(const DcMidiData  &a1, const QByteArray  &a2)
{ 
    return DcMidiData(DcMidiData(a1.toByteArray() + a2)); 
}
//-------------------------------------------------------------------------
unsigned char DcMidiData::fromEnd( int n)
{
    // Return the byte n from the end
    if(length() > n && n >= 0)
    {
        return _data.at(length()-n);
    }
    else
    {
        // TODO: throw an error
        return 0xFF;
    }
}

//-------------------------------------------------------------------------
unsigned char DcMidiData::at( int idx ) const
{
    return _data.at(idx);
}

//-------------------------------------------------------------------------
int DcMidiData::length() const
{
    return _data.size();
}

//-------------------------------------------------------------------------
bool DcMidiData::isEmpty() const
{
    return _data.isEmpty();
}


typedef QList<DcMidiData> DcMidiDataList_t;



Q_DECLARE_METATYPE(DcMidiData);

#endif // DcMidiData_h__

