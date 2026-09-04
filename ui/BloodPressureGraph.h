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

namespace measurements
{
    enum struct measurement_type
    {
        systolic,
        diastolic,
        map,
        pulse
    };

    struct measurement
    {
        QDateTime date_time;
        double systolic{0};
        double diastolic{0};
        double map{0};
        double pulse{0};
        double key{0};
        friend QDebug operator<<(QDebug d, const measurement &m);

        auto operator<=>(const measurement &measurement) const = default;
    };

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

        measurement m_measurement;
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

#endif // MYAPP_BLOODPRESSUREGRAPH_H
