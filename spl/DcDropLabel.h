#ifndef DCDROPLABEL_H
#define DCDROPLABEL_H

#include <QLabel>

class DcDropLabel : public QLabel
{
    Q_OBJECT
  public:
      explicit DcDropLabel(QWidget *parent = 0);
    ~DcDropLabel();

    // QWidget interface
protected:
    void dragEnterEvent(QDragEnterEvent *);
    void dropEvent(QDropEvent *);
};

#endif // DCDROPLABEL_H
