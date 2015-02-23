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
#include "DcMidi/DcMidiIdent.h"
#include <qglobal.h>

struct DcDeviceDetails : public DcMidiDevIdent
{
    DcDeviceDetails() 
    {
        clear();
    };
    
    void clear()
    {
        DcMidiDevIdent::clear();
        PresetWriteHdr.clear();
        FactoryPresetWriteHdr.clear();
        PresetRdResponce_NACK.setPattern("");
        PresetWr_NAK.setPattern("");
        PresetWr_ACK.setPattern("");

        PresetNumberOffset = 0;
        PresetSize = 0;
        PresetCount = 0;
        PresetsPerBank = 0;
        PresetStartOfDataOffset = 0;
        PresetsPerBank = 0;
        PresetDataLength = 0;
        PresetChkSumOffset = 0;
        PresetNameLen = 0;
        PresetNameOffset = 0;
        DeviceIconResPath.clear();
        CrippledIo = false;
    }

    bool isEmpty()
    {
        return DcMidiDevIdent::isEmpty();
    }

    unsigned int getUid()
    {
        return qHash(Name + 
            DcMidiDevIdent::FwVersion  + 
            DcMidiDevIdent::Family.toString() + 
            DcMidiDevIdent::Product.toString() + 
            DcMidiDevIdent::Manufacture.toString());
    }
    quint8  PresetNumberOffset;
    quint16 PresetSize;
    quint16 PresetCount;
    quint16 PresetStartOfDataOffset;
    quint16 PresetsPerBank;
    quint16 PresetDataLength;
    quint16 PresetChkSumOffset;
    quint16 PresetNameLen;
    quint16 PresetNameOffset;
    
    
    DcMidiData PresetWriteHdr;
    DcMidiData FactoryPresetWriteHdr;
    QRegExp     PresetRdResponce_NACK;
    QRegExp     PresetWr_NAK;
    QRegExp     PresetWr_ACK;

    QString     Name;
    QByteArray PresetReadTemplate;
    QString    DeviceIconResPath;

    QRegExp PresetRd_NAK;
    QRegExp PresetRd_ACK;
    bool     CrippledIo;


};


