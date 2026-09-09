#include "BloodPressureGraph.h"

#include "MyModel.h"
#include "overloaded.h"

#include <QBrush>
#include <QColor>
#include <QDateTime>
#include <QPen>
#include <QSharedPointer>
#include <backend/src/backend.cxxqt.h>
#include <rust/cxx.h>

#include <algorithm>
#include <format>
#include <optional>
#include <random>
#include <span>
#include <utility>

namespace
{
    using namespace measurements;
} // namespace


QCPMeasurementPreview::QCPMeasurementPreview(QCustomPlot *parentPlot)
    : QCPItemText{parentPlot}, m_dateTimeFormat{"hh:mm:ss\ndd.MM.yy"}
{}

void QCPMeasurementPreview::setMeasurement(const measurement &m)
{
    if (m_measurement == m) return;
    m_measurement = m;
    updateText();
}

void QCPMeasurementPreview::setDateTimeFormat(const QString &format)
{
    if (m_dateTimeFormat == format) return;
    m_dateTimeFormat = format;
    updateText();
}

void QCPMeasurementPreview::setDateTimeFormat(QString &&format)
{
    if (m_dateTimeFormat == format) return;
    m_dateTimeFormat = std::move(format);
    updateText();
}

void QCPMeasurementPreview::updateText()
{
    const auto locale = QLocale{};
    m_rows = {
        std::pair{tr("Systolic:"), tr("%1 mmHg").arg(locale.toString(m_measurement.systolic, 'g', 3))},
        std::pair{tr("Diastolic:"), tr("%1 mmHg").arg(locale.toString(m_measurement.diastolic, 'g', 3))},
        std::pair{tr("Map:"), tr("%1 mmHg").arg(locale.toString(m_measurement.map, 'g', 3))},
        std::pair{tr("Pulse:"), tr("%1 / min").arg(locale.toString(m_measurement.pulse, 'g', 3))},
        std::pair{tr("Date:"), locale.toString(m_measurement.date_time, m_dateTimeFormat)}
    };
}

