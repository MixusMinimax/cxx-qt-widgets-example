//
// Created by maxi on 8/25/26.
//

#ifndef MYAPP_GRAPHWIDGET_H
#define MYAPP_GRAPHWIDGET_H

#include <QWidget>

class GraphWidget : public QWidget {
    Q_OBJECT

public:
    explicit GraphWidget(QWidget *parent = nullptr);

    ~GraphWidget() override;
};


#endif //MYAPP_GRAPHWIDGET_H
