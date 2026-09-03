#include "BloodPressureGraph.h"

#include "MyModel.h"

#include <QBrush>
#include <QColor>
#include <QDateTime>
#include <QPen>
#include <QSharedPointer>

#include <algorithm>
#include <optional>
#include <span>
#include <vector>

namespace
{
    template<class... Ts>
    struct overloaded : Ts...
    {
        using Ts::operator()...;
    };
}

struct BloodPressureGraph::InternalState
{
    bool click_started = false;
    QPointF click_start;

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
    std::vector measurements{
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
        measurement{
            .date_time = QDateTime{QDate{2026, 9, 2}, QTime{13, 00}}, .systolic = 131, .diastolic = 86, .pulse = 56
        },
        measurement{
            .date_time = QDateTime{QDate{2026, 9, 2}, QTime{17, 01}}, .systolic = 130, .diastolic = 75, .pulse = 64
        },
        measurement{
            .date_time = QDateTime{QDate{2026, 9, 3}, QTime{17, 32}}, .systolic = 130, .diastolic = 83, .pulse = 61
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

    constexpr QColor pressure_color{75, 75, 190};
    constexpr QColor pressure_color_bg{53, 53, 200, 84};
    constexpr QColor pressure_color_light{210, 210, 250};
    constexpr QColor pulse_color{150, 33, 33};
    constexpr QColor pulse_color_bg{200, 53, 53, 84};
    constexpr QColor pulse_color_light{250, 210, 210};

    g_systolic->setPen(QPen{pressure_color, 2});
    g_diastolic->setPen(QPen{pressure_color, 2});
    const QCPScatterStyle blood_scatter{
        QCPScatterStyle::ssDisc,
        QPen{pressure_color},
        QBrush{pressure_color},
        6,
    };
    g_systolic->setScatterStyle(blood_scatter);
    g_diastolic->setScatterStyle(blood_scatter);

    g_systolic->setBrush(QBrush{pressure_color_bg});
    g_systolic->setBrush(QBrush{pressure_color_bg});
    g_systolic->setChannelFillGraph(g_diastolic);

    g_map->setPen(QPen{Qt::white, 2});

    g_pulse->setPen(QPen{pulse_color, 2});
    g_pulse->setScatterStyle(
        QCPScatterStyle{
            QCPScatterStyle::ssCircle,
            QPen{pulse_color},
            QBrush{Qt::white},
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
    const auto selection_line = new QCPItemStraightLine{this};
    selection_line->point1->setType(QCPItemPosition::ptAbsolute);
    selection_line->point2->setType(QCPItemPosition::ptAbsolute);
    selection_line->setPen(QPen{Qt::darkBlue});
    selection_line->setLayer("grid");
    selection_line->setVisible(false);
    selection_line->setAntialiased(false);

    const auto value_line = new QCPItemStraightLine{this};
    value_line->point1->setType(QCPItemPosition::ptAbsolute);
    value_line->point2->setType(QCPItemPosition::ptAbsolute);
    value_line->setPen(QPen{Qt::darkBlue, 1, Qt::DashLine});
    value_line->setLayer("grid");
    value_line->setVisible(false);
    value_line->setAntialiased(false);

    const auto pressure_tag = new QCPAxisTag{yAxis};
    const auto pulse_tag = new QCPAxisTag{yAxis2};
    pressure_tag->setPen(QPen{pressure_color});
    pressure_tag->setBrush(QBrush{pressure_color_light});
    pressure_tag->setVisible(false);
    pulse_tag->setPen(QPen{pulse_color});
    pulse_tag->setBrush(QBrush{pulse_color_light});
    pulse_tag->setVisible(false);

    // effects
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
        m_state->click_start = e->position();
        m_state->under_cursor_at_click_start = m_state->under_cursor;
    });

    connect(this, &QCustomPlot::mouseMove, [=, this](const QMouseEvent *e) {
        const auto pos = e->position();

        // cancel click if we move too far
        if (m_state->click_started) {
            const auto vec = pos - m_state->click_start;
            const auto sq_len = vec.x() * vec.x() + vec.y() * vec.y();
            if (sq_len > 5 * 5) m_state->click_started = false;
        }

        enum struct data_type
        {
            systolic,
            diastolic,
            map,
            pulse
        };

        // use binary search to find the start of the relevant range. Then we search for the closest point in the
        // radius. If there is none, the closest horizontal x-coordinate is used. Otherwise, nothing is found.
        const auto find_closest_2d_b =
            [this, &measurements](
                const QPointF mouse_pos, const double radius_hor_px, const double radius_px, const bool clip_radius
            ) -> std::variant<std::monostate, std::tuple<data_type, measurement, double, QCPAxis *>, measurement> {
            const auto relevant_radius = clip_radius ? radius_hor_px : std::max(radius_hor_px, radius_px);
            const auto key_left = xAxis->pixelToCoord(mouse_pos.x() - relevant_radius);
            const auto key_right = xAxis->pixelToCoord(mouse_pos.x() + relevant_radius);
            std::optional<std::tuple<data_type, const measurement *, double, QCPAxis *>> closest_in_radius{};
            std::optional<const measurement *> closest_horizontal{};
            auto closest_distance_sq = std::numeric_limits<double>::max();
            auto closest_horizontal_distance = std::numeric_limits<double>::max();
            const std::span relevant_measurements{
                std::ranges::lower_bound(measurements, key_left, {}, &measurement::key), measurements.end()
            };
            for (const measurement &m: relevant_measurements) {
                if (m.key > key_right) break;
                const auto dx = std::abs(mouse_pos.x() - xAxis->coordToPixel(m.key));
                if (dx < closest_horizontal_distance && dx <= radius_hor_px) {
                    closest_horizontal_distance = dx;
                    closest_horizontal = &m;
                }
                const auto check_value = [&](const data_type type, const double value, QCPAxis *yAxis) {
                    const auto dy = mouse_pos.y() - yAxis->coordToPixel(value);
                    const auto distance_sq = dx * dx + dy * dy;
                    if (distance_sq < closest_distance_sq && distance_sq <= radius_px * radius_px) {
                        closest_distance_sq = distance_sq;
                        closest_in_radius = {type, &m, value, yAxis};
                    }
                };
                check_value(data_type::systolic, m.systolic, yAxis);
                check_value(data_type::diastolic, m.diastolic, yAxis);
                if (m_mapSelectable) check_value(data_type::map, m.map, yAxis);
                check_value(data_type::pulse, m.pulse, yAxis2);
            }
            if (closest_in_radius) {
                auto &[t, m, value, yAxis] = *closest_in_radius;
                return std::tuple{t, *m, value, yAxis};
            }
            if (closest_horizontal) {
                return **closest_horizontal;
            }
            return {};
        };

        enum struct tag_visibility
        {
            automatic,
            visible,
            invisible,
        };

        tag_visibility pressure_tag_visible{};
        tag_visibility pulse_tag_visible{};

        const auto snapped_pos = std::visit(
            overloaded{
                [&](std::monostate) {
                    selection_line->point1->setTypeX(QCPItemPosition::ptAbsolute);
                    selection_line->point1->setCoords(pos.x(), 0);
                    selection_line->point2->setTypeX(QCPItemPosition::ptAbsolute);
                    selection_line->point2->setCoords(pos.x(), 10);
                    value_line->point1->setTypeY(QCPItemPosition::ptAbsolute);
                    value_line->point1->setCoords(0, pos.y());
                    value_line->point2->setTypeY(QCPItemPosition::ptAbsolute);
                    value_line->point2->setCoords(10, pos.y());
                    return pos;
                },
                [&](const std::tuple<data_type, measurement, double, QCPAxis *> &p) {
                    const auto &[type, m, value, yAxis] = p;
                    selection_line->point1->setTypeX(QCPItemPosition::ptPlotCoords);
                    selection_line->point1->setCoords(m.key, 0);
                    selection_line->point2->setTypeX(QCPItemPosition::ptPlotCoords);
                    selection_line->point2->setCoords(m.key, 10);
                    value_line->point1->setTypeY(QCPItemPosition::ptPlotCoords);
                    value_line->point1->setAxes(xAxis, yAxis);
                    value_line->point1->setCoords(0, value);
                    value_line->point2->setTypeY(QCPItemPosition::ptPlotCoords);
                    value_line->point2->setAxes(xAxis, yAxis);
                    value_line->point2->setCoords(10, value);
                    pressure_tag_visible =
                        type != data_type::pulse ? tag_visibility::visible : tag_visibility::invisible;
                    pulse_tag_visible = type == data_type::pulse ? tag_visibility::visible : tag_visibility::invisible;
                    return QPointF{xAxis->coordToPixel(m.key), yAxis->coordToPixel(value)};
                },
                [&](const measurement &m) {
                    selection_line->point1->setTypeX(QCPItemPosition::ptPlotCoords);
                    selection_line->point1->setCoords(m.key, 0);
                    selection_line->point2->setTypeX(QCPItemPosition::ptPlotCoords);
                    selection_line->point2->setCoords(m.key, 10);
                    value_line->point1->setTypeY(QCPItemPosition::ptAbsolute);
                    value_line->point1->setCoords(0, pos.y());
                    value_line->point2->setTypeY(QCPItemPosition::ptAbsolute);
                    value_line->point2->setCoords(10, pos.y());
                    return QPointF{xAxis->coordToPixel(m.key), pos.y()};
                }
            },
            find_closest_2d_b(e->pos(), 10, 20, true)
        );
        selection_line->setVisible(true);
        value_line->setVisible(true);
        const auto mmHg = yAxis->pixelToCoord(snapped_pos.y());
        pressure_tag->setVisible(
            yAxis->range().contains(mmHg)
            && (pressure_tag_visible == tag_visibility::visible
                || pressure_tag_visible == tag_visibility::automatic && mmHg >= 55)
        );
        pressure_tag->setValue(mmHg);
        pressure_tag->setText(QString::number(mmHg, 'g', 3));

        const auto bpm = yAxis2->pixelToCoord(snapped_pos.y());
        pulse_tag->setVisible(
            yAxis2->range().contains(bpm)
            && (pulse_tag_visible == tag_visibility::visible
                || pulse_tag_visible == tag_visibility::automatic && bpm <= 125)
        );
        pulse_tag->setValue(bpm);
        pulse_tag->setText(QString::number(bpm, 'g', 3));

        replot(rpQueuedReplot);
    });

    connect(this, &BloodPressureGraph::mouseLeave, [this, selection_line, value_line, pressure_tag, pulse_tag] {
        selection_line->setVisible(false);
        value_line->setVisible(false);
        pressure_tag->setVisible(false);
        pulse_tag->setVisible(false);
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

void BloodPressureGraph::setMapSelectable(const bool value) { m_mapSelectable = value; }

bool BloodPressureGraph::mapSelectable() const { return m_mapSelectable; }

void BloodPressureGraph::onSpeedChanged(const int speed) { qDebug() << "Speed: " << speed; }

void BloodPressureGraph::leaveEvent(QEvent *event) { emit mouseLeave(event); }
