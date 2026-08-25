#include "BloodPressureGraph.h"
#include "MyModel.h"

BloodPressureGraph::BloodPressureGraph(QWidget *parent) : QCustomPlot(parent), m_model{nullptr} {
    // generate some data:
    QVector<double> x(101), y(101); // initialize with entries 0..100
    for (int i = 0; i < 101; ++i) {
        x[i] = i / 50.0 - 1; // x goes from -1 to 1
        y[i] = x[i] * x[i]; // let's plot a quadratic function
    }
    // create graph and assign data to it:
    addGraph();
    graph(0)->setData(x, y);
    // give the axes some labels:
    xAxis->setLabel("x");
    yAxis->setLabel("y");
    // set axes ranges, so we see all data:
    xAxis->setRange(-1, 1);
    yAxis->setRange(0, 1);
    replot();
}

BloodPressureGraph::~BloodPressureGraph() = default;

void BloodPressureGraph::setModel(MyModel *model) {
    if (model == m_model) return;
    if (m_model) {
        for (auto &&conn: m_modelConnections)
            disconnect(conn);
    }
    m_model = model;
    if (m_model) {
        m_modelConnections = {
            connect(m_model, &MyModel::speedChanged, this, &BloodPressureGraph::onSpeedChanged),
        };
        onSpeedChanged(m_model->speed());
    } else {
        m_modelConnections = {};
    }
}

MyModel *BloodPressureGraph::model() const {
    return m_model;
}

void BloodPressureGraph::onSpeedChanged(const int speed) {
    qDebug() << "Speed: " << speed;
    graph(0)->data()->begin()->value = speed / 100.0;
    replot();
}
