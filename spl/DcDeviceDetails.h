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

struct DcDeviceDetails
{
    DcDeviceDetails() 
    {
        clear();
    };
    
    void clear()
    {
        Manufacture.clear();
        PresetHdr.clear();
        DataInNACK.clear();
        ReadPresetTemplate.clear();
        DevSOX.clear();
        Name = "Unknown";
        PresetsPerBank = 2;
    }
    bool isEmpty()
    {
        return DevSOX.length() == 0;
    }

    QString getUid()
    {
        return Name + FwVersion;
    }

    quint8 PresetDataByteOffset;
    quint16 PresetSize;
    quint16 PresetCount;
    quint16 BankCount;
    QRtMidiData Manufacture;
    QRtMidiData PresetHdr;
    QRtMidiData DataInNACK;
    QRtMidiData DevSOX;
    QString Name;
    QString FwVersion;
    
    QByteArray ReadPresetTemplate;
    unsigned char SyxCh;
    QRtMidiData Family;
    QRtMidiData Product;
    QRtMidiData ShortHdr;
    int PresetsPerBank;
};


