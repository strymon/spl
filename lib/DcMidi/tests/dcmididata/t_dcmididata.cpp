#include <QtTest>

#include "DcMidiData.h"

class t_DcMidiData: public QObject
{
    Q_OBJECT

private slots:

    void constructWithConstChar()
    {
        DcMidiData mdt1("F0F67FF7");
        DcMidiData mdt2("F0 F6 7F F7");
        QVERIFY(mdt1 == mdt2);

        QVERIFY(mdt2 == "F0F67FF7");
        QVERIFY(mdt2 != "F0F67F");
        QVERIFY(mdt1 == "F0 F6 7F F7");
        QVERIFY(mdt1 == "0xF0 0xF6 0x7F 0xF7");
    }

    void setData()
    {
        static const char* tdata = "0xF01233";

        DcMidiData md;

        md.setData("F0 vv vv F7",0xA,0x12);
        QVERIFY(md == "F0 0A 12 F7");

        md.setData("F0");
        QVERIFY(md == "F0");

        md.setData("vv A5 vv 45 vv",0xF0,0x22,0x66);
        QVERIFY(md == "F0 A5 22 45 66");
    }

    void setData14Bit()
    {
        DcMidiData md;
        md.setData("F0 p14 F7",0x80);
        QVERIFY(md == "F0 01 00 F7");

        md.setData("F0 p14 F7",0x81);
        QVERIFY(md == "F0 01 01 F7");

    }

    void setData14Bit7Bit()
    {
        DcMidiData md;
        md.setData("F0 p14 vv F7",0x80,0x12);
        QVERIFY(md == "F0 01 00 12 F7");

        md.setData("F0 vv p14 p14",0x2,0xA5,0x3FFF);
        QVERIFY(md == "F0 02 01 25 7F 7F");
        QCOMPARE(md.toString(' '),QString("F0 02 01 25 7F 7F"));

        md.setData("p14",0x3FFF);
        QVERIFY(md == "7F 7F");
    }

    void setDataWithMidiString()
    {
      DcMidiData md;
      md.setData("F0 mstr F7","7F 55 02");
      QVERIFY(md == "F0 7F 55 02 F7");

      md.setData("mstr F7","F0 7F 55 02");
      QVERIFY(md == "F0 7F 55 02 F7");

      md.setData("F0 7F mstr","55 02 F7");
      QVERIFY(md == "F0 7F 55 02 F7");
    }

    void setDataWithCString()
    {
        // "F0 12 str F7","Hello" => F0 12 48 65 6C 6C 6F F7
        DcMidiData md;
        md.setData("F0 12 cstr F7","Hello");
        QCOMPARE(md.toString(' '),QString("F0 12 48 65 6C 6C 6F F7"));

        md.setData("F0 12 cstr 00 00 cstr F7","Hello","ABCD");
        QCOMPARE(md.toString(' '),QString("F0 12 48 65 6C 6C 6F 00 00 41 42 43 44 F7"));
    }

    void conversion()
    {
      DcMidiData md;
      md.setText("F000010203F7");
      QCOMPARE(md.toString(' '),QString("F0 00 01 02 03 F7"));
      QCOMPARE(md.toString(),QString("F000010203F7"));
      QCOMPARE(md.toString('-'),QString("F0-00-01-02-03-F7"));

      md.setText("F0 00 01 02 03 F7");
      QCOMPARE(md.toInt(1,2),1);
      QCOMPARE(md.toInt(2,2),0x80+2);

      md.setText("414243");
      QByteArray ba = md.toByteArray();
      QCOMPARE(QString(ba.data()),QString("ABC"));

    }

    void set14param()
    {
        DcMidiData md("F0 01 02 03 04 F7");
        int val = 0x81;

        md.set14bit(3,val);
        QCOMPARE(md.toString(' ') , QString("F0 01 02 01 01 F7"));

        int tval = md.get14bit(3,0);
        QCOMPARE(val,tval);
    }

    void numHexToByteArrayTest()
    {
        QByteArray ba = DcMidiData::numHexToByteArray(0x41424344);
        QCOMPARE(QString(ba.data()),QString("ABCD"));
    }


    void containsTest()
    {
        DcMidiData md;
        md.setText("F0 33 12 F7");
        checkContainsHelper(md,true);
        md.setText("F0 33 12 F7 00");
        checkContainsHelper(md,true);
        md.setText("22 F0 33 12 F7 00");
        checkContainsHelper(md,true);

        md.setText("F5 F4 C0 B0 01 22");
        checkContainsHelper(md,false);

        md.setText("C0");
        QVERIFY(md.contains("C0"));
        QVERIFY(!md.contains("F4"));
    }

