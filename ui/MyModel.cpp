#include "MyModel.h"

#include <backend/src/backend.cxxqt.h>
#include <qcustomplot.h>

QDebug measurements::operator<<(QDebug d, const measurement &m)
{
    return d.nospace()
        << "{date_time: "
        << QCPAxisTickerDateTime::keyToDateTime(m.key)
        << ", systolic: "
        << m.systolic
        << ", diastolic: "
        << m.diastolic
        << ", map: "
        << m.map
        << ", pulse: "
        << m.pulse
        << ", key: "
        << m.key
        << '}';
}
