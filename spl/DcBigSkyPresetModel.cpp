#include "DcBigSkyPresetModel.h"


DcBigSkyPresetModel::DcBigSkyPresetModel(QObject *parent) :
    DcPresetModel(parent)
{

}

DcBigSkyPresetModel::DcBigSkyPresetModel(const DcMidiData &md,
                                         QObject *parent) :
    DcPresetModel(md,parent)
{

}

DcBigSkyPresetModel::~DcBigSkyPresetModel()
{

}

#define MIDIB(O)  QString("<tr><td align=\"right\">%1:</td><td align=\"left\">%2(%3)</td></td>").arg(O).arg(DcMidiData(_md.at(O)).toString()).arg(_md.at(O))
QString DcBigSkyPresetModel::toString() const
{


    QStringList names;
    names << "BLOOM" << "DUST" << "CHORALE" << "SHIMMER" << "MAG_REVERSE" << "NONLINEAR" <<
             "REFLECTIONS" << "ROOM" << "HALL" << "PLATE" << "SPRING" << "SWELL";
    int m = _md.at(9);
    QString machine = names.at(m);
    machine.append(QString("&nbsp;").repeated(11-machine.length()));

    QString rtval; /*=DcPresetModel::toString() +*/
    rtval += QString("<h3>%1</h3><hr>").arg((QString)DcPresetModel::DataFilters::Name(_md).toByteArray());

    rtval += QLatin1String("<table border=\"0\" cellpadding=\"0\" cellspacing=\"3\">") +
            QString("<tr><td align=\"right\">%1</td><td>%2</td></tr>").arg("NAME:").arg((QString)DcPresetModel::DataFilters::Name(_md).toByteArray()) +
            "<tr><td>MACHINE:</td> <td>"+machine+"</td></tr>" +
            MIDIB(9) +
            MIDIB(10) +
            MIDIB(11) +
            MIDIB(12) +
            MIDIB(13) +
            MIDIB(14) +
            MIDIB(15) +
            MIDIB(16) +
            MIDIB(17) +
            MIDIB(18) +
            MIDIB(19) +
            MIDIB(20) +
            "</table>";

    return rtval;
}

