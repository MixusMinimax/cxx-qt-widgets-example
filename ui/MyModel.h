#ifndef MYAPP_MYMODEL_H
#define MYAPP_MYMODEL_H

#include <QDebug>
#include <rust/cxx.h>

namespace measurements
{
    struct measurement;
    QDebug operator<<(QDebug d, const measurement &m);
}

template<typename T>
constexpr bool std::ranges::enable_borrowed_range<::rust::Slice<T>> = true;
template<typename T>
constexpr bool std::ranges::enable_view<::rust::Slice<T>> = true;


#endif // MYAPP_MYMODEL_H