void QCPMeasurementPreview::draw(QCPPainter *painter)
{
    auto pos = position->pixelPosition();
    const auto axis_rect = position->axisRect();
    painter->setFont(mainFont());
    const auto font_metrics = painter->fontMetrics();
    std::array<std::pair<QRect, QRect>, 5> rects{};
    int max_width_left{0};
    int max_width_right{0};
    int total_height{0};
    for (std::size_t i = 0; i < 5; ++i) {
        const auto [left, right] = rects[i] = {
            font_metrics.boundingRect(0, 0, 0, 0, Qt::TextDontClip | mTextAlignment, m_rows[i].first),
            font_metrics.boundingRect(0, 0, 0, 0, Qt::TextDontClip | mTextAlignment, m_rows[i].second),
        };
        max_width_left = std::max(max_width_left, left.width());
        max_width_right = std::max(max_width_right, right.width());
        total_height += std::max(left.height(), right.height());
    }
    auto text_box_rect = QRect{0, 0, max_width_left + 1 + m_columnGap + max_width_right, total_height}.adjusted(
        -mPadding.left(), -mPadding.top(), mPadding.right(), mPadding.bottom()
    );
    const auto text_pos = getTextDrawPoint(
        QPointF(0, 0), text_box_rect, mPositionAlignment
    ); // 0, 0 because the transform does the translation
    rects[0].first.moveTopLeft(text_pos.toPoint() + QPoint(mPadding.left(), mPadding.top()));
    rects[0].second.moveTopLeft(rects[0].first.topLeft() + QPoint{max_width_left + m_columnGap, 0});
    for (std::size_t i = 0; i < 5; ++i) {
        if (i != 0) {
            const auto top = std::max(rects[i - 1].first.bottom() + 1, rects[i - 1].second.bottom() + 1);
            rects[i].first.moveLeft(rects[i - 1].first.left());
            rects[i].first.moveTop(top);
            rects[i].second.moveLeft(rects[i - 1].second.left());
            rects[i].second.moveTop(top);
        }
    }
    text_box_rect.moveTopLeft(text_pos.toPoint());
    const auto clip_pad = qCeil(mainPen().widthF());
    const auto bounding_rect = text_box_rect.adjusted(-clip_pad, -clip_pad, clip_pad, clip_pad);
    if (mPositionAlignment.testFlag(Qt::AlignVCenter)) {
        // I don't really care about the other cases because I'm not using them
        pos.ry() = std::max(
            axis_rect->top() + bounding_rect.height() / 2.0 + 4,
            std::min(axis_rect->bottom() - bounding_rect.height() / 2.0 - 4, pos.y())
        );
    }
    auto transform = painter->transform();
    transform.translate(pos.x(), pos.y());
    if (!qFuzzyIsNull(mRotation)) transform.rotate(mRotation);

    if (transform.mapRect(bounding_rect).intersects(painter->transform().mapRect(clipRect()))) {
        painter->setTransform(transform);
        if ((mainBrush().style() != Qt::NoBrush && mainBrush().color().alpha() != 0)
            || (mainPen().style() != Qt::NoPen && mainPen().color().alpha() != 0)) {
            painter->setPen(mainPen());
            painter->setBrush(mainBrush());
            painter->drawRect(text_box_rect);
        }

        for (std::size_t i = 0; i < 5; ++i) {
            if (TYPES[i]
                && TYPES[i] == m_highlight
                && m_highlightBrushes[i] != Qt::NoBrush
                && m_highlightBrushes[i].color().alpha() != 0) {
                painter->setPen(Qt::NoPen);
                painter->setBrush(m_highlightBrushes[i]);
                const auto r = rects[i].first.united(rects[i].second);
                painter->drawRect(r);
            }

            if (TYPES[i]
                && TYPES[i] == m_highlight
                && m_highlightPens[i] != Qt::NoPen
                && m_highlightPens[i].color().alpha() != 0) {
                painter->setPen(m_highlightPens[i]);
            } else if (m_pens[i] != Qt::NoPen && m_pens[i].color().alpha() != 0) {
                painter->setPen(m_pens[i]);
            } else {
                painter->setPen(QPen{mainColor()});
            }
            painter->setBrush(Qt::NoBrush);
            painter->drawText(rects[i].first, m_rows[i].first, QTextOption{mTextAlignment});
            painter->drawText(rects[i].second, m_rows[i].second, QTextOption{mTextAlignment});
        }
    }
}

struct BloodPressureGraph::InternalState
{
    bool click_started{false};
    bool is_double_click{false};
    QPointF click_start;
    std::chrono::high_resolution_clock::time_point click_start_time;

    std::optional<measurement> under_cursor;
    std::optional<measurement> under_cursor_at_click_start;

    ::rust::Vec<measurement> measurements;
};

