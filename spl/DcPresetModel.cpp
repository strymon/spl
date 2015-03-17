#include "DcPresetModel.h"
#include "DcMidiDevDefs.h"

DcPresetModel::DcPresetModel(QObject *parent) :
    QObject(parent)
{

}

DcPresetModel::DcPresetModel(const DcMidiData &md, QObject *parent) :
    QObject(parent),
    _md(md)

{

}

DcPresetModel::~DcPresetModel()
{

}

QString DcPresetModel::toString() const
{
    QString rtval = QString("<h3>%1</h3><hr>").arg((QString)DcPresetModel::DcPresetDataFilters::Name(_md).toByteArray());
    rtval += "ID Hdr:" + DcPresetModel::DcPresetDataFilters::Identify(_md).toString(' ') + "<br>";
    rtval += "Product: " + DcPresetModel::DcPresetDataFilters::ProductId(_md).toString(' ') + "<br>";
    rtval += "Opcode: " + DcPresetModel::DcPresetDataFilters::Opcode(_md).toString(' ') + "<br>";
    rtval += "Location: " + DcPresetModel::DcPresetDataFilters::Location(_md).toString(' ') + "<br>";
    rtval += "Name: " + (QString)DcPresetModel::DcPresetDataFilters::Name(_md).toByteArray() + "<br>";
    rtval += "Chk Byte: " + DcPresetModel::DcPresetDataFilters::Checkbyte(_md).toString(' ') + "<br>";
    rtval += "<br>";
    rtval += _md.toString();
    rtval += "</p>";
    return rtval;
}

DcMidiData DcPresetModel::DcPresetDataFilters::Data(const DcMidiData &md)
{
    /*
     let u8  := unsigned 8 bit number
         m7  := midi 7 bit number
         m15 := midi 14 bit number := [m7,m7]
      preset = {        sox : u8,
                manufacture : [m7,m7,m7],
                family      : m7,
                product_id  : m7,
                location    : u14 = [m7,m7],
                data        : [m7,...] (length = kPresetDataLength
                check_byte  : m7,
                eox         : u8
                }

     */

    return DcMidiData(md.mid(kPresetDataOffset,kPresetDataLength));
}

DcMidiData DcPresetModel::DcPresetDataFilters::AltData(const DcMidiData &md)
{
    DcMidiData rtval = md.mid(kPresetDataOffset,83);
    int len = kPresetDataLength-(538+83) - 2;
    rtval.append(DcMidiData(md.mid(538+83,len)));
    return rtval;
}

DcMidiData DcPresetModel::DcPresetDataFilters::Location(const DcMidiData &md)
{
    return DcMidiData(md.mid(kPresetLocationOffset,2));
}

DcMidiData DcPresetModel::DcPresetDataFilters::ProductId(const DcMidiData &md)
{
    return DcMidiData(md.mid(kPresetProductIdOffset,1));
}

DcMidiData DcPresetModel::DcPresetDataFilters::Identify(const DcMidiData &md)
{
    return DcMidiData(md.mid(1,5));
}

DcMidiData DcPresetModel::DcPresetDataFilters::Opcode(const DcMidiData &md)
{
    return DcMidiData(md.mid(kPresetOpcodeOffset,1));
}

DcMidiData DcPresetModel::DcPresetDataFilters::Checkbyte(const DcMidiData &md)
{
    return DcMidiData(md.mid(kPreseChecksumOffset,1));
}

DcMidiData DcPresetModel::DcPresetDataFilters::Name(const DcMidiData &md)
{
    return DcMidiData(md.mid(kPresetNameOffset,kPresetNameLen));
}



