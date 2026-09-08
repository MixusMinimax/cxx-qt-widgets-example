#ifndef MYAPP_MYMODEL_H
#define MYAPP_MYMODEL_H

#include <QDateTime>
#include <QDebug>
#include <QObject>

#include <array>

namespace measurements
{
    enum struct measurement_type
    {
        systolic,
        diastolic,
        map,
        pulse
    };

    struct measurement
    {
        std::array<std::uint32_t, 4> id;
        QDateTime date_time;
        double systolic{0};
        double diastolic{0};
        double map{0};
        double pulse{0};
        double key{0};

        auto operator<=>(const measurement &measurement) const = default;

        friend QDebug operator<<(QDebug d, const measurement &m);
    };
} // namespace measurements


#endif // MYAPP_MYMODEL_H
