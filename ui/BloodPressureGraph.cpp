#include "BloodPressureGraph.h"

#include "MyModel.h"

#include <QBrush>
#include <QColor>
#include <QDateTime>
#include <QPen>
#include <QSharedPointer>

#include <algorithm>
#include <optional>
#include <ranges>
#include <span>

template<class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

struct BloodPressureGraph::InternalState
{
    bool click_started = false;
    QPoint click_start;

    std::optional<const QCPGraphData *> under_cursor;
    std::optional<const QCPGraphData *> under_cursor_at_click_start;
};

BloodPressureGraph::BloodPressureGraph(QWidget *parent)
    : QCustomPlot{parent}, m_model{nullptr}, m_state{std::make_unique<InternalState>()}
{
    struct measurement
    {
        QDateTime date_time;
        double systolic{0};
        double diastolic{0};
        double map{0};
        double pulse{0};
        double key{0};
    };
    std::array measurements{
        measurement{
            .date_time = QDateTime{QDate{2026, 8, 15}, QTime{17, 42}}, .systolic = 135, .diastolic = 84, .pulse = 66
        },
        measurement{
            .date_time = QDateTime{QDate{2026, 8, 24}, QTime{21, 20}}, .systolic = 135, .diastolic = 85, .pulse = 96
        },
        measurement{
            .date_time = QDateTime{QDate{2026, 8, 28}, QTime{16, 54}}, .systolic = 125, .diastolic = 81, .pulse = 74
        },
        measurement{
            .date_time = QDateTime{QDate{2026, 8, 29}, QTime{11, 42}}, .systolic = 144, .diastolic = 81, .pulse = 59
        },
        measurement{
            .date_time = QDateTime{QDate{2026, 8, 29}, QTime{15, 27}}, .systolic = 140, .diastolic = 81, .pulse = 64
        },
        measurement{
            .date_time = QDateTime{QDate{2026, 8, 29}, QTime{17, 6}}, .systolic = 127, .diastolic = 84, .pulse = 74
        },
        measurement{
            .date_time = QDateTime{QDate{2026, 8, 30}, QTime{11, 21}}, .systolic = 133, .diastolic = 91, .pulse = 57
        },
        measurement{
            .date_time = QDateTime{QDate{2026, 8, 31}, QTime{13, 25}}, .systolic = 135, .diastolic = 85, .pulse = 71
        },
        measurement{
            .date_time = QDateTime{QDate{2026, 9, 1}, QTime{16, 37}}, .systolic = 123, .diastolic = 78, .pulse = 64
        },
    };
    for (auto &m: measurements) {
        // Mean Arterial Pressure = 1/3*(SBP) + 2/3*(DBP)
        // DOI: 10.1097/CCM.0000000000000324
        m.map = 1.0 / 3 * m.systolic + 2.0 / 3 * m.diastolic;
        m.key = QCPAxisTickerDateTime::dateTimeToKey(m.date_time);
    }

    const auto g_systolic = addGraph(xAxis, yAxis);
    const auto g_diastolic = addGraph(xAxis, yAxis);
    const auto g_map = addGraph(xAxis, yAxis);

    const auto g_pulse = addGraph(xAxis, yAxis2);

    // add data
    for (auto &&[date_time, systolic, diastolic, map, pulse, key]: measurements) {
        g_systolic->data()->add(QCPGraphData{key, systolic});
        g_diastolic->data()->add(QCPGraphData{key, diastolic});
        g_map->data()->add(QCPGraphData{key, map});
        g_pulse->data()->add(QCPGraphData{key, pulse});
    }

    // graph style
    const QPen blood_pen{QColor{150, 33, 33, 255}, 2};
    g_systolic->setPen(blood_pen);
    g_diastolic->setPen(blood_pen);
    const QCPScatterStyle blood_scatter{
        QCPScatterStyle::ssDisc,
        QPen{QColor{150, 33, 33, 255}},
        QBrush{QColor{255, 255, 255, 255}},
        6,
    };
    g_systolic->setScatterStyle(blood_scatter);
    g_diastolic->setScatterStyle(blood_scatter);

    g_systolic->setBrush(QBrush{QColor{200, 53, 53, 84}});
    g_systolic->setChannelFillGraph(g_diastolic);

    g_map->setPen(QPen{QColor{255, 255, 255, 255}, 2});

    g_pulse->setPen(QPen{QColor{150, 33, 33, 255}, 2});
    g_pulse->setScatterStyle(
        QCPScatterStyle{
            QCPScatterStyle::ssCircle,
            QPen{QColor{150, 33, 33, 255}},
            QBrush{QColor{255, 255, 255, 255}},
            8,
        }
    );

    // interactions
    axisRect()->setRangeDrag(Qt::Horizontal);
    axisRect()->setRangeZoom(Qt::Horizontal);
    setInteractions(interactions() | QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    g_systolic->setSelectable(QCP::stSingleData);
    g_diastolic->setSelectable(QCP::stSingleData);
    g_map->setSelectable(QCP::stSingleData);
    g_pulse->setSelectable(QCP::stSingleData);

    yAxis2->setVisible(true);
    // give the axes some labels:
    xAxis->setLabel(tr("Date"));
    yAxis->setLabel(tr("Pressure (mmHg)"));
    yAxis2->setLabel(tr("Pulse (min^-1)"));
    // set ticker
    QSharedPointer<QCPAxisTickerDateTime> dateTicker{new QCPAxisTickerDateTime};
    dateTicker->setDateTimeFormat("d. MMMM\nyyyy\nhh:mm");
    xAxis->setTicker(dateTicker);
    xAxis->setTickLabelFont(QFont{QFont{}.family(), 8});
    // set axis ranges, so we see all data
    // default range encompasses all values TODO: default range should probably be current week or something
    const auto first_day = measurements.front().date_time.date();
    const auto last_day = measurements.back().date_time.date().addDays(1);
    xAxis->setRange(QCPAxisTickerDateTime::dateTimeToKey(first_day), QCPAxisTickerDateTime::dateTimeToKey(last_day));
    yAxis->setRange(0, 200);
    yAxis2->setRange(20, 300);

    // selection line
    const auto selectionLine = new QCPItemStraightLine{this};
    selectionLine->point1->setType(QCPItemPosition::ptAbsolute);
    selectionLine->point2->setType(QCPItemPosition::ptAbsolute);
    selectionLine->setPen(QPen{Qt::blue});
    selectionLine->setLayer("grid");

    if (QTimeZone::systemTimeZone().hasDaylightTime()) {
        auto adjust_ticker_for_daylight_time = [this, dateTicker = std::move(dateTicker)](const QCPRange &new_range) {
            auto dt = QCPAxisTickerDateTime::keyToDateTime(new_range.center());
            const auto d = dt.date();
            if (dt.isDaylightTime()) {
                dt = QDateTime{QDate{d.year(), 7, 1}, QTime{0, 0}};
            } else {
                dt = QDateTime{QDate{d.year(), 1, 1}, QTime{0, 0}};
            }
            if (const auto dt_ms = QCPAxisTickerDateTime::dateTimeToKey(dt); dt_ms != dateTicker->tickOrigin()) {
                dateTicker->setTickOrigin(dt);
                replot(rpQueuedReplot);
            }
        };
        adjust_ticker_for_daylight_time(xAxis->range());
        connect(xAxis, qOverload<const QCPRange &>(&QCPAxis::rangeChanged), std::move(adjust_ticker_for_daylight_time));
    }

    connect(this, &QCustomPlot::mousePress, [this](const QMouseEvent *e) {
        m_state->click_started = true;
        m_state->click_start = e->pos();
        m_state->under_cursor_at_click_start = m_state->under_cursor;
    });

    connect(this, &QCustomPlot::mouseMove, [=, this](const QMouseEvent *e) {
        // cancel click if we move too far
        if (m_state->click_started) {
            const auto vec = e->pos() - m_state->click_start;
            const auto sq_len = vec.x() * vec.x() + vec.y() * vec.y();
            if (sq_len > 5 * 5) m_state->click_started = false;
        }
        const auto find_closest = [this, g_systolic] [[nodiscard]] (
                                      const int x_px, const double radius_px
                                  ) -> std::optional<const QCPGraphData *> {
            // TODO: consider vertical distance, if cursor is close enough to data point.
            //       Otherwise, just horizontal distance. A horizontal line with value tags on the axis would be cool.
            const auto data = g_systolic->data();
            const auto end_it = data->constEnd();
            const auto key = xAxis->pixelToCoord(x_px);
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

        enum struct data_type
        {
            systolic,
            diastolic,
            map,
            pulse
        };

        const auto find_closest_2d = [=, this] [[nodiscard]] (
                                         const QPoint mouse_pos, const double radius_px
                                     ) -> std::optional<std::pair<data_type, QCPGraphData>> {
            const auto key_left = xAxis->pixelToCoord(mouse_pos.x() - radius_px);
            const auto key_right = xAxis->pixelToCoord(mouse_pos.x() + radius_px);
            std::optional<std::pair<data_type, QCPGraphData>> closest_in_radius{};
            std::optional<std::pair<data_type, QCPGraphData>> closest_horizontal{};
            double closest_distance_sq = std::numeric_limits<double>::max();
            double closest_horizontal_distance = std::numeric_limits<double>::max();
            for (const auto &[type, data, yAxis]: {
                     std::tuple{data_type::systolic, g_systolic->data(), yAxis},
                     std::tuple{data_type::diastolic, g_diastolic->data(), yAxis},
                     std::tuple{data_type::map, g_map->data(), yAxis},
                     std::tuple{data_type::pulse, g_pulse->data(), yAxis2},
                 }) {
                for (const auto &point: std::span{data->findBegin(key_left), data->constEnd()}
                         | std::ranges::views::take_while([&](const QCPGraphData &v) { return v.key <= key_right; })) {
                    const auto dx = std::abs(mouse_pos.x() - xAxis->coordToPixel(point.key));
                    const auto dy = std::abs(mouse_pos.y() - yAxis->coordToPixel(point.value));
                    if (dx < closest_horizontal_distance) {
                        closest_horizontal_distance = dx;
                        closest_horizontal = {type, point};
                    }
                    const auto distance_sq = dx * dx + dy * dy;
                    if (distance_sq < closest_distance_sq && distance_sq <= radius_px * radius_px) {
                        closest_distance_sq = distance_sq;
                        closest_in_radius = {type, point};
                    }
                }
            }

            return closest_in_radius ? closest_in_radius : closest_horizontal;
        };

        // use binary search to find the start of the relevant range. Then we search for the closest point in the
        // radius. If there is none, the closest horizontal x-coordinate is used. Otherwise, nothing is found.
        const auto find_closest_2d_b =
            [this, &measurements](
                const QPoint mouse_pos, const double radius_hor_px, const double radius_px
            ) -> std::variant<std::monostate, std::pair<data_type, measurement>, measurement> {
            const auto relevant_radius = std::max(radius_hor_px, radius_px);
            const auto key_left = xAxis->pixelToCoord(mouse_pos.x() - relevant_radius);
            const auto key_right = xAxis->pixelToCoord(mouse_pos.x() + relevant_radius);
            std::optional<std::pair<data_type, const measurement *>> closest_in_radius{};
            std::optional<const measurement *> closest_horizontal{};
            double closest_distance_sq = std::numeric_limits<double>::max();
            double closest_horizontal_distance = std::numeric_limits<double>::max();
            for (const measurement &m: std::span{
                     std::ranges::lower_bound(measurements, key_left, {}, [](const measurement &m) { return m.key; }),
                     measurements.end()
                 }) {
                if (m.key > key_right) break;
                const auto dx = std::abs(mouse_pos.x() - xAxis->coordToPixel(m.key));
                if (dx < closest_horizontal_distance && dx <= radius_hor_px) {
                    closest_horizontal_distance = dx;
                    closest_horizontal = &m;
                }
                const auto check_value = [&](const data_type type, const double value, const QCPAxis *yAxis) {
                    const auto dy = mouse_pos.y() - yAxis->coordToPixel(value);
                    const auto distance_sq = dx * dx + dy * dy;
                    if (distance_sq < closest_distance_sq && distance_sq <= radius_px * radius_px) {
                        closest_distance_sq = distance_sq;
                        closest_in_radius = {type, &m};
                    }
                };
                check_value(data_type::systolic, m.systolic, yAxis);
                check_value(data_type::diastolic, m.diastolic, yAxis);
                check_value(data_type::map, m.map, yAxis);
                check_value(data_type::pulse, m.map, yAxis2);
            }
            if (closest_in_radius) {
                return std::pair{closest_in_radius->first, *closest_in_radius->second};
            }
            if (closest_horizontal) {
                return **closest_horizontal;
            }
            return {};
        };

        std::visit(
            overloaded{
                [&](std::monostate) {
                    selectionLine->point1->setTypeX(QCPItemPosition::ptAbsolute);
                    selectionLine->point1->setCoords(e->pos().x(), 0);
                    selectionLine->point2->setTypeX(QCPItemPosition::ptAbsolute);
                    selectionLine->point2->setCoords(e->pos().x(), 10);
                },
                [&](const std::pair<data_type, measurement> &p) {
                    const auto &[type, m] = p;
                    selectionLine->point1->setTypeX(QCPItemPosition::ptPlotCoords);
                    selectionLine->point1->setCoords(m.key, 0);
                    selectionLine->point2->setTypeX(QCPItemPosition::ptPlotCoords);
                    selectionLine->point2->setCoords(m.key, 10);
                    // TODO: horizontal line
                },
                [&](const measurement &m) {
                    selectionLine->point1->setTypeX(QCPItemPosition::ptPlotCoords);
                    selectionLine->point1->setCoords(m.key, 0);
                    selectionLine->point2->setTypeX(QCPItemPosition::ptPlotCoords);
                    selectionLine->point2->setCoords(m.key, 10);
                }
            },
            find_closest_2d_b(e->pos(), 8, 12)
        );
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
