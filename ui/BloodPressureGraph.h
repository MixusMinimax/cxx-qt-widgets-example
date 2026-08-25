#ifndef MYAPP_BLOODPRESSUREGRAPH_H
#define MYAPP_BLOODPRESSUREGRAPH_H

#include <QCustomPlot>
#include <QWidget>

class MyModel;

class BloodPressureGraph : public QCustomPlot {
    Q_OBJECT

public:
    explicit BloodPressureGraph(QWidget *parent = nullptr);

    ~BloodPressureGraph() override;

    void setModel(MyModel *model);

    [[nodiscard]] MyModel *model() const;

private slots:
    void onSpeedChanged(int speed);

private:
    MyModel *m_model;
    QMetaObject::Connection m_modelConnection;
};


#endif //MYAPP_BLOODPRESSUREGRAPH_H
