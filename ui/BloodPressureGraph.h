#ifndef MYAPP_BLOODPRESSUREGRAPH_H
#define MYAPP_BLOODPRESSUREGRAPH_H

#include <QCustomPlot>
#include <QWidget>

#include <array>
#include <memory>

class MyModel;

class BloodPressureGraph : public QCustomPlot
{
    Q_OBJECT

public:
    explicit BloodPressureGraph(QWidget *parent = nullptr);

    ~BloodPressureGraph() override;

    void setModel(MyModel *model);

    [[nodiscard]] MyModel *model() const;

    void setMapSelectable(bool value);

    [[nodiscard]] bool mapSelectable() const;

signals:
    void mouseClick(QMouseEvent *release_event, QPointF click_start);
    void mouseLeave(QEvent *event);

private slots:
    void onSpeedChanged(int speed);

protected:
    void leaveEvent(QEvent *event) override;

private:
    bool m_mapSelectable{true};
    MyModel *m_model;
    std::array<QMetaObject::Connection, 1> m_modelConnections;

    struct InternalState;
    std::unique_ptr<InternalState> m_state;
};


#endif // MYAPP_BLOODPRESSUREGRAPH_H
