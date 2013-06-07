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
#include "QRtMidiIdent.h"

//-------------------------------------------------------------------------
QRtMidiDevIdent::QRtMidiDevIdent()
{
    clear();
}

//-------------------------------------------------------------------------
QRtMidiDevIdent::QRtMidiDevIdent(const QRtMidiData &data )
{
    clear();
    fromIdentData(data);

}

//-------------------------------------------------------------------------
void QRtMidiDevIdent::clear()
{
    Manufacture.clear();
    FwVersion.clear();
    SyxCh = 0;
    Family.clear();
    Product.clear();
    ShortHdr.clear();
    MfjIdSize = 1;
}

//-------------------------------------------------------------------------
bool QRtMidiDevIdent::isEmpty()
{
    return Manufacture.length() == 0;
}

//-------------------------------------------------------------------------
QString QRtMidiDevIdent::getManufactureName()
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
void QRtMidiDevIdent::fromIdentData( const QRtMidiData & data )
{
    int len = data.length();

    // Parse the MIDI identity data
    if(data.contains(kIdentReply) && len >= 13)
    {
        // If the Manufacturer's Id is the optional 3 bytes, then adjust the 
        // byte position of the identity data by the extra two bytes.
        int adj = (data.at(5) == 0) ? 2 : 0;
        MfjIdSize = adj+1;

        if(len >= 13+adj)
        {
            // See the comment above to understand the byte positions of this data
            Manufacture = data.mid(5,MfjIdSize);

            SyxCh       = data.at(2);
            Family      = data.mid(6+adj,2);
            Product     = data.mid(8+adj,2);
            ShortHdr    = Manufacture + Family.mid(0,1) + Product.mid(0,1);
            FwVersion.sprintf("%c.%c.%c.%c",data.at(10+adj),data.at(11+adj),data.at(12+adj),data.at(13+adj));
        }
    }
}
