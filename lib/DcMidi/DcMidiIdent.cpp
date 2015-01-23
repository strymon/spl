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
#include "DcMidiIdent.h"

//-------------------------------------------------------------------------
DcMidiDevIdent::DcMidiDevIdent()
{
    clear();
}

//-------------------------------------------------------------------------
DcMidiDevIdent::DcMidiDevIdent(const DcMidiData &data )
{
    clear();
    fromIdentData(data);

}

//-------------------------------------------------------------------------
void DcMidiDevIdent::clear()
{
    Manufacture.clear();
    FwVersion.clear();
    FwVerInt = 0;
    SyxCh = 0;
    Family.clear();
    Product.clear();
    SOXHdr.clear();
    MfjIdSize = 1;
}

//-------------------------------------------------------------------------
QString DcMidiDevIdent::toString()
{
    QString str;
    if(Manufacture.toString() == "000155"
        && (Product.toString() == "0200") 
        && (Family.toString() == "0200"))
    {
        str = "Strymon BC, v" + FwVersion;
    }
    else
    {
        str = QLatin1String("Identity: ");
        str += Family.toString() + QLatin1Char(' ');
        str += Product.toString() + QLatin1Char(' ');
        str += Manufacture.toString() + QLatin1String(" (") + getManufactureName() + QLatin1String(") ");
        str += FwVersion;
    }
    
    return str;
}

//-------------------------------------------------------------------------
bool DcMidiDevIdent::isEmpty()
{
    return Manufacture.length() == 0;
}

//-------------------------------------------------------------------------
QString DcMidiDevIdent::getManufactureName()
{
    QString n = "unknown";

    if(!isEmpty())
    {
        int id = Manufacture.toInt(0,MfjIdSize,-1);
        if(id > -1)
        {
            n = MidiNameToIds[id];
            if(n.isEmpty())
            {
                n.sprintf("unknown %0X",id);
            }
        }
    }

    return n;
}

//-------------------------------------------------------------------------
// TODO: I should have just stored the raw id response data and had the 
// accessors parse the data each time it's needed.
void DcMidiDevIdent::fromIdentData( const DcMidiData & data )
{
    int len = data.length();

    // Parse the MIDI identity data
    if(data.contains("F0 7E XX 06 02") && len >= 13)
    {
        // If the Manufacturer's Id is the optional 3 bytes, then adjust the 
        // byte position of the identity data by the extra two bytes.
        int adj = (data.at(5) == 0) ? 2 : 0;
        MfjIdSize = adj+1;

        if(len >= 13+adj)
        {
            // See the comment above to understand the byte positions of this data
            Manufacture = data.mid(5,MfjIdSize); // e.g. Strymon 3 byte manufacture ID: 000155
            SyxCh       = data.at(2);  // e.g. if it's the universal Ch, it would equal 7E
            Family      = data.mid(6+adj,2);
            Product     = data.mid(8+adj,2);
            
            // The short hdr (my definition, not in spec) is the Manufacture id and one byte of 
            // the family and the product.
            SOXHdr    = DcMidiData("F0") + Manufacture + Family.mid(0,1) + Product.mid(0,1);
            
            FwVersion.sprintf("%c.%c.%c.%c",data.at(10+adj),data.at(11+adj),data.at(12+adj),data.at(13+adj));
            FwVerInt = (data.at(10+adj)<<24) + (data.at(11+adj)<<16)+(data.at(12+adj)<<8)+(data.at(13+adj));
        }
        ResponceData = data;
    }
}

//-------------------------------------------------------------------------
quint16 DcMidiDevIdent::getProductId()
{
    int rtval = 0;
    if(!Product.isEmpty())
    {
        rtval = Product.toInt(0,1,0);
        rtval += (Product.toInt(1,1,0)<<8);
    }

    return rtval;
}

//-------------------------------------------------------------------------
quint8 DcMidiDevIdent::getProductByte()
{
    return (quint8)getProductId()&0xFF;
}

//-------------------------------------------------------------------------
unsigned short DcMidiDevIdent::getFamilyId()
{
    int rtval = 0;
    if(!Family.isEmpty())
    {
        rtval = Family.toInt(0,1,0);
        rtval += (Family.toInt(1,1,0)<<8);
    }

    return rtval;
}

//-------------------------------------------------------------------------
quint8 DcMidiDevIdent::getFamilyByte()
{
    return (quint8)getFamilyId()&0xFF;
}

//-------------------------------------------------------------------------
QString DcMidiDevIdent::getShortId()
{
    QString rtval;
    rtval.sprintf("%02X%02X",getFamilyByte(),getProductByte());
    return rtval;
}
