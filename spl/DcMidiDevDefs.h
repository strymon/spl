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

#include <QHash>
#include <QByteArray>

/* Identity Request Response
    0xF0  SysEx
    0x7E  Non-real time
    0x7F  The SysEx channel. Could be from 0x00 to 0x7F.  7F indicates "disregard channel".
    0x06  Sub-ID -- General Information
    0x02  Sub-ID2 -- Identity Reply
    0xID  Manufacturer's ID
    0xf1  The f1 and f2 bytes make up the family code. Each
    0xf2  manufacturer assigns different family codes to his products.
    0xp1  The p1 and p2 bytes make up the model number. Each
    0xp2  manufacturer assigns different model numbers to his products.
    0xv1  The v1, v2, v3 and v4 bytes make up the version number.
    0xv2
    0xv3
    0xv4
    0xF7  End of SysEx
*/

// Strymon Pedal Specific

// Strymon TimeLine Commands:

class DcMidiDevDefs
{
public:

    DcMidiDevDefs();

    // Strymon Pedal Specific
    static const char* kStrymonDevice;
    static const char* kTimeLineIdent;
    static const char* kMobiusIdent;
    static const char* kBigSkyIdent;
    static const char* kTestDevice;
    static const char* kStatusMsg; /* = "F0 00 01 55 12 01 22 F7"; */
    static const char* kIdentReply;
};

// Some important Sysex Offset
#define MIDI_SYSEX_PATCH_OFFSET		7
#define MIDI_PATCH_DATA_OFFSET		9		

//static const char* kWritePresetOpcodeStr = "62";
//static const unsigned char kWritePresetOpcode = 0x62;
//static const int   kPresetNumberByteOffset = 7;

#define MIDI_WRITE_SINGLE_PATCH				0x62
#define MIDI_PATCH_WRITE_SUCCESS			0x45
#define MIDI_PATCH_WRITE_FAIL				0x46
#define MIDI_WRITE_SINGLE_FACTORY_PATCH		0x65

#define MIDI_ESN_WRITE_SUCCESS				0x45	
#define MIDI_ESN_WRITE_FAIL					0x46	

#define MIDI_REQUEST_SINGLE_PATCH			0x63  

#define MIDI_PATCH_REQUEST_FAIL				0x47

#define MIDI_ENTER_SAVE_MODE				0x64
#define MIDI_DEST_BANK_NAME					0x66
#define MIDI_ERASE_COMPLETE                	0x70
#define MIDI_FLASH_PROGRAM_COMPLETE        	0x71
#define MIDI_FLASH_PROGRAM_ERROR           	0x72
#define MIDI_FLASH_CHECKSUM_ERROR          	0x73
#define MIDI_FLASH_TRANSMITION_ERROR       	0x74
#define MIDI_FLASH_CHECKSUM_GOOD	      	0x75
#define MIDI_FLASH_REQUEST_PACKET			0x7A
#define MIDI_FLASH_PROGRAM_READY			0x7B
#define MIDI_PRIVATE_RESET					0x1B
#define MIDI_BOOT_OPT						0x1C
#define MIDI_REPLACE_FACTORY_FROM_RAM		0x1D
#define MIDI_REINIT_PRESETS					0x1F
#define MIDI_WRITE_ESN						0x20
#define MIDI_READ_ESN						0x21
#define MIDI_GET_CURRENT_STATE				0x22


//-------------------------------------------------------------------------
// Strymon Example MIDI Data
    
// TimeLine - Identity Response:
/*
static const char* kTimeLineIdentResponceExample = "\
    F0 7E 00 06 02 00 01 55 12 00 01 00 3B 31 32 34 F7";

//    Example Mobius Preset Data:
    F0 00 01 55 12 02 62 00 00 07 20 3F 7B 01 7F 2C \
    40 4E 56 29 0B 0C 0D 0E 0F 10 01 11 0A 02 00 00 \
    01 02 00 01 02 01 01 01 01 01 00 07 49 01 00 7F \
    3F 3F 3F 3F 3F 3F 3F 3F 3F 3F 3F 3F 3F 3F 12 13 \
    14 15 16 17 18 01 02 01 02 01 00 04 23 49 27 0D \
    0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19 1A 1B \
    1C 1D 1E 1F 20 21 22 23 24 25 26 27 28 29 2A 2B \
    2C 2D 2E 2F 30 31 32 33 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F 7F \
    7F 7F 7F 7F 7F 7F 7F 02 43 41 4C 4C 20 54 48 45 \
    20 43 4F 50 53 20 20 20 55 F7";
*/

static const int kPresetNameOffset = 632;
static const int kPresetNameLen    = 16;
static const int kPreseLength      = 650;
static const int kPresetDataOffset  = 9;
static const int kPresetProductIdOffset = 5;
static const int kPresetOpcodeOffset = 6;
static const int kPresetLocationOffset = 7;
static const int kPresetDataLength  =  650-9-2;
static const int kPresetChecksumOffset =  648;
static const int kPreseChecksumOffset = kPreseLength-2;


