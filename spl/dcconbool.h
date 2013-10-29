#ifndef DCCONBOOL_H
#define DCCONBOOL_H

#include <QObject>

class DcConBool : public QObject
{
    Q_OBJECT
public:
    explicit DcConBool(QObject *parent = 0);
    
    bool isEnabled() { return _enabled; }
signals:
    
public slots:
    void enabled(bool e);
    void toggle();

private:
    bool _enabled;


    
};

#endif // DCCONBOOL_H
