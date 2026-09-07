#include "MyModel.h"

QDebug measurements::operator<<(QDebug d, const measurement &m)
{
    return d.nospace()
        << "{date_time: "
        << m.date_time
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

MyModel::MyModel(QObject *parent) : QObject(parent), m_speed{0} {}

MyModel::~MyModel() = default;

int MyModel::speed() const { return m_speed; }

void MyModel::setSpeed(const int speed)
{
    if (speed == m_speed) return;
    m_speed = speed;
    emit speedChanged(speed);
}

void MyModel::incrementSpeed() { setSpeed(m_speed + 1); }
