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
#include "QRtMidi/QRtMidiIdent.h"

struct DcDeviceDetails : public QRtMidiDevIdent
{
    DcDeviceDetails() 
    {
        clear();
    };
    
    void clear()
    {
        QRtMidiDevIdent::clear();
        PresetWriteHdr.clear();
        FactoryPresetWriteHdr.clear();
        PresetRdResponce_NACK.setPattern("");
        PresetWrResponce_NACK.setPattern("");
        PresetWrResponce_ACK.setPattern("");

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

    quint8  PresetNumberOffset;
    quint16 PresetSize;
    quint16 PresetCount;
    quint16 PresetStartOfDataOffset;
    quint16 PresetsPerBank;
    quint16 PresetDataLength;
    quint16 PresetChkSumOffset;
    quint16 PresetNameLen;
    quint16 PresetNameOffset;
    
    
    QRtMidiData PresetWriteHdr;
    QRtMidiData FactoryPresetWriteHdr;
    QRegExp     PresetRdResponce_NACK;
    QRegExp     PresetWrResponce_NACK;
    QRegExp     PresetWrResponce_ACK;

    QString     Name;
    QByteArray PresetReadTemplate;
    QString    DeviceIconResPath;
    


};


