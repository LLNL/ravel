/****************************************************************************
** Meta object code from reading C++ file 'traditionalvis.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.2.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../src/traditionalvis.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'traditionalvis.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.2.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_TraditionalVis_t {
    QByteArrayData data[9];
    char stringdata[84];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_TraditionalVis_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_TraditionalVis_t qt_meta_stringdata_TraditionalVis = {
    {
QT_MOC_LITERAL(0, 0, 14),
QT_MOC_LITERAL(1, 15, 15),
QT_MOC_LITERAL(2, 31, 0),
QT_MOC_LITERAL(3, 32, 19),
QT_MOC_LITERAL(4, 52, 6),
QT_MOC_LITERAL(5, 59, 7),
QT_MOC_LITERAL(6, 67, 5),
QT_MOC_LITERAL(7, 73, 4),
QT_MOC_LITERAL(8, 78, 4)
    },
    "TraditionalVis\0timeScaleString\0\0"
    "taskPropertyDisplay\0Event*\0setTime\0"
    "start\0stop\0jump\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TraditionalVis[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   34,    2, 0x06,
       3,    1,   37,    2, 0x06,

 // slots: name, argc, parameters, tag, flags
       5,    3,   40,    2, 0x0a,
       5,    2,   47,    2, 0x2a,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void, 0x80000000 | 4,    2,

 // slots: parameters
    QMetaType::Void, QMetaType::Float, QMetaType::Float, QMetaType::Bool,    6,    7,    8,
    QMetaType::Void, QMetaType::Float, QMetaType::Float,    6,    7,

       0        // eod
};

void TraditionalVis::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        TraditionalVis *_t = static_cast<TraditionalVis *>(_o);
        switch (_id) {
        case 0: _t->timeScaleString((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 1: _t->taskPropertyDisplay((*reinterpret_cast< Event*(*)>(_a[1]))); break;
        case 2: _t->setTime((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 3: _t->setTime((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (TraditionalVis::*_t)(QString );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&TraditionalVis::timeScaleString)) {
                *result = 0;
            }
        }
        {
            typedef void (TraditionalVis::*_t)(Event * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&TraditionalVis::taskPropertyDisplay)) {
                *result = 1;
            }
        }
    }
}

const QMetaObject TraditionalVis::staticMetaObject = {
    { &TimelineVis::staticMetaObject, qt_meta_stringdata_TraditionalVis.data,
      qt_meta_data_TraditionalVis,  qt_static_metacall, 0, 0}
};


const QMetaObject *TraditionalVis::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TraditionalVis::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_TraditionalVis.stringdata))
        return static_cast<void*>(const_cast< TraditionalVis*>(this));
    return TimelineVis::qt_metacast(_clname);
}

int TraditionalVis::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = TimelineVis::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void TraditionalVis::timeScaleString(QString _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void TraditionalVis::taskPropertyDisplay(Event * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
