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

#include <vector>

using namespace std;

class Densitometer : public QDialog
{
    Q_OBJECT

    Mat img;
    int posx = 0;
    int posy = 0;

    int nrotation = 1;
    int rotation [5] = { -1, cv::ROTATE_90_COUNTERCLOCKWISE, cv::ROTATE_180, cv::ROTATE_90_CLOCKWISE };

    int nzoom = 3;
    double zoom = 1;

    int nlight = 1;
    double light [5] = { 1, 1.25, 1.5, 1.75, -1 };

    int radius = 20;
    Mat aim;

    tk::spline calib;

    void mousePressEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent*);
    void mouseMoveEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent * event);
    void keyPressEvent(QKeyEvent *event);

    void draw( int x, int y, int w, int h );

public:
    Densitometer(QWidget *parent = nullptr);
    ~Densitometer();

private:
    Ui::Densitometer *ui;
};
#endif // DENSITOMETER_H
