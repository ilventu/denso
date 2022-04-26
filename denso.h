#ifndef DENSO_H
#define DENSO_H

#include <QMainWindow>
#include <QComboBox>

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>

#include <vector>
#include "spline.h"

#include "profileseditor.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Denso; }
QT_END_NAMESPACE

using namespace cv;
using namespace std;

class Denso : public QMainWindow
{
    Q_OBJECT

public:
    Denso(QWidget *parent = nullptr);
    ~Denso();

private:
    QString fileName;
    Mat img;
    double img_min, img_max;

    double posx = 0;
    double posy = 0;

    int nrotation = 1;
    int nzoom = 3;
    int nlight = 1;
    bool positive = false;

    void setRotation ( int n );
    void setZoom ( int n );
    void setLight ( int n );

    double radius = 18.5;
    Mat aim;

    QString digitFont;

    bool bCalib;
    tk::spline calib;

    QComboBox profileCombo;
    ProfilesEditor *wndProfiles;

    void showEvent( QShowEvent* event );
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent*);
    void mouseMoveEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent * event);
    void keyPressEvent(QKeyEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

    void readFile( const QString &file );
    void readFile( const QStringList &files );
    void draw( );
    void DrawNumber ( QPainter &painter, QPoint point, const QString &text );

    void closeEvent(QCloseEvent *event);
    void readSettings();

private slots:
    void onChangeProfile (int index);

    void on_actionBacklight_triggered();

    void on_actionRotate_left_triggered();

    void on_actionRotate_right_triggered();

    void on_actionZoom_in_triggered();

    void on_actionZoom_out_triggered();

    void on_actionEdit_profiles_triggered();

private:
    Ui::Denso *ui;
};

#endif // DENSO_H
