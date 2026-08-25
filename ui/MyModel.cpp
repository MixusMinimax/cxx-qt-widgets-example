#include "MyModel.h"

MyModel::MyModel(QObject *parent) : QObject(parent), m_speed{0} {}

MyModel::~MyModel() = default;

int MyModel::speed() const { return m_speed; }

void MyModel::setSpeed(const int speed) {
    if (speed == m_speed) return;
    m_speed = speed;
    emit speedChanged(speed);
}

void MyModel::incrementSpeed() { setSpeed(m_speed + 1); }
