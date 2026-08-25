//
// Created by maxi on 8/25/26.
//

#include "GraphWidget.h"

#include <QPushButton>

#include <qcustomplot.h>

GraphWidget::GraphWidget(QWidget *parent) : QWidget(parent) {
    const auto button = new QPushButton(this);
    button->setText(tr("Hello, World!"));
}

GraphWidget::~GraphWidget() = default;
