S3 for Qt
===============

This code is S3 interface to put/get object. This code is tested only on Riak CS(S3 compatible storage), not S3 itself.

How to Use
--------------

Add to Your Project
~~~~~~~~~~~~~~~~~~~~~~~~

Download/git submodule add/etc this module to your directory, then add ``qt-s3.pri`` to your ``.pro`` file.

Creating Bucket object
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // If you use Riak CS, host name should be
   // same name defined at app.config.
   QString s3Host = "s3.amazonaws.com";
   
   // Proxy is optional
   // If you use Riak CS, it is a real IP address to access it.
   QString s3Proxy = "http://127.0.0.1:8080";

   QS3::S3 s3(s3Host, s3Proxy);
   QS3::Bucket* bucket = s3.bucket("bucket-name", s3accessKey, s3secretKey);

Putting Object
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // uploaded is called when each item would be uploaded
   connect(bucket, &S3::Bucket::uploaded, [=] (const QString& key, int httpStatus) {
       qDebug() << key << " is uploaded";
   });

   // finished is called when all items would be uploaded
   connect(bucket, &S3::Bucket::finished, [=] () {
       qDebug() << " all items are uploaded";
   });

   // this signal is emitted when progress information is updated
   connect(bucket, &S3::Bucket::progress, [=] (const QString& key, qint64 sent, qint64 total) {
       qDebug() << "progress:" << key << sent << "/" << total;
   });

   // upload method is async method.
   bucket->upload("test_file.txt", QByteArray("test file content"));

   // If you want to wait until finish, use QEventLoop
   QEventLoop loop;
   connect(bucket, SIGNAL(finished()), &loop, SLOT(quit()));
   bucket->upload("test_file.txt", QByteArray("test file content"));
   loop.exec();

Getting Object (1)
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // uploaded is called when each item would be downloaded
   // This is an only way to receive data.
   connect(bucket, &S3::Bucket::downloaded, [=] (const QString& key, const QByteArray& content, int httpStatus) {
       qDebug() << key << " is downloaded";
   });

   // uploaded is called when all items would be downloaded
   connect(bucket, &S3::Bucket::finished, [=] () {
       qDebug() << " all items are uploaded";
   });

   // this signal is emitted when progress information is updated
   connect(bucket, &S3::Bucket::progress, [=] (const QString& key, qint64 sent, qint64 total) {
       qDebug() << "progress:" << key << sent << "/" << total;
   });

   // download method is async method.
   bucket->download("test_file.txt");

   // If you want to wait until finish, use QEventLoop
   QEventLoop loop;
   connect(bucket, SIGNAL(finished()), &loop, SLOT(quit()));
   bucket->download("test_file.txt");
   loop.exec();

Getting Object (2)
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // at first, total size is not accurate because it doesn't know
   // actual data size.
   connect(bucket, &S3::Bucket::progress, [=] (const QString& key, qint64 sent, qint64 total) {
       qDebug() << "progress:" << key << sent << "/" << total;
   });

   // downloadSync method is sync method.
   QByteArray content
   int httpStatus = bucket->downloadSync("test_file.txt", content);

License
--------------

MIT

How to Run unit-test
-------------------------

1. Install `riakcs-helper <https://github.com/shibukawa/riakcs-helper>`_ and init.
2. Prepare a bucket and a user (including access key, secret key) to use in test.
3. Fix const strings in `test/s3test.cpp` and `test/buckettest.cpp`.

History
-------------

12/10/2014
~~~~~~~~~~~~~~~

* initial version

12/16/2014
~~~~~~~~~~~~~~~

* ``downloaded`` and ``uploaded`` signal send ``httpStatus`` as a last param.
* remove ``progress`` signal of total progress.
* rename ``progressTask`` signal to ``progress``.
* add ``downloadSync`` method.
