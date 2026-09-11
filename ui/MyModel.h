#ifndef MYAPP_MYMODEL_H
#define MYAPP_MYMODEL_H

#include <QDebug>

namespace measurements
{
    struct measurement;
    QDebug operator<<(QDebug d, const measurement &m);
}

#endif // MYAPP_MYMODEL_H
