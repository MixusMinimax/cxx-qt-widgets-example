#include "BloodPressureGraph.h"
#include "MyModel.h"

#include <QBrush>
#include <QColor>
#include <QDateTime>
#include <QPen>
#include <QSharedPointer>

#include <optional>

struct BloodPressureGraph::InternalState
{
    bool   click_started = false;
    QPoint click_start;

    std::optional<const QCPGraphData *> under_cursor;
    std::optional<const QCPGraphData *> under_cursor_at_click_start;
};

BloodPressureGraph::BloodPressureGraph(QWidget *parent)
    : QCustomPlot(parent), m_model{nullptr}, m_state{std::make_unique<InternalState>()}
{
    struct measurement
    {
        QDateTime date_time;
        double    systolic  = 0;
        double    diastolic = 0;
        double    map       = 0;
        double    pulse     = 0;
    };
    std::array measurements{
        measurement{
            .date_time = QDateTime(QDate(2026, 8, 15), QTime(17, 42)), .systolic = 135, .diastolic = 84, .pulse = 66
        },
        measurement{
            .date_time = QDateTime(QDate(2026, 8, 24), QTime(21, 20)), .systolic = 135, .diastolic = 85, .pulse = 96
        },
        measurement{
            .date_time = QDateTime(QDate(2026, 8, 28), QTime(16, 54)), .systolic = 125, .diastolic = 81, .pulse = 74
        },
        measurement{
            .date_time = QDateTime(QDate(2026, 8, 29), QTime(11, 42)), .systolic = 144, .diastolic = 81, .pulse = 59
        },
        measurement{
            .date_time = QDateTime(QDate(2026, 8, 29), QTime(15, 27)), .systolic = 140, .diastolic = 81, .pulse = 64
        },
        measurement{
            .date_time = QDateTime(QDate(2026, 8, 29), QTime(17, 6)), .systolic = 127, .diastolic = 84, .pulse = 74
        },
        measurement{
            .date_time = QDateTime(QDate(2026, 8, 30), QTime(11, 21)), .systolic = 133, .diastolic = 91, .pulse = 57
        },
    };
    for (auto &m: measurements) {
        // Mean Arterial Pressure = 1/3*(SBP) + 2/3*(DBP)
        // DOI: 10.1097/CCM.0000000000000324
        m.map = 1.0 / 3 * m.systolic + 2.0 / 3 * m.diastolic;
    }

    const auto g_systolic  = addGraph(xAxis, yAxis);
    const auto g_diastolic = addGraph(xAxis, yAxis);
    const auto g_map       = addGraph(xAxis, yAxis);

    const auto g_pulse     = addGraph(xAxis, yAxis2);

    // graph style
    const auto blood_pen = QPen{QColor{150, 33, 33, 255}, 2};
    g_systolic->setPen(blood_pen);
    g_diastolic->setPen(blood_pen);
    const auto blood_scatter = QCPScatterStyle{
        QCPScatterStyle::ssDisc,
        QPen{QColor{150, 33, 33, 255}},
        QBrush{QColor{255, 255, 255, 255}},
        6,
    };
    g_systolic->setScatterStyle(blood_scatter);
    g_diastolic->setScatterStyle(blood_scatter);

    g_systolic->setBrush(QBrush{QColor{200, 53, 53, 84}});
    g_systolic->setChannelFillGraph(g_diastolic);

    auto map_pen = QPen{QColor{255, 255, 255, 255}};
    map_pen.setWidth(2);
    g_map->setPen(map_pen);

    g_pulse->setPen(QPen{QColor{150, 33, 33, 255}, 2});
    g_pulse->setScatterStyle(QCPScatterStyle{
        QCPScatterStyle::ssCircle,
        QPen{QColor{150, 33, 33, 255}},
        QBrush{QColor{255, 255, 255, 255}},
        8,
    });

    const auto first_day = measurements.front().date_time.date();
    const auto last_day  = measurements.back().date_time.date().addDays(1);

    for (auto &&[date_time, systolic, diastolic, map, pulse]: measurements) {
        const auto key = QCPAxisTickerDateTime::dateTimeToKey(date_time);
        g_systolic->data()->add(QCPGraphData(key, systolic));
        g_diastolic->data()->add(QCPGraphData(key, diastolic));
        g_map->data()->add(QCPGraphData(key, map));
        g_pulse->data()->add(QCPGraphData(key, pulse));
    }

    yAxis2->setVisible(true);
    // give the axes some labels:
    xAxis->setLabel(tr("Date"));
    yAxis->setLabel(tr("Pressure (mmHg)"));
    yAxis2->setLabel(tr("Pulse (min^-1)"));
    // set ticker
    QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
    dateTicker->setDateTimeFormat("d. MMMM\nyyyy");
    xAxis->setTicker(std::move(dateTicker));
    xAxis->setTickLabelFont(QFont(QFont().family(), 8));
    // set axes ranges, so we see all data:
    xAxis->setRange(QCPAxisTickerDateTime::dateTimeToKey(first_day), QCPAxisTickerDateTime::dateTimeToKey(last_day));
    yAxis->setRange(0, 200);
    yAxis2->setRange(20, 300);

    axisRect()->setRangeDrag(Qt::Horizontal);
    axisRect()->setRangeZoom(Qt::Horizontal);
    setInteractions(interactions() | QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    g_systolic->setSelectable(QCP::stSingleData);
    g_diastolic->setSelectable(QCP::stSingleData);
    g_map->setSelectable(QCP::stSingleData);
    g_pulse->setSelectable(QCP::stSingleData);

    // selection line
    auto selectionLine = new QCPItemStraightLine{this};
    selectionLine->point1->setType(QCPItemPosition::ptAbsolute);
    selectionLine->point2->setType(QCPItemPosition::ptAbsolute);
    selectionLine->setPen(QPen{Qt::blue});
    selectionLine->setLayer("grid");

    connect(this, &QCustomPlot::mousePress, [this](const QMouseEvent *e) {
        m_state->click_started               = true;
        m_state->click_start                 = e->pos();
        m_state->under_cursor_at_click_start = m_state->under_cursor;
    });

    connect(this, &QCustomPlot::mouseMove, [this, selectionLine, g_systolic](const QMouseEvent *e) {
        // cancel click if we move too far
        if (m_state->click_started) {
            const auto vec    = e->pos() - m_state->click_start;
            const auto sq_len = vec.x() * vec.x() + vec.y() * vec.y();
            if (sq_len > 5 * 5) m_state->click_started = false;
        }
        const auto find_closest = [this, g_systolic] [[nodiscard]] (
                                      const int x_px, const double radius_px) -> std::optional<const QCPGraphData *> {
            const auto data      = g_systolic->data();
            const auto end_it    = data->constEnd();
            const auto key       = xAxis->pixelToCoord(x_px);
            const auto before_it = data->findBegin(key, true);
            if (before_it == data->constEnd()) return std::nullopt;
            const auto diff_before = x_px - xAxis->coordToPixel(before_it->key);
            // before first data point:
            if (diff_before < 0) return -diff_before <= radius_px ? std::optional{&*before_it} : std::nullopt;
            const auto after_it = before_it + 1;
            // after last data point:
            if (after_it == end_it) return diff_before <= radius_px ? std::optional{&*before_it} : std::nullopt;
            // the cursor is guaranteed to be between two points
            const auto diff_after = xAxis->coordToPixel(after_it->key) - x_px;
            if (diff_before > radius_px && diff_after > radius_px) return std::nullopt;
            if (diff_before < diff_after) return &*before_it;
            return &*after_it;
        };

        if (const auto closest = m_state->under_cursor = find_closest(e->pos().x(), 8)) {
            selectionLine->point1->setTypeX(QCPItemPosition::ptPlotCoords);
            selectionLine->point1->setCoords((*closest)->key, 0);
            selectionLine->point2->setTypeX(QCPItemPosition::ptPlotCoords);
            selectionLine->point2->setCoords((*closest)->key, 10);
        } else {
            selectionLine->point1->setTypeX(QCPItemPosition::ptAbsolute);
            selectionLine->point1->setCoords(e->pos().x(), 0);
            selectionLine->point2->setTypeX(QCPItemPosition::ptAbsolute);
            selectionLine->point2->setCoords(e->pos().x(), 10);
        }
        selectionLine->setVisible(true);
        replot(rpQueuedReplot);
    });

    connect(this, &BloodPressureGraph::mouseLeave, [this, selectionLine] {
        selectionLine->setVisible(false);
        replot(rpQueuedReplot);
    });

    connect(this, &QCustomPlot::mouseRelease, [this](QMouseEvent *event) {
        if (m_state->click_started) {
            emit mouseClick(event, m_state->click_start);
            auto debug = qDebug() << "Clicked at" << m_state->click_start;
            if (m_state->under_cursor_at_click_start) {
                debug << (*m_state->under_cursor_at_click_start)->value;
            }
        }
    });

    replot();
}

BloodPressureGraph::~BloodPressureGraph() = default;

void BloodPressureGraph::setModel(MyModel *model)
{
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

auto BloodPressureGraph::model() const -> MyModel * { return m_model; }

void BloodPressureGraph::onSpeedChanged(const int speed) { qDebug() << "Speed: " << speed; }

void BloodPressureGraph::leaveEvent(QEvent *event) { emit mouseLeave(event); }
