/****************************************************************************
** Meta object code from reading C++ file 'metricrangedialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.2.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../src/metricrangedialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'metricrangedialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.2.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_MetricRangeDialog_t {
    QByteArrayData data[9];
    char stringdata[96];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_MetricRangeDialog_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_MetricRangeDialog_t qt_meta_stringdata_MetricRangeDialog = {
    {
QT_MOC_LITERAL(0, 0, 17),
QT_MOC_LITERAL(1, 18, 12),
QT_MOC_LITERAL(2, 31, 0),
QT_MOC_LITERAL(3, 32, 13),
QT_MOC_LITERAL(4, 46, 13),
QT_MOC_LITERAL(5, 60, 16),
QT_MOC_LITERAL(6, 77, 3),
QT_MOC_LITERAL(7, 81, 4),
QT_MOC_LITERAL(8, 86, 8)
    },
    "MetricRangeDialog\0valueChanged\0\0"
    "long long int\0onButtonClick\0"
    "QAbstractButton*\0qab\0onOK\0onCancel\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MetricRangeDialog[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   34,    2, 0x06,

 // slots: name, argc, parameters, tag, flags
       4,    1,   37,    2, 0x0a,
       7,    0,   40,    2, 0x0a,
       8,    0,   41,    2, 0x0a,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    2,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MetricRangeDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        MetricRangeDialog *_t = static_cast<MetricRangeDialog *>(_o);
        switch (_id) {
        case 0: _t->valueChanged((*reinterpret_cast< long long int(*)>(_a[1]))); break;
        case 1: _t->onButtonClick((*reinterpret_cast< QAbstractButton*(*)>(_a[1]))); break;
        case 2: _t->onOK(); break;
        case 3: _t->onCancel(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (MetricRangeDialog::*_t)(long long int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&MetricRangeDialog::valueChanged)) {
                *result = 0;
            }
        }
    }
}

const QMetaObject MetricRangeDialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_MetricRangeDialog.data,
      qt_meta_data_MetricRangeDialog,  qt_static_metacall, 0, 0}
};


const QMetaObject *MetricRangeDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MetricRangeDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MetricRangeDialog.stringdata))
        return static_cast<void*>(const_cast< MetricRangeDialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int MetricRangeDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
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
void MetricRangeDialog::valueChanged(long long int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
