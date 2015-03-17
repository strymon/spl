#ifndef DCPRESETMODEL_H
#define DCPRESETMODEL_H

#include <QObject>

#include <DcMidi/DcMidiData.h>

class DcPresetModel : public QObject
{
    Q_OBJECT
public:
    explicit DcPresetModel(QObject *parent = 0);
    DcPresetModel(const DcMidiData& md, QObject *parent = 0);
    ~DcPresetModel();
    bool isValid() const { return true; }
    virtual QString toString() const;

    class DataFilters
    {
    public:
        static DcMidiData Location(const DcMidiData& md);
        static DcMidiData Identify(const DcMidiData& md);
        static DcMidiData Opcode(const DcMidiData& md);
        static DcMidiData ProductId(const DcMidiData& md);
        static DcMidiData Data(const DcMidiData& md);
        static DcMidiData AltData(const DcMidiData& md);
        static DcMidiData Checkbyte(const DcMidiData& md);
        static DcMidiData Name(const DcMidiData& md);
    };
signals:

public slots:

protected:
    DcMidiData _md;
private:


};


#endif // DCPRESETMODEL_H
