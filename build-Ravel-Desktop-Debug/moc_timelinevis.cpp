/****************************************************************************
** Meta object code from reading C++ file 'timelinevis.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.2.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../src/timelinevis.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'timelinevis.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.2.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_TimelineVis_t {
    QByteArrayData data[10];
    char stringdata[93];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_TimelineVis_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_TimelineVis_t qt_meta_stringdata_TimelineVis = {
    {
QT_MOC_LITERAL(0, 0, 11),
QT_MOC_LITERAL(1, 12, 11),
QT_MOC_LITERAL(2, 24, 0),
QT_MOC_LITERAL(3, 25, 6),
QT_MOC_LITERAL(4, 32, 5),
QT_MOC_LITERAL(5, 38, 9),
QT_MOC_LITERAL(6, 48, 8),
QT_MOC_LITERAL(7, 57, 14),
QT_MOC_LITERAL(8, 72, 10),
QT_MOC_LITERAL(9, 83, 8)
    },
    "TimelineVis\0selectEvent\0\0Event*\0event\0"
    "aggregate\0overdraw\0selectEntities\0"
    "QList<int>\0entities\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TimelineVis[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    3,   24,    2, 0x0a,
       7,    1,   31,    2, 0x0a,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3, QMetaType::Bool, QMetaType::Bool,    4,    5,    6,
    QMetaType::Void, 0x80000000 | 8,    9,

       0        // eod
};

void TimelineVis::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        TimelineVis *_t = static_cast<TimelineVis *>(_o);
        switch (_id) {
        case 0: _t->selectEvent((*reinterpret_cast< Event*(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 1: _t->selectEntities((*reinterpret_cast< QList<int>(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<int> >(); break;
            }
            break;
        }
    }
}

const QMetaObject TimelineVis::staticMetaObject = {
    { &VisWidget::staticMetaObject, qt_meta_stringdata_TimelineVis.data,
      qt_meta_data_TimelineVis,  qt_static_metacall, 0, 0}
};


const QMetaObject *TimelineVis::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TimelineVis::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_TimelineVis.stringdata))
        return static_cast<void*>(const_cast< TimelineVis*>(this));
    return VisWidget::qt_metacast(_clname);
}

int TimelineVis::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = VisWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
