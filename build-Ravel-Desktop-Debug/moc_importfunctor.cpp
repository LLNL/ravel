/****************************************************************************
** Meta object code from reading C++ file 'importfunctor.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.2.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../src/importfunctor.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'importfunctor.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.2.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_ImportFunctor_t {
    QByteArrayData data[15];
    char stringdata[168];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_ImportFunctor_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_ImportFunctor_t qt_meta_stringdata_ImportFunctor = {
    {
QT_MOC_LITERAL(0, 0, 13),
QT_MOC_LITERAL(1, 14, 9),
QT_MOC_LITERAL(2, 24, 0),
QT_MOC_LITERAL(3, 25, 4),
QT_MOC_LITERAL(4, 30, 6),
QT_MOC_LITERAL(5, 37, 14),
QT_MOC_LITERAL(6, 52, 11),
QT_MOC_LITERAL(7, 64, 12),
QT_MOC_LITERAL(8, 77, 12),
QT_MOC_LITERAL(9, 90, 17),
QT_MOC_LITERAL(10, 108, 14),
QT_MOC_LITERAL(11, 123, 7),
QT_MOC_LITERAL(12, 131, 3),
QT_MOC_LITERAL(13, 135, 16),
QT_MOC_LITERAL(14, 152, 14)
    },
    "ImportFunctor\0switching\0\0done\0Trace*\0"
    "reportProgress\0doImportOTF\0dataFileName\0"
    "doImportOTF2\0finishInitialRead\0"
    "updateMatching\0portion\0msg\0updatePreprocess\0"
    "switchProgress\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ImportFunctor[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   59,    2, 0x06,
       3,    1,   60,    2, 0x06,
       5,    2,   63,    2, 0x06,

 // slots: name, argc, parameters, tag, flags
       6,    1,   68,    2, 0x0a,
       8,    1,   71,    2, 0x0a,
       9,    0,   74,    2, 0x0a,
      10,    2,   75,    2, 0x0a,
      13,    2,   80,    2, 0x0a,
      14,    0,   85,    2, 0x0a,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4,    2,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,    2,    2,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,   11,   12,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,   11,   12,
    QMetaType::Void,

       0        // eod
};

void ImportFunctor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        ImportFunctor *_t = static_cast<ImportFunctor *>(_o);
        switch (_id) {
        case 0: _t->switching(); break;
        case 1: _t->done((*reinterpret_cast< Trace*(*)>(_a[1]))); break;
        case 2: _t->reportProgress((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 3: _t->doImportOTF((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 4: _t->doImportOTF2((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 5: _t->finishInitialRead(); break;
        case 6: _t->updateMatching((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 7: _t->updatePreprocess((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 8: _t->switchProgress(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (ImportFunctor::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ImportFunctor::switching)) {
                *result = 0;
            }
        }
        {
            typedef void (ImportFunctor::*_t)(Trace * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ImportFunctor::done)) {
                *result = 1;
            }
        }
        {
            typedef void (ImportFunctor::*_t)(int , QString );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ImportFunctor::reportProgress)) {
                *result = 2;
            }
        }
    }
}

const QMetaObject ImportFunctor::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ImportFunctor.data,
      qt_meta_data_ImportFunctor,  qt_static_metacall, 0, 0}
};


const QMetaObject *ImportFunctor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ImportFunctor::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ImportFunctor.stringdata))
        return static_cast<void*>(const_cast< ImportFunctor*>(this));
    return QObject::qt_metacast(_clname);
}

int ImportFunctor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void ImportFunctor::switching()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void ImportFunctor::done(Trace * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ImportFunctor::reportProgress(int _t1, QString _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_END_MOC_NAMESPACE
