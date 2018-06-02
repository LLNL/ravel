/****************************************************************************
** Meta object code from reading C++ file 'viswidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.2.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../src/viswidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'viswidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.2.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_VisWidget_t {
    QByteArrayData data[17];
    char stringdata[157];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_VisWidget_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_VisWidget_t qt_meta_stringdata_VisWidget = {
    {
QT_MOC_LITERAL(0, 0, 9),
QT_MOC_LITERAL(1, 10, 10),
QT_MOC_LITERAL(2, 21, 0),
QT_MOC_LITERAL(3, 22, 11),
QT_MOC_LITERAL(4, 34, 5),
QT_MOC_LITERAL(5, 40, 4),
QT_MOC_LITERAL(6, 45, 4),
QT_MOC_LITERAL(7, 50, 12),
QT_MOC_LITERAL(8, 63, 6),
QT_MOC_LITERAL(9, 70, 3),
QT_MOC_LITERAL(10, 74, 16),
QT_MOC_LITERAL(11, 91, 10),
QT_MOC_LITERAL(12, 102, 9),
QT_MOC_LITERAL(13, 112, 7),
QT_MOC_LITERAL(14, 120, 11),
QT_MOC_LITERAL(15, 132, 14),
QT_MOC_LITERAL(16, 147, 8)
    },
    "VisWidget\0repaintAll\0\0timeChanged\0"
    "start\0stop\0jump\0eventClicked\0Event*\0"
    "evt\0entitiesSelected\0QList<int>\0"
    "processes\0setTime\0selectEvent\0"
    "selectEntities\0entities\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_VisWidget[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   54,    2, 0x06,
       3,    3,   55,    2, 0x06,
       7,    1,   62,    2, 0x06,
      10,    1,   65,    2, 0x06,

 // slots: name, argc, parameters, tag, flags
      13,    3,   68,    2, 0x0a,
      13,    2,   75,    2, 0x2a,
      14,    1,   80,    2, 0x0a,
      15,    1,   83,    2, 0x0a,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Float, QMetaType::Float, QMetaType::Bool,    4,    5,    6,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void, 0x80000000 | 11,   12,

 // slots: parameters
    QMetaType::Void, QMetaType::Float, QMetaType::Float, QMetaType::Bool,    4,    5,    6,
    QMetaType::Void, QMetaType::Float, QMetaType::Float,    4,    5,
    QMetaType::Void, 0x80000000 | 8,    2,
    QMetaType::Void, 0x80000000 | 11,   16,

       0        // eod
};

void VisWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        VisWidget *_t = static_cast<VisWidget *>(_o);
        switch (_id) {
        case 0: _t->repaintAll(); break;
        case 1: _t->timeChanged((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 2: _t->eventClicked((*reinterpret_cast< Event*(*)>(_a[1]))); break;
        case 3: _t->entitiesSelected((*reinterpret_cast< QList<int>(*)>(_a[1]))); break;
        case 4: _t->setTime((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 5: _t->setTime((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 6: _t->selectEvent((*reinterpret_cast< Event*(*)>(_a[1]))); break;
        case 7: _t->selectEntities((*reinterpret_cast< QList<int>(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<int> >(); break;
            }
            break;
        case 7:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<int> >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (VisWidget::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&VisWidget::repaintAll)) {
                *result = 0;
            }
        }
        {
            typedef void (VisWidget::*_t)(float , float , bool );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&VisWidget::timeChanged)) {
                *result = 1;
            }
        }
        {
            typedef void (VisWidget::*_t)(Event * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&VisWidget::eventClicked)) {
                *result = 2;
            }
        }
        {
            typedef void (VisWidget::*_t)(QList<int> );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&VisWidget::entitiesSelected)) {
                *result = 3;
            }
        }
    }
}

const QMetaObject VisWidget::staticMetaObject = {
    { &QGLWidget::staticMetaObject, qt_meta_stringdata_VisWidget.data,
      qt_meta_data_VisWidget,  qt_static_metacall, 0, 0}
};


const QMetaObject *VisWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *VisWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_VisWidget.stringdata))
        return static_cast<void*>(const_cast< VisWidget*>(this));
    if (!strcmp(_clname, "CommDrawInterface"))
        return static_cast< CommDrawInterface*>(const_cast< VisWidget*>(this));
    return QGLWidget::qt_metacast(_clname);
}

int VisWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGLWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void VisWidget::repaintAll()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void VisWidget::timeChanged(float _t1, float _t2, bool _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void VisWidget::eventClicked(Event * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void VisWidget::entitiesSelected(QList<int> _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_END_MOC_NAMESPACE
