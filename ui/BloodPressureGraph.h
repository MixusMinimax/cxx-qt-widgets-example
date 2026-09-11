#ifndef MYAPP_BLOODPRESSUREGRAPH_H
#define MYAPP_BLOODPRESSUREGRAPH_H

#include "MyModel.h"

#include <QCustomPlot>
#include <QWidget>
#include <backend/src/backend.cxxqt.h>
#include <rust/cxx.h>

#include <array>
#include <memory>

namespace measurements
{
    class QCPMeasurementPreview : public QCPItemText
    {
        Q_OBJECT

    public:
        explicit QCPMeasurementPreview(QCustomPlot *parentPlot);
        ~QCPMeasurementPreview() override = default;

        void setMeasurement(const measurement &m);
        void setPens(const std::array<QPen, 5> &pens) { m_pens = pens; }
        void setPens(std::array<QPen, 5> &&pens) { m_pens = std::move(pens); }
        void setHighlightPens(const std::array<QPen, 5> &pens) { m_highlightPens = pens; }
        void setHighlightPens(std::array<QPen, 5> &&pens) { m_highlightPens = std::move(pens); }
        void setHighlightBrushes(const std::array<QBrush, 5> &brushes) { m_highlightBrushes = brushes; }
        void setHighlightBrushes(std::array<QBrush, 5> &&brushes) { m_highlightBrushes = std::move(brushes); }
        void setHighlight(const std::optional<measurement_type> highlight) { m_highlight = highlight; }
        void setColumnGap(const int gap) { m_columnGap = gap; }
        void setDateTimeFormat(const QString &format);
        void setDateTimeFormat(QString &&format);

    protected:
        void updateText();

        void draw(QCPPainter *painter) override;

        measurement m_measurement{};
        std::array<std::pair<QString, QString>, 5> m_rows;
        std::array<QPen, 5> m_pens{Qt::NoPen, Qt::NoPen, Qt::NoPen, Qt::NoPen, Qt::NoPen};
        std::array<QPen, 5> m_highlightPens{Qt::NoPen, Qt::NoPen, Qt::NoPen, Qt::NoPen, Qt::NoPen};
        std::array<QBrush, 5> m_highlightBrushes{Qt::NoBrush, Qt::NoBrush, Qt::NoBrush, Qt::NoBrush, Qt::NoBrush};
        std::optional<measurement_type> m_highlight{};
        int m_columnGap{4};
        QString m_dateTimeFormat;

        static constexpr std::array<std::optional<measurement_type>, 5> TYPES{
            measurement_type::systolic,
            measurement_type::diastolic,
            measurement_type::map,
            measurement_type::pulse,
            std::nullopt
        };
    };
} // namespace measurements

class BloodPressureGraph : public QCustomPlot
{
    Q_OBJECT

public:
    explicit BloodPressureGraph(QWidget *parent = nullptr);

    ~BloodPressureGraph() override;

    void setModel(measurements::MeasurementModel *model);

    [[nodiscard]] measurements::MeasurementModel *model() const { return m_model; }

    /*!
     * Whether the mean arterial pressure (map) is selectable in the graph. If false, the cursor will not snap to it.
     */
    void setMapSelectable(const bool value) { m_mapSelectable = value; }

    [[nodiscard]] bool mapSelectable() const { return m_mapSelectable; }

signals:
    void mouseLeave(QEvent *event);
    void measurementEditStarted(measurements::measurement m);
    void measurementCreateStarted(QDateTime date_time);

protected slots:
    void updateMeasurements(::rust::Slice<const measurements::measurement> measurements);

protected:
    void leaveEvent(QEvent *event) override;

    QCPGraph *m_gSystolic, *m_gDiastolic, *m_gMap, *m_gPulse;

private:
    bool m_mapSelectable{true};
    measurements::MeasurementModel *m_model{nullptr};
    std::array<QMetaObject::Connection, 1> m_modelConnections;

    struct InternalState;
    std::unique_ptr<InternalState> m_state;
};

#endif // MYAPP_BLOODPRESSUREGRAPH_H
