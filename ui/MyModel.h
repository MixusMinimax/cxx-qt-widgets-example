#ifndef MYAPP_MYMODEL_H
#define MYAPP_MYMODEL_H

#include <QObject>

class MyModel : public QObject {
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


#endif //MYAPP_MYMODEL_H
