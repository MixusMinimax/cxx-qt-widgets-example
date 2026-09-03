// ReSharper disable CppMemberFunctionMayBeConst
#include "qcustomplot_ext.h"

QCPAxisTag::QCPAxisTag(QCPAxis *parent_axis) : QObject{parent_axis}, mAxis{parent_axis}
{
    mDummyTracer = new QCPItemTracer{mAxis->parentPlot()};
    mDummyTracer->setVisible(false);
    switch (mAxis->axisType()) {
        case QCPAxis::atLeft:
        case QCPAxis::atRight:
            mDummyTracer->position->setTypeX(QCPItemPosition::ptAxisRectRatio);
            mDummyTracer->position->setTypeY(QCPItemPosition::ptPlotCoords);
            mDummyTracer->position->setAxes(nullptr, mAxis);
            break;
        case QCPAxis::atTop:
        case QCPAxis::atBottom:
            mDummyTracer->position->setTypeX(QCPItemPosition::ptPlotCoords);
            mDummyTracer->position->setTypeY(QCPItemPosition::ptAxisRectRatio);
            mDummyTracer->position->setAxes(mAxis, nullptr);
            break;
    }
    switch (mAxis->axisType()) {
        case QCPAxis::atLeft:
            mDummyTracer->position->setCoords(0, 0);
            break;
        case QCPAxis::atRight:
            mDummyTracer->position->setCoords(1, 0);
            break;
        case QCPAxis::atTop:
            mDummyTracer->position->setCoords(0, 0);
            break;
        case QCPAxis::atBottom:
            mDummyTracer->position->setCoords(0, 1);
            break;
    }
    mDummyTracer->position->setAxisRect(mAxis->axisRect());

    // The text label is anchored at the arrow start (tail) and has its "position"
    // aligned at the left, and vertically centered to the text label box.
    mLabel = new QCPItemTextTag{mAxis->parentPlot()};
    mLabel->setTagSize(8);
    mLabel->setLayer("overlay");
    mLabel->setClipToAxisRect(false);
    mLabel->setPadding(QMargins{5, 0, 5, 0});
    mLabel->setBrush(QBrush{Qt::white});
    mLabel->setPen(QPen{Qt::blue});
    mLabel->position->setParentAnchor(mDummyTracer->position);

    // set orientation-dependent positions
    switch (mAxis->axisType()) {
        case QCPAxis::atLeft:
            mLabel->setPositionAlignment(Qt::AlignRight | Qt::AlignVCenter);
            break;
        case QCPAxis::atRight:
            mLabel->setPositionAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            break;
        case QCPAxis::atTop:
            mLabel->setPositionAlignment(Qt::AlignHCenter | Qt::AlignBottom);
            break;
        case QCPAxis::atBottom:
            mLabel->setPositionAlignment(Qt::AlignHCenter | Qt::AlignTop);
            break;
    }

    setPosition(0);
}

QCPAxisTag::~QCPAxisTag()
{
    if (mDummyTracer) mDummyTracer->parentPlot()->removeItem(mDummyTracer);
    if (mLabel) mLabel->parentPlot()->removeItem(mLabel);
}

void QCPAxisTag::setPen(const QPen &pen) { mLabel->setPen(pen); }

void QCPAxisTag::setBrush(const QBrush &brush) { mLabel->setBrush(brush); }

void QCPAxisTag::setText(const QString &text) { mLabel->setText(text); }

void QCPAxisTag::setVisible(const bool on) { mLabel->setVisible(on); }

void QCPAxisTag::setTagSize(const int size) { mLabel->setTagSize(size); }

void QCPAxisTag::setPosition(const double value)
{
    switch (mAxis->axisType()) {
        case QCPAxis::atLeft:
            mDummyTracer->position->setCoords(0, value);
            mLabel->position->setCoords(mAxis->offset(), 0);
            break;
        case QCPAxis::atRight:
            mDummyTracer->position->setCoords(1, value);
            mLabel->position->setCoords(mAxis->offset(), 0);
            break;
        case QCPAxis::atTop:
            mDummyTracer->position->setCoords(value, 0);
            mLabel->position->setCoords(0, mAxis->offset());
            break;
        case QCPAxis::atBottom:
            mDummyTracer->position->setCoords(value, 1);
            mLabel->position->setCoords(0, mAxis->offset());
            break;
    }
}

QPen QCPAxisTag::pen() const { return mLabel->pen(); }
QBrush QCPAxisTag::brush() const { return mLabel->brush(); }
QString QCPAxisTag::text() const { return mLabel->text(); }
bool QCPAxisTag::visible() const { return mLabel->visible(); }
int QCPAxisTag::tagSize() const { return mLabel->tagSize(); }
QCPItemPosition *QCPAxisTag::position() const { return mDummyTracer->position; }

// QCPItemTextTag

