#include "dcconbool.h"

DcConBool::DcConBool(QObject *parent) :
    QObject(parent)
{
    enabled(false);
}

void DcConBool::enabled(bool e)
{
    _enabled = e;
}

void DcConBool::toggle()
{
    _enabled = !_enabled;
}
