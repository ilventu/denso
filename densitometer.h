#ifndef DENSITOMETER_H
#define DENSITOMETER_H

#include <QDialog>

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>

#include "spline.h"

using namespace cv;

QT_BEGIN_NAMESPACE
namespace Ui { class Densitometer; }
QT_END_NAMESPACE

class Densitometer : public QDialog
{
    Q_OBJECT

    Mat img;
    int posx = 0;
    int posy = 0;

    int nzoom = 0;
    double zoom = 1;

    Mat aim;

    tk::spline calib;

    void mousePressEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent*);
    void mouseMoveEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent * event);

    void draw( int x, int y, int w, int h );

public:
    Densitometer(QWidget *parent = nullptr);
    ~Densitometer();

private:
    Ui::Densitometer *ui;
};
#endif // DENSITOMETER_H