QCPItemTextTag::QCPItemTextTag(QCustomPlot *parentPlot) : QCPItemText{parentPlot} {}
QCPItemTextTag::~QCPItemTextTag() = default;
void QCPItemTextTag::setTagSize(const int size) { mTagSize = size; }
int QCPItemTextTag::tagSize() const { return mTagSize; }

void QCPItemTextTag::draw(QCPPainter *painter)
{
    // effective padding includes the triangle
    QMargins effectivePadding{mPadding};
    if (mPositionAlignment.testFlags(Qt::AlignLeft | Qt::AlignVCenter)) {
        effectivePadding.setLeft(effectivePadding.left() + mTagSize);
    } else if (mPositionAlignment.testFlags(Qt::AlignRight | Qt::AlignVCenter)) {
        effectivePadding.setRight(effectivePadding.right() + mTagSize);
    } else if (mPositionAlignment.testFlags(Qt::AlignHCenter | Qt::AlignBottom)) {
        effectivePadding.setBottom(effectivePadding.bottom() + mTagSize);
    } else if (mPositionAlignment.testFlags(Qt::AlignHCenter | Qt::AlignTop)) {
        effectivePadding.setTop(effectivePadding.top() + mTagSize);
    }


    const QPointF pos{position->pixelPosition()};
    QTransform transform = painter->transform();
    transform.translate(pos.x(), pos.y());
    if (!qFuzzyIsNull(mRotation)) transform.rotate(mRotation);
    painter->setFont(mainFont());
    QRect textRect = painter->fontMetrics().boundingRect(0, 0, 0, 0, Qt::TextDontClip | mTextAlignment, mText);
    QRect textBoxRect = textRect.adjusted(
        -effectivePadding.left(), -effectivePadding.top(), effectivePadding.right(), effectivePadding.bottom()
    );
    const auto textPos = getTextDrawPoint(
        QPointF(0, 0), textBoxRect, mPositionAlignment
    ); // 0, 0 because the transform does the translation
    textRect.moveTopLeft(textPos.toPoint() + QPoint(effectivePadding.left(), effectivePadding.top()));
    textBoxRect.moveTopLeft(textPos.toPoint());
    const auto clipPad = qCeil(mainPen().widthF());
    const auto boundingRect = textBoxRect.adjusted(-clipPad, -clipPad, clipPad, clipPad);
    if (transform.mapRect(boundingRect).intersects(painter->transform().mapRect(clipRect()))) {
        painter->setTransform(transform);
        if ((mainBrush().style() != Qt::NoBrush && mainBrush().color().alpha() != 0)
            || (mainPen().style() != Qt::NoPen && mainPen().color().alpha() != 0)) {
            painter->setPen(mainPen());
            painter->setBrush(mainBrush());
            auto polygon = QPolygon{};
            polygon.reserve(6);
            if (mPositionAlignment.testFlags(Qt::AlignLeft | Qt::AlignVCenter)) {
                polygon
                    << textBoxRect.topLeft() + QPoint{mTagSize, 0}
                    << textBoxRect.topRight()
                    << textBoxRect.bottomRight()
                    << textBoxRect.bottomLeft() + QPoint{mTagSize, 0}
                    << QPoint{textBoxRect.left(), (textBoxRect.top() + textBoxRect.bottom()) / 2}
                    << textBoxRect.topLeft() + QPoint{mTagSize, 0};
            } else if (mPositionAlignment.testFlags(Qt::AlignRight | Qt::AlignVCenter)) {
                polygon
                    << textBoxRect.topLeft()
                    << textBoxRect.topRight() - QPoint{mTagSize, 0}
                    << QPoint{textBoxRect.right(), (textBoxRect.top() + textBoxRect.bottom()) / 2}
                    << textBoxRect.bottomRight() - QPoint{mTagSize, 0}
                    << textBoxRect.bottomLeft()
                    << textBoxRect.topLeft();
            } else if (mPositionAlignment.testFlags(Qt::AlignHCenter | Qt::AlignBottom)) {
                polygon
                    << textBoxRect.topLeft()
                    << textBoxRect.topRight()
                    << textBoxRect.bottomRight() - QPoint{0, mTagSize}
                    << QPoint{(textBoxRect.left() + textBoxRect.right()) / 2, textBoxRect.bottom()}
                    << textBoxRect.bottomLeft() - QPoint{0, mTagSize}
                    << textBoxRect.topLeft();
            } else if (mPositionAlignment.testFlags(Qt::AlignHCenter | Qt::AlignTop)) {
                polygon
                    << textBoxRect.topLeft() + QPoint{0, mTagSize}
                    << QPoint{(textBoxRect.left() + textBoxRect.right()) / 2, textBoxRect.top()}
                    << textBoxRect.topRight() + QPoint{0, mTagSize}
                    << textBoxRect.bottomRight()
                    << textBoxRect.bottomLeft()
                    << textBoxRect.topLeft();
            }
            painter->drawPolygon(polygon);
        }
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen{mainColor()});
        painter->drawText(textRect, Qt::TextDontClip | mTextAlignment, mText);
    }
}
