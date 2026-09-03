#ifndef QCUSTOMPLOT_EXT_H
#define QCUSTOMPLOT_EXT_H

#include "../qcustomplot.h"

#include <QBrush>
#include <QObject>
#include <QPen>

class QCPAxisTag;
class QCPItemTextTag;

class QCPAxisTag : QObject
{
    Q_OBJECT

public:
    explicit QCPAxisTag(QCPAxis *parent_axis);
    ~QCPAxisTag() override;

    void setPen(const QPen &pen);
    void setBrush(const QBrush &brush);
    void setText(const QString &text);
    void setVisible(bool on);
    void setTagSize(int size);
    void setPosition(double value);

    Q_REQUIRED_RESULT QPen pen() const;
    Q_REQUIRED_RESULT QBrush brush() const;
    Q_REQUIRED_RESULT QString text() const;
    Q_REQUIRED_RESULT bool visible() const;
    Q_REQUIRED_RESULT int tagSize() const;

    Q_REQUIRED_RESULT QCPItemPosition *position() const;

protected:
    QCPAxis *mAxis;
    QPointer<QCPItemTracer> mDummyTracer;
    QPointer<QCPItemTextTag> mLabel;
};


class QCPItemTextTag : public QCPItemText
{
    Q_OBJECT

public:
    explicit QCPItemTextTag(QCustomPlot *parentPlot);
    ~QCPItemTextTag() override;

    void setTagSize(int size);
    [[nodiscard]] int tagSize() const;

protected:
    int mTagSize{0};

    void draw(QCPPainter *painter) override;
};

#endif