    void startsWithTest()
    {
        DcMidiData md;
        md.setText("F0 33 12 F7");
        QVERIFY(md.startsWith("F0 33 12 F7"));
        QVERIFY(!md.startsWith("33 12 F7"));
        QVERIFY(!md.startsWith("12 F7"));
        QVERIFY(!md.startsWith("F7"));
        QVERIFY(md.startsWith("F0 33 12 XX"));
        QVERIFY(md.startsWith("F0 33 XX XX"));
        QVERIFY(md.startsWith("F0 XX XX F7"));
        QVERIFY(!md.startsWith("33 12 XX"));
        QVERIFY(md.startsWith("F0 XX 12 F7"));
        QVERIFY(md.startsWith("F0 33 XX F7"));
        QVERIFY(md.startsWith("F0 33 12 XX"));
        QVERIFY(md.startsWith("F0"));
        QVERIFY(md.startsWith("F0 33"));
        QVERIFY(md.startsWith("F0 33 12"));
        QVERIFY(!md.startsWith("12 F7"));
        QVERIFY(!md.startsWith("F7"));
    }

    void lengthTest()
    {
        DcMidiData md("F0 01 02 F7");
        QCOMPARE(md.length(),4);
    }

    void testSplit()
    {
        DcMidiData md("F0 70 71 72 73 74 75 76 F7");

        QList<DcMidiData> lst = md.split(2);
        QCOMPARE(lst.length(), 5);

        lst = md.split(3);
        QCOMPARE(lst.length(), 3);

        lst = md.split(4);
        QCOMPARE(lst.length(), 3);

        lst = md.split(5);
        QCOMPARE(lst.length(), 2);
        QCOMPARE(lst.at(0).length(), 5);
        QCOMPARE(lst.at(1).length(), 4);


    }


    void stdVecTest()
    {
        unsigned char tdata[] = {0xF0, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0xF7};
        int tdata_sz = 9;
        QCOMPARE((int)tdata[tdata_sz-1],(int)0xF7);


        DcMidiData md("F0 70 71 72 73 74 75 76 F7");
        int len = md.length();

        std::vector<unsigned char> vec = md.toStdVector();
        QCOMPARE(len, (int)vec.size());
        int offset;

        for (offset = 0; offset < tdata_sz ; offset++)
        {
            vec = md.toStdVector(offset);
            QCOMPARE((int)vec.size(),len - offset);
            QVERIFY(vec.at(0) == tdata[offset]);
            QVERIFY(vec.at(vec.size()-1) == tdata[tdata_sz-1]);
        }

        vec = md.toStdVector(0,1);
        QCOMPARE((int)vec.size(),1);
        QVERIFY(vec.at(0) == tdata[0]);

        for (offset = 0; offset < tdata_sz ; offset++)
        {
            vec = md.toStdVector(offset,1);
            QCOMPARE((int)vec.size(),1);
            QVERIFY(vec.at(0) == tdata[offset]);
        }
        
        vec = md.toStdVector(7,10);
        QCOMPARE((int)vec.size(),2);
        QVERIFY(vec.at(0) == tdata[7]);
        QVERIFY(vec.at(1) == tdata[8]);

    }

    void testStamp()
    {
        DcMidiData md("F0 00 F7");
        qint64 ts = QDateTime::currentMSecsSinceEpoch();
        md.setTimeStamp(ts);
        QVERIFY(md.getTimeStamp() == ts);
        
        
        ts = QDateTime::currentMSecsSinceEpoch();
        QThread::msleep(3);
        md.setTimeStamp();
        QVERIFY(md.getTimeStamp() > (ts+2LL));

        ts = QDateTime::currentMSecsSinceEpoch();
        QThread::msleep(10);
        md.setTimeStamp();
        QVERIFY(md.getTimeStamp() < ts+15);
    }

private:

    void checkContainsHelper(DcMidiData md,bool b)
    {
        qDebug() << md.toString();
        QVERIFY(b == md.contains("F0 33 12 F7"));
        QVERIFY(b == md.contains("33 12 F7"));
        QVERIFY(b == md.contains("12 F7"));
        QVERIFY(b == md.contains("F7"));
        QVERIFY(b == md.contains("F0 33 12 XX"));
        QVERIFY(b == md.contains("F0 33 XX XX"));
        QVERIFY(b == md.contains("F0 XX XX F7"));
        QVERIFY(b == md.contains("33 12 XX"));
        QVERIFY(b == md.contains("F0 XX 12 F7"));
        QVERIFY(b == md.contains("F0 33 XX F7"));
        QVERIFY(b == md.contains("F0 33 12 XX"));
        QVERIFY(b == md.contains("F0"));
        QVERIFY(b == md.contains("F0 33"));
        QVERIFY(b == md.contains("F0 33 12"));
        QVERIFY(b == md.contains("33 12 F7"));
        QVERIFY(b == md.contains("12 F7"));
        QVERIFY(b == md.contains("F7"));
    }

};

QTEST_MAIN(t_DcMidiData);

#include "t_dcmididata.moc"

