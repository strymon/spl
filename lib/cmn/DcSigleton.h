// This code was taken from https://qt-project.org/wiki/Qt_thread-safe_singleton
// In it's Last edit: April 20, 2012 state.
#ifndef DcSigleton_h__
#define DcSigleton_h__

#include <QtCore/QtGlobal>
#include <QtCore/QScopedPointer>
#include "DcCallOnce.h"

template <class T>
class DcSingleton
{
public:
    static T& instance()
    {
        qCallOnce(init, flag);
        return *tptr;
    }

    static void init()
    {
        tptr.reset(new T);
    }

private:
    DcSingleton() {};
    ~DcSingleton() {};
    Q_DISABLE_COPY(DcSingleton)

        static QScopedPointer<T> tptr;
    static QBasicAtomicInt flag;
};

template<class T> QScopedPointer<T> DcSingleton<T>::tptr(0);
template<class T> QBasicAtomicInt DcSingleton<T>::flag
    = Q_BASIC_ATOMIC_INITIALIZER(CallOnce::CO_Request);
#endif // DcSigleton_h__
