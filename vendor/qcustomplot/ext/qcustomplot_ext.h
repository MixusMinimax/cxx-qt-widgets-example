/***************************************************************************
**                                                                        **
**  Extensions for QCustomPlot                                            **
**  Copyright (C) 2026 Maxi Barmetler                                     **
**  QCustomPlot is created by Emanuel Eichhammer:                         **
**  Copyright (C) 2011-2022 Emanuel Eichhammer                            **
**                                                                        **
**  This file is subject to the                                           **
**  GNU GENERAL PUBLIC LICENSE, Version 3.                                **
**  A copy can be found at ../GPL.txt.                                    **
**                                                                        **
****************************************************************************/

#ifndef QCUSTOMPLOT_EXT_H
#define QCUSTOMPLOT_EXT_H

#include "../qcustomplot.h"

#include <QBrush>
#include <QObject>
#include <QPen>

class QCPAxisTag;
class QCPItemTextTag;

/*!
    This class is based on https://www.qcustomplot.com/index.php/tutorials/specialcases/axistags with modifications.

    This tag renders as a rectangle with one edge pointy, depending on the position of the supplied axis.
    It works with horizontal and vertical axes.
    By default, the y-coordinate is in plot-space.
 */
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
    void setValue(double value);

    Q_REQUIRED_RESULT QPen pen() const;
    Q_REQUIRED_RESULT QBrush brush() const;
    Q_REQUIRED_RESULT QString text() const;
    Q_REQUIRED_RESULT bool visible() const;
    Q_REQUIRED_RESULT int tagSize() const;

    Q_REQUIRED_RESULT QCPItemPosition *position() const;

protected:
    QPointer<QCPAxis> mAxis;
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
