#pragma once

#include <QMutex>
#include <QRegExp>
#include <QString>
#include <QMutexLocker>
#include <QQueue>
#include <QWaitCondition>
#include <QByteArray>

#include "DcMidiData.h"
#include "DcMidiIn.h"

class DcSioMidi;

class DcSignal
{
public:

    DcSignal() : _reciver(0) {};
    DcSignal(const QObject* recv, const char* method, void* user = 0);
    void invoke();

private:
    const QObject* _reciver;
    QByteArray _methodName;
    void* _usrParam;

};

// const_cast<QObject *>(


class DcMidiTrigger
{
    
    static const int kMaxQueueSize = 200;

public:
    
    DcMidiTrigger() : _waitting(false),_count(0),_allowFutherProcessing(true),_signal(0) {} ;

    DcMidiTrigger(QString pattern);
    
    DcMidiTrigger(DcMidiData& pattern);

    DcMidiTrigger(QString pattern,const QObject *receiver, const char *member);

    void reset();

    ~DcMidiTrigger() 
    {
        if(_signal)
            delete _signal;
    }

    inline QRegExp getRegExp() const { return _regex; }
    
    void setRegExp(QRegExp val) { _regex = val; }
    
    void setRegExp(QString pattern);

    bool dequeue(DcMidiData& md);

    bool allowFutherProcessing() const { return _allowFutherProcessing; }
    
    void setAllowFutherProcessing(bool val) { _allowFutherProcessing = val; }

    bool wait(unsigned long time = ULONG_MAX);

    void lock() {_mtx.lock();}
    
    void unlock() {_mtx.unlock();}

    quint32 getCount() { return _count; }
    quint32 clearCount() { return _count; }

private:
    friend class DcMidiIn;
    friend class DcSioMidi;

    bool handler(DcMidiData& md);

    QRegExp               _regex;
    QWaitCondition        _wc;
    
    QQueue<DcMidiData>  _queue;
    

    QMutex _mtx;
    QMutex _lockMtx;
    bool _waitting;
    int   _count;
    bool _allowFutherProcessing;
    DcSignal* _signal;
};

/*!
    The DcAutoTrigger is a convenience class that simplifies adding
    and removing "trigger channels" to and from a Midi input object.

    DcAudoTrigger should be created within a function where a
    temporary DcTriggerChannel is needed to detect some response
    from a midi device.  Once the DcAutoTrigger goes out of scope
    it will remove the trigger channel from the DcMidiIn device.
!*/

class DcAutoTrigger
{

public:

    inline DcAutoTrigger(DcMidiTrigger* tc, DcMidiIn* dev)
    {
        Q_ASSERT(dev);
        Q_ASSERT(tc);
        _tc = tc;
        _dev = dev;
        dev->addTrigger(*tc);
        deleteTc = false;
    }

    inline DcAutoTrigger(QString pattern, DcMidiIn* dev)
    {
        Q_ASSERT(dev);
        _tc = new DcMidiTrigger(pattern);
        if(_tc)
        {
            deleteTc = true;
            _dev = dev;
            dev->addTrigger(*_tc);
        }
        // TODO: throw on allocation error
    }

    // A method to change the current trigger pattern and reset the
    // trigger internals
    inline void setPattern(QString pattern)
    {
        if(_tc)
        {
            _tc->setRegExp(pattern);
            _tc->reset();
        }
    }

    inline bool dequeue(DcMidiData& md) {return _tc->dequeue(md);}
    inline bool wait(unsigned int timems) {return _tc->wait(timems);}
    inline quint32 getCount() {return _tc->getCount();}
    inline void clearCount() {_tc->clearCount();}

    inline ~DcAutoTrigger()
    {
        _dev->removeTrigger(*_tc);

        if(deleteTc && _tc)
        {
            delete _tc;
        }
    };
    
    DcMidiIn* getDevice() const { return _dev; }

private:

    DcMidiIn* _dev;
    
    
    DcMidiTrigger* _tc;
    bool deleteTc;

};

