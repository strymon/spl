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
#include "QRtMidi\QRtMidiIdent.h"

struct DcDeviceDetails : public QRtMidiDevIdent
{
    DcDeviceDetails() 
    {
        clear();
    };
    
    void clear()
    {
        QRtMidiDevIdent::clear();
        PresetHdr.clear();
        DataInNACK.clear();
        DevSOX.clear();
        PresetDataByteOffset = 0;
        PresetSize = 0;
        PresetCount = 0;
        BankCount = 0;
        PresetsPerBank = 0;
    }

    bool isEmpty()
    {
        return QRtMidiDevIdent::isEmpty();
    }

    QString getUid()
    {
        // TODO: this is not very "uuid"
        return Name + QRtMidiDevIdent::FwVersion;
    }

    quint8 PresetDataByteOffset;
    quint16 PresetSize;
    quint16 PresetCount;
    quint16 BankCount;
    int     PresetsPerBank;
    
    QRtMidiData DevSOX;
    QRtMidiData PresetHdr;
    QRtMidiData DataInNACK;
    QString     Name;
    QByteArray ReadPresetTemplate;


};


