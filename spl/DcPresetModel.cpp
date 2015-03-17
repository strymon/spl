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
    QString rtval = QString("<h3>%1</h3><hr>").arg((QString)DcPresetModel::DataFilters::Name(_md).toByteArray());
    rtval += "ID Hdr:" + DcPresetModel::DataFilters::Identify(_md).toString(' ') + "<br>";
    rtval += "Product: " + DcPresetModel::DataFilters::ProductId(_md).toString(' ') + "<br>";
    rtval += "Opcode: " + DcPresetModel::DataFilters::Opcode(_md).toString(' ') + "<br>";
    rtval += "Location: " + DcPresetModel::DataFilters::Location(_md).toString(' ') + "<br>";
    rtval += "Name: " + (QString)DcPresetModel::DataFilters::Name(_md).toByteArray() + "<br>";
    rtval += "Chk Byte: " + DcPresetModel::DataFilters::Checkbyte(_md).toString(' ') + "<br>";
    rtval += "<br>";

    int starto = 84;
    int endo = 18;
    int sz = _md.length();
    DcMidiData md = _md.mid(0,starto) + _md.mid(sz-endo,endo);
    rtval += md.toString();
    rtval += "</p>";
    return rtval;
}

DcMidiData DcPresetModel::DataFilters::Data(const DcMidiData &md)
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

DcMidiData DcPresetModel::DataFilters::AltData(const DcMidiData &md)
{
    DcMidiData rtval = md.mid(kPresetDataOffset,83);
    int len = kPresetDataLength-(538+83) - 2;
    rtval.append(DcMidiData(md.mid(538+83,len)));
    return rtval;
}

DcMidiData DcPresetModel::DataFilters::Location(const DcMidiData &md)
{
    return DcMidiData(md.mid(kPresetLocationOffset,2));
}

DcMidiData DcPresetModel::DataFilters::ProductId(const DcMidiData &md)
{
    return DcMidiData(md.mid(kPresetProductIdOffset,1));
}

DcMidiData DcPresetModel::DataFilters::Identify(const DcMidiData &md)
{
    return DcMidiData(md.mid(1,5));
}

DcMidiData DcPresetModel::DataFilters::Opcode(const DcMidiData &md)
{
    return DcMidiData(md.mid(kPresetOpcodeOffset,1));
}

DcMidiData DcPresetModel::DataFilters::Checkbyte(const DcMidiData &md)
{
    return DcMidiData(md.mid(kPreseChecksumOffset,1));
}

DcMidiData DcPresetModel::DataFilters::Name(const DcMidiData &md)
{
    return DcMidiData(md.mid(kPresetNameOffset,kPresetNameLen));
}