BloodPressureGraph::BloodPressureGraph(QWidget *parent)
    : QCustomPlot{parent}, m_model{nullptr}, m_state{std::make_unique<InternalState>()}
{
    m_state->measurements = {
        measurement{
            .systolic = 135, .diastolic = 84, .pulse = 66, .date_time = QDateTime{QDate{2026, 8, 15}, QTime{17, 42}}
        },
        measurement{
            .systolic = 135, .diastolic = 85, .pulse = 96, .date_time = QDateTime{QDate{2026, 8, 24}, QTime{21, 20}}
        },
        measurement{
            .systolic = 125, .diastolic = 81, .pulse = 74, .date_time = QDateTime{QDate{2026, 8, 28}, QTime{16, 54}}
        },
        measurement{
            .systolic = 144, .diastolic = 81, .pulse = 59, .date_time = QDateTime{QDate{2026, 8, 29}, QTime{11, 42}}
        },
        measurement{
            .systolic = 140, .diastolic = 81, .pulse = 64, .date_time = QDateTime{QDate{2026, 8, 29}, QTime{15, 27}}
        },
        measurement{
            .systolic = 127, .diastolic = 84, .pulse = 74, .date_time = QDateTime{QDate{2026, 8, 29}, QTime{17, 6}}
        },
        measurement{
            .systolic = 133, .diastolic = 91, .pulse = 57, .date_time = QDateTime{QDate{2026, 8, 30}, QTime{11, 21}}
        },
        measurement{
            .systolic = 135, .diastolic = 85, .pulse = 71, .date_time = QDateTime{QDate{2026, 8, 31}, QTime{13, 25}}
        },
        measurement{
            .systolic = 123, .diastolic = 78, .pulse = 64, .date_time = QDateTime{QDate{2026, 9, 1}, QTime{16, 37}}
        },
        measurement{
            .systolic = 131, .diastolic = 86, .pulse = 56, .date_time = QDateTime{QDate{2026, 9, 2}, QTime{13, 00}}
        },
        measurement{
            .systolic = 130, .diastolic = 75, .pulse = 64, .date_time = QDateTime{QDate{2026, 9, 2}, QTime{17, 01}}
        },
        measurement{
            .systolic = 130, .diastolic = 83, .pulse = 61, .date_time = QDateTime{QDate{2026, 9, 3}, QTime{17, 32}}
        },
        measurement{
            .systolic = 132, .diastolic = 89, .pulse = 65, .date_time = QDateTime{QDate{2026, 9, 5}, QTime{16, 7}}
        },
        measurement{
            .systolic = 143, .diastolic = 84, .pulse = 53, .date_time = QDateTime{QDate{2026, 9, 7}, QTime{14, 7}}
        },
    };
    // purposefully deterministic, for repeatability of tests.
    std::mt19937 rng{123456789}; // NOLINT(*-msc51-cpp)
    for (auto &m: m_state->measurements) {
        std::ranges::for_each(m.id, [&rng](std::uint8_t &n) { n = rng(); });
        m.key = QCPAxisTickerDateTime::dateTimeToKey(m.date_time);
        // Mean Arterial Pressure = 1/3*(SBP) + 2/3*(DBP)
        // DOI: 10.1097/CCM.0000000000000324
        m.map = 1.0 / 3 * m.systolic + 2.0 / 3 * m.diastolic;
    }

    m_gSystolic = addGraph(xAxis, yAxis);
    m_gDiastolic = addGraph(xAxis, yAxis);
    m_gMap = addGraph(xAxis, yAxis);

    m_gPulse = addGraph(xAxis, yAxis2);

    // add data
    for (auto &&m: m_state->measurements) {
        m_gSystolic->data()->add(QCPGraphData{m.key, m.systolic});
        m_gDiastolic->data()->add(QCPGraphData{m.key, m.diastolic});
        m_gMap->data()->add(QCPGraphData{m.key, m.map});
        m_gPulse->data()->add(QCPGraphData{m.key, m.pulse});
    }

    // graph style

    constexpr QColor pressure_color{75, 75, 190};
    constexpr QColor pressure_color_bg{53, 53, 200, 84};
    constexpr QColor pressure_color_light{210, 210, 250};
    constexpr QColor pulse_color{150, 33, 33};
    constexpr QColor pulse_color_bg{200, 53, 53, 84};
    constexpr QColor pulse_color_light{250, 210, 210};

    m_gSystolic->setPen(QPen{pressure_color, 2});
    m_gDiastolic->setPen(QPen{pressure_color, 2});
    const QCPScatterStyle blood_scatter{
        QCPScatterStyle::ssDisc,
        QPen{pressure_color},
        QBrush{pressure_color},
        6,
    };
    m_gSystolic->setScatterStyle(blood_scatter);
    m_gDiastolic->setScatterStyle(blood_scatter);

    m_gSystolic->setBrush(QBrush{pressure_color_bg});
    m_gSystolic->setBrush(QBrush{pressure_color_bg});
    m_gSystolic->setChannelFillGraph(m_gDiastolic);

    m_gMap->setPen(QPen{Qt::white, 2});

    m_gPulse->setPen(QPen{pulse_color, 2});
    m_gPulse->setScatterStyle(
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
    m_gSystolic->setSelectable(QCP::stSingleData);
    m_gDiastolic->setSelectable(QCP::stSingleData);
    m_gMap->setSelectable(QCP::stSingleData);
    m_gPulse->setSelectable(QCP::stSingleData);

    // give the axes some labels:
    xAxis->setLabel(tr("Date"));
    yAxis->setLabel(tr("Pressure (mmHg)"));
    yAxis2->setLabel(tr("Pulse (min^-1)"));
    yAxis2->setVisible(true);
    // set ticker
    QSharedPointer<QCPAxisTickerDateTime> dateTicker{new QCPAxisTickerDateTime};
    dateTicker->setDateTimeFormat("d. MMMM\nyyyy\nhh:mm");
    xAxis->setTicker(dateTicker);
    xAxis->setTickLabelFont(QFont{QFont{}.family(), 8});
    xAxis->grid()->setSubGridVisible(true);
    yAxis->grid()->setSubGridVisible(true);

    // set axis ranges, so we see all data
    // default range encompasses all values TODO: default range should probably be current week or something
    const auto first_day = m_state->measurements.front().date_time.date();
    const auto last_day = m_state->measurements.back().date_time.date().addDays(1);
    xAxis->setRange(QCPAxisTickerDateTime::dateTimeToKey(first_day), QCPAxisTickerDateTime::dateTimeToKey(last_day));
    yAxis->setRange(0, 200);
    yAxis2->setRange(20, 300);

    // selection highlight
    // the default tracer can't have different
    const auto cursor_tracer = new QCPItemTracer{this};
    cursor_tracer->setVisible(false);
    cursor_tracer->position->setType(QCPItemPosition::ptAbsolute);

    const auto selection_line = new QCPItemStraightLine{this};
    selection_line->point1->setParentAnchorX(cursor_tracer->position);
    selection_line->point1->setTypeX(QCPItemPosition::ptAbsolute);
    selection_line->point1->setTypeY(QCPItemPosition::ptAxisRectRatio);
    selection_line->point1->setCoords(0, 0);
    selection_line->point2->setParentAnchorX(cursor_tracer->position);
    selection_line->point2->setTypeY(QCPItemPosition::ptAxisRectRatio);
    selection_line->point2->setType(QCPItemPosition::ptAbsolute);
    selection_line->point2->setCoords(0, 1);
    selection_line->setPen(QPen{Qt::darkBlue});
    selection_line->setLayer("grid");
    selection_line->setVisible(false);
    selection_line->setAntialiased(false);

    const auto value_line = new QCPItemStraightLine{this};
    value_line->point1->setParentAnchorY(cursor_tracer->position);
    value_line->point1->setTypeX(QCPItemPosition::ptAxisRectRatio);
    value_line->point1->setTypeY(QCPItemPosition::ptAbsolute);
    value_line->point1->setCoords(0, 0);
    value_line->point2->setParentAnchorY(cursor_tracer->position);
    value_line->point2->setTypeX(QCPItemPosition::ptAxisRectRatio);
    value_line->point2->setTypeY(QCPItemPosition::ptAbsolute);
    value_line->point2->setCoords(1, 0);
    value_line->setPen(QPen{Qt::darkBlue, 1, Qt::DashLine});
    value_line->setLayer("grid");
    value_line->setVisible(false);
    value_line->setAntialiased(false);

    // tag on the left side showing mmHg, follows the cursor
    const auto pressure_tag = new QCPAxisTag{yAxis};
    pressure_tag->setPen(QPen{pressure_color});
    pressure_tag->setBrush(QBrush{pressure_color_light});
    pressure_tag->setVisible(false);
    // instead of setting the value, we will just attach it to the cursor here
    pressure_tag->position()->setParentAnchorY(cursor_tracer->position);
    pressure_tag->position()->setTypeY(QCPItemPosition::ptAbsolute);
    pressure_tag->setValue(0);

    // tag on the right side showing min^-1, follows the cursor
    const auto pulse_tag = new QCPAxisTag{yAxis2};
    pulse_tag->setPen(QPen{pulse_color});
    pulse_tag->setBrush(QBrush{pulse_color_light});
    pulse_tag->setVisible(false);
    // instead of setting the value, we will just attach it to the cursor here
    pulse_tag->position()->setParentAnchorY(cursor_tracer->position);
    pulse_tag->position()->setTypeY(QCPItemPosition::ptAbsolute);
    pulse_tag->setValue(0);

    auto measurement_preview = new QCPMeasurementPreview{this};
    measurement_preview->setVisible(false);
    measurement_preview->setPen(QPen{Qt::black});
    measurement_preview->setPens({pressure_color, pressure_color, pressure_color, pulse_color, Qt::NoPen});
    measurement_preview->setBrush(QBrush{Qt::white});
    measurement_preview->setHighlightBrushes(
        {pressure_color_light, pressure_color_light, pressure_color_light, pulse_color_light, Qt::NoBrush}
    );
    measurement_preview->setPositionAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    measurement_preview->setPadding({3, 3, 3, 3});
    measurement_preview->setColumnGap(8);
    measurement_preview->setDateTimeFormat("dd.MM.yy @ hh:mm");
    measurement_preview->position->setParentAnchor(cursor_tracer->position);
    measurement_preview->position->setCoords(10, 0);

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

    const auto move_cursor = [=, this](const QPointF pos) {
        // use binary search to find the start of the relevant range. Then we search for the closest point in the
        // radius. If there is none, the closest horizontal x-coordinate is used. Otherwise, nothing is found.
        const auto find_closest =
            [this](const QPointF mouse_pos, const double radius_hor_px, const double radius_px, const bool clip_radius)
            -> std::variant<std::monostate, std::tuple<measurement_type, measurement, double, QCPAxis *>, measurement> {
            const auto relevant_radius = clip_radius ? radius_hor_px : std::max(radius_hor_px, radius_px);
            const auto key_left = xAxis->range().clamp(xAxis->pixelToCoord(mouse_pos.x() - relevant_radius));
            const auto key_right = xAxis->range().clamp(xAxis->pixelToCoord(mouse_pos.x() + relevant_radius));
            std::optional<std::tuple<measurement_type, const measurement *, double, QCPAxis *>> closest_in_radius{};
            std::optional<const measurement *> closest_horizontal{};
            auto closest_distance_sq = std::numeric_limits<double>::max();
            auto closest_horizontal_distance = std::numeric_limits<double>::max();
            const std::span relevant_measurements{
                std::ranges::lower_bound(m_state->measurements, key_left, {}, &measurement::key),
                m_state->measurements.end()
            };
            for (const measurement &m: relevant_measurements) {
                if (m.key > key_right) break;
                const auto dx = std::abs(mouse_pos.x() - xAxis->coordToPixel(m.key));
                if (dx < closest_horizontal_distance && dx <= radius_hor_px) {
                    closest_horizontal_distance = dx;
                    closest_horizontal = &m;
                }
                const auto check_value = [&](const measurement_type type, const double value, QCPAxis *yAxis) {
                    const auto dy = mouse_pos.y() - yAxis->coordToPixel(value);
                    const auto distance_sq = dx * dx + dy * dy;
                    if (distance_sq < closest_distance_sq && distance_sq <= radius_px * radius_px) {
                        closest_distance_sq = distance_sq;
                        closest_in_radius = {type, &m, value, yAxis};
                    }
                };
                check_value(measurement_type::systolic, m.systolic, yAxis);
                check_value(measurement_type::diastolic, m.diastolic, yAxis);
                if (m_mapSelectable) check_value(measurement_type::map, m.map, yAxis);
                check_value(measurement_type::pulse, m.pulse, yAxis2);
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
        std::optional<measurement_type> highlight{};

        const auto snapped_pos = std::visit(
            overloaded{
                [&](std::monostate) {
                    cursor_tracer->position->setType(QCPItemPosition::ptAbsolute);
                    cursor_tracer->position->setCoords(pos.x(), pos.y());
                    m_state->under_cursor = std::nullopt;
                    return pos;
                },
                [&](const std::tuple<measurement_type, measurement, double, QCPAxis *> &p) {
                    const auto &[type, m, value, yAxis] = p;
                    cursor_tracer->position->setType(QCPItemPosition::ptPlotCoords);
                    cursor_tracer->position->setAxes(xAxis, yAxis);
                    cursor_tracer->position->setCoords(m.key, value);
                    m_state->under_cursor = m;
                    highlight = type;
                    pressure_tag_visible =
                        type != measurement_type::pulse ? tag_visibility::visible : tag_visibility::invisible;
                    pulse_tag_visible =
                        type == measurement_type::pulse ? tag_visibility::visible : tag_visibility::invisible;
                    return QPointF{xAxis->coordToPixel(m.key), yAxis->coordToPixel(value)};
                },
                [&](const measurement &m) {
                    cursor_tracer->position->setTypeX(QCPItemPosition::ptPlotCoords);
                    cursor_tracer->position->setTypeY(QCPItemPosition::ptAbsolute);
                    cursor_tracer->position->setAxes(xAxis, yAxis);
                    cursor_tracer->position->setCoords(m.key, pos.y());
                    m_state->under_cursor = m;
                    return QPointF{xAxis->coordToPixel(m.key), pos.y()};
                }
            },
            find_closest(pos, 10, 20, true)
        );
        selection_line->setVisible(true);
        value_line->setVisible(true);
        const auto mmHg = yAxis->pixelToCoord(snapped_pos.y());
        pressure_tag->setVisible(
            yAxis->range().contains(mmHg)
            && (pressure_tag_visible == tag_visibility::visible
                || pressure_tag_visible == tag_visibility::automatic && mmHg >= 55)
        );
        pressure_tag->setText(QString::number(mmHg, 'g', 3));

        const auto bpm = yAxis2->pixelToCoord(snapped_pos.y());
        pulse_tag->setVisible(
            yAxis2->range().contains(bpm)
            && (pulse_tag_visible == tag_visibility::visible
                || pulse_tag_visible == tag_visibility::automatic && bpm <= 125)
        );
        pulse_tag->setText(QString::number(bpm, 'g', 3));
        if (m_state->under_cursor) {
            measurement_preview->setVisible(true);
            measurement_preview->setMeasurement(*m_state->under_cursor);
            measurement_preview->setHighlight(highlight);
            if (snapped_pos.x() > axisRect()->right() - 185) {
                measurement_preview->position->setCoords(-10, 0);
                measurement_preview->setPositionAlignment(Qt::AlignRight | Qt::AlignVCenter);
            } else if (snapped_pos.x() < axisRect()->right() - 190) {
                measurement_preview->position->setCoords(10, 0);
                measurement_preview->setPositionAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            }
        } else {
            measurement_preview->setVisible(false);
        }
    };

    connect(this, &QCustomPlot::mousePress, [this](const QMouseEvent *e) {
        m_state->click_started = true;
        m_state->is_double_click = false;
        m_state->click_start_time = std::chrono::high_resolution_clock::now();
        m_state->click_start = e->position();
        m_state->under_cursor_at_click_start = m_state->under_cursor;
    });

    connect(this, &QCustomPlot::mouseDoubleClick, [this](const QMouseEvent *e) {
        m_state->click_started = true;
        m_state->is_double_click = true;
        m_state->click_start_time = std::chrono::high_resolution_clock::now();
        m_state->click_start = e->position();
        m_state->under_cursor_at_click_start = m_state->under_cursor;
    });

    connect(this, &QCustomPlot::mouseMove, [this, move_cursor](const QMouseEvent *e) {
        const auto pos = e->position();

        static const int MIN_DRAG_DISTANCE = QApplication::startDragDistance();

        // cancel click if we move too far
        if (m_state->click_started) {
            const auto vec = pos - m_state->click_start;
            const auto sq_len = vec.x() * vec.x() + vec.y() * vec.y();
            if (sq_len > MIN_DRAG_DISTANCE * MIN_DRAG_DISTANCE) m_state->click_started = false;
        }

        move_cursor(pos);

        replot(rpQueuedReplot);
    });

    connect(this, &BloodPressureGraph::mouseLeave, [this, selection_line, value_line, pressure_tag, pulse_tag] {
        m_state->click_started = false;
        selection_line->setVisible(false);
        value_line->setVisible(false);
        pressure_tag->setVisible(false);
        pulse_tag->setVisible(false);
        replot(rpQueuedReplot);
    });

    connect(this, &QCustomPlot::mouseRelease, [this, cursor_tracer] {
        static const std::chrono::milliseconds start_drag_time{QApplication::startDragTime()};

        if (m_state->click_started) {
            m_state->click_started = false;
            const auto elapsed = std::chrono::high_resolution_clock::now() - m_state->click_start_time;
            if (elapsed >= start_drag_time) return;

            auto debug = qDebug() << "Clicked at" << cursor_tracer->position->pixelPosition();
            if (m_state->is_double_click) {
                debug = debug << "double click";
                if (axisRect()->rect().contains(m_state->click_start.toPoint())) {
                    if (const auto &measurement = m_state->under_cursor) {
                        emit measurementEditStarted(*measurement);
                    } else {
                        emit measurementCreateStarted(
                            QCPAxisTickerDateTime::keyToDateTime(xAxis->pixelToCoord(m_state->click_start.x()))
                        );
                    }
                }
            }
            if (m_state->under_cursor_at_click_start) {
                debug = debug << *m_state->under_cursor_at_click_start;
            }
        }
    });

    replot();
}

BloodPressureGraph::~BloodPressureGraph() = default;

void BloodPressureGraph::setModel(MeasurementModel *model)
{
    if (model == m_model) return;
    if (m_model) {
        for (auto &&conn: m_modelConnections)
            disconnect(conn);
    }
    m_model = model;
    if (m_model) {
        const auto req_id = m_model->load_measurements();
        m_modelConnections = {connect(
            m_model,
            &MeasurementModel::measurements_loaded,
            [this, req_id](const uint32_t id, ::rust::Vec<measurement> measurements) {
                auto debug = qDebug().nospace() << "req id: " << req_id << "(" << id << "), measurements: [";
                std::ranges::for_each(measurements, [&debug](const measurement &m) {
                    debug = debug << "{s:" << m.systolic << ", d:" << m.diastolic << "},";
                });
                debug << "]";
                setMeasurements(std::move(measurements));
            }
        )};
    } else {
        m_modelConnections = {};
    }
}


void BloodPressureGraph::setMeasurements(::rust::Vec<measurement> measurements)
{
    m_gSystolic->data()->clear();
    m_gDiastolic->data()->clear();
    m_gMap->data()->clear();
    m_gPulse->data()->clear();

    for (auto &m: measurements) {
        if (m.key == 0) m.key = QCPAxisTickerDateTime::dateTimeToKey(m.date_time);
        if (m.map == 0) {
            // Mean Arterial Pressure = 1/3*(SBP) + 2/3*(DBP)
            // DOI: 10.1097/CCM.0000000000000324
            m.map = 1.0 / 3 * m.systolic + 2.0 / 3 * m.diastolic;
        }

        m_gSystolic->data()->add(QCPGraphData{m.key, m.systolic});
        m_gDiastolic->data()->add(QCPGraphData{m.key, m.diastolic});
        m_gMap->data()->add(QCPGraphData{m.key, m.map});
        m_gPulse->data()->add(QCPGraphData{m.key, m.pulse});
    }

    m_state->measurements = std::move(measurements);

    replot(rpQueuedReplot);
}

void BloodPressureGraph::leaveEvent(QEvent *event) { emit mouseLeave(event); }
