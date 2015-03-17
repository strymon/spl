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

