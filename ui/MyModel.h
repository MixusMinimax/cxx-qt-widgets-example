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

class MyModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int speed READ speed WRITE setSpeed NOTIFY speedChanged)

public:
    explicit MyModel(QObject *parent = nullptr);

    ~MyModel() override;

    [[nodiscard]] int speed() const;

    void setSpeed(int speed);

    void incrementSpeed();

signals:
    void speedChanged(int speed);

private:
    int m_speed;
};


#endif // MYAPP_MYMODEL_H
