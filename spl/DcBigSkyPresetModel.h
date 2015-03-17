#ifndef DCBIGSKYPRESETMODEL_H
#define DCBIGSKYPRESETMODEL_H

#include "DcPresetModel.h"



class DcBigSkyPresetModel : public DcPresetModel
{
    Q_OBJECT
public:
    explicit DcBigSkyPresetModel(QObject *parent = 0);
    DcBigSkyPresetModel(const DcMidiData& md, QObject *parent = 0);
    ~DcBigSkyPresetModel();
    bool isValid() const { return true; }

signals:

public slots:

};

#endif // DCBIGSKYPRESETMODEL_H
