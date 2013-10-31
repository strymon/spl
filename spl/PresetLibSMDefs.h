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
#ifndef PresetLibSMDefs_h__
#define PresetLibSMDefs_h__

#pragma once
#include "DcStateMachineHelpers.h"


enum DcEventOffset
{
    VerifyDeviceConnectionOffset,
    DetectDeviceOffset,
    FetchCompleteSuccessOffset,
    FetchDataExistsEventOffset,
    WriteCompleteSuccessOffset,
    InPresetEditOffset,
    MidiDataOffset,
    WorkListDirtyOffset,
    WorkListCleanOffset,
    FetchCancledOffset,
    DeviceListInSyncOffset,
    WorkListIsDifferentThanDeviceListOffset,
    WorkListIsSameAsDeviceListOffset,

    DataXfer_ListEmptyOffset,
    DataXfer_ACKOffset,
    DataXfer_NACKOffset,
    DataXfer_TimeoutOffset,
    DataXfer_CancledOffset,

};

typedef DcSimpleEvent<VerifyDeviceConnectionOffset> VerifyDeviceConnection;
typedef DcSimpleEvent<DetectDeviceOffset> DetectDevice;
typedef DcSimpleEvent<InPresetEditOffset> InPresetEditEvent;
typedef DcSimpleEvent<FetchCompleteSuccessOffset> FetchCompleteSuccessEvent;
typedef DcSimpleEvent<FetchDataExistsEventOffset> FetchDataExistsEvent;
typedef DcSimpleEvent<WriteCompleteSuccessOffset> WriteCompleteSuccessEvent;

typedef DcSimpleEvent<MidiDataOffset> BaseMidiDataEvent;
typedef DcSimpleEvent<WorkListDirtyOffset> WorkListDirtyEvent;
typedef DcSimpleEvent<WorkListCleanOffset> WorkListCleanEvent;
typedef DcSimpleEvent<WorkListIsDifferentThanDeviceListOffset> WorkListIsDifferentThanDeviceListEvent;
typedef DcSimpleEvent<WorkListIsSameAsDeviceListOffset> WorkListIsSameAsDeviceListEvent;


typedef DcSimpleEvent<FetchCancledOffset> FetchCancledEvent;

typedef DcSimpleEvent<DeviceListInSyncOffset> DeviceListInSyncEvent;

// Events defined for the DataXfer class
typedef DcSimpleEvent<DataXfer_ListEmptyOffset> DataXfer_ListEmptyEvent;
typedef DcSimpleEvent<DataXfer_NACKOffset> DataXfer_NACKEvent;
typedef DcSimpleEvent<DataXfer_ACKOffset> DataXfer_ACKEvent;
typedef DcSimpleEvent<DataXfer_TimeoutOffset> DataXfer_TimeoutEvent;
typedef DcSimpleEvent<DataXfer_CancledOffset> DataXfer_CancledEvent;


struct MidiDataEvent : public BaseMidiDataEvent
{
    MidiDataEvent(const QRtMidiData &val)
        : value(val) {}

    QRtMidiData value;
};



// Perhaps one way to create a state transition based on the data in a
// QRtMidiData event.
class ContainsMidiDataTransition : public DcCustomTransition
{
public:
    ContainsMidiDataTransition (const char* value,QState* sourceState = 0)
        : DcCustomTransition(MidiDataEvent::TYPE,sourceState)
    {
        _value = value;
    }

protected:

    virtual bool eventTest(QEvent *e)
    {
        if (e->type() != MidiDataEvent::TYPE) 
            return false;
        MidiDataEvent *se = static_cast<MidiDataEvent*>(e);
        return (se->value.contains(_value));
    }

    virtual void onTransition(QEvent *) {}

private:
    QByteArray _value;
};

/* Example 
QState *s1= new QState();
QState *s2= new QState();

MidiDataTransition *isDc = new MidiDataTransition("F0 00 01 55");
isDc->setTargetState(s2);
s1->addTransition(isDc);

QRtMidiData md("F0 00 01 55");
sm.postevent(new MidiDataEvent(md));

*/
#endif // PresetLibSMDefs_h__
