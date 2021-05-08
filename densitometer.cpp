#include <vector>

using namespace std;

#include <QTimer>
#include <QMimeData>
#include <QUrl>
#include <QClipboard>
#include <QRgb>
#include <QString>
#include <QMouseEvent>
#include <QPainter>
#include <QSvgRenderer>
#include <QTextStream>
#include <QDebug>

#include "densitometer.h"
#include "ui_densitometer.h"

typedef unsigned short pixel;

#define LMAX        65535.0
#define LUT_SIZE    (LMAX + 1)

inline QImage  cvMatToQImage( const cv::Mat &inMat )
{
   switch ( inMat.type() )
   {
      // 8-bit, 4 channel
      case CV_8UC4:
      {
         QImage image( inMat.data,
                       inMat.cols, inMat.rows,
                       static_cast<int>(inMat.step),
                       QImage::Format_ARGB32 );

         return image;
      }

      // 8-bit, 3 channel
      case CV_8UC3:
      {
         QImage image( inMat.data,
                       inMat.cols, inMat.rows,
                       static_cast<int>(inMat.step),
                       QImage::Format_RGB888 );

         return image.rgbSwapped();
      }

      // 8-bit, 1 channel
      case CV_8UC1:
      {
         QImage image( inMat.data,
                       inMat.cols, inMat.rows,
                       static_cast<int>(inMat.step),
                       QImage::Format_Grayscale8 );
         return image;
      }

       // 8-bit, 1 channel
       case CV_16UC1:
       {
          QImage image( inMat.data,
                        inMat.cols, inMat.rows,
                        static_cast<int>(inMat.step),
                        QImage::Format_Grayscale16 );
          return image;
       }

      default:
         //qWarning() << "ASM::cvMatToQImage() - cv::Mat image type not handled in switch:" << inMat.type();
         break;
   }

   return QImage();
}

/* IT8
std::vector<double> expected = {
    1.18,
    6.54,
    12.22,
    15.84,
    17.94,
    22.84,
    27.33,
    31.54,
    35.84,
    40.1,
    43.31,
    47.27,
    50.95,
    54.6,
    57.89,
    61.21,
    64.7,
    68.7,
    71.55,
    74.05,
    76.89,
    80.39,
    84.49,
    95.87
}; */

/* IT8
std::vector<double> measured = {
2.50875647058823,
6.74798039215686,
9.85380392156863,
11.9078431372549,
13.4386941176471,
16.4575490196078,
20.2122823529412,
23.3870666666667,
27.305031372549,
30.6344549019608,
34.2234117647059,
37.5844392156863,
42.0169411764706,
45.3329803921569,
49.616862745098,
53.3094509803922,
57.8662745098039,
61.966862745098,
66.4063529411765,
70.022862745098,
74.2927843137255,
78.5310588235294,
83.242,
98.1937647058824
}; */

// Stouffer
std::vector<double> expected = {
    0.0891,
    0.1259,
    0.1778,
    0.2512,
    0.3548,
    0.5012,
    0.7079,
    1,
    1.413,
    1.995,
    2.818,
    3.981,
    5.623,
    7.943,
    11.22,
    15.85,
    22.39,
    31.62,
    44.67,
    63.1,
    89.13
};

/* stouffer mask
std::vector<double> measured = {
    2.47,
    2.61,
    2.83,
    3.15,
    3.64,
    4.29,
    5.2,
    5.95,
    7.01,
    8.41,
    10.5,
    12.93,
    16.24,
    20.52,
    25.73,
    33,
    41.15,
    51.4,
    65.4,
    81.9,
    97.3
};
*/

/* stouffer mask negative
*/
std::vector<double> measured = {
3.62,
3.85,
4.28,
4.86,
5.4,
6.08,
7.18,
8.38,
9.98,
12.12,
15.24,
18.68,
23.75,
29.94,
37.63,
48.15,
60.17,
75.25,
91.62,
96.51,
99.52
};

/* no mask
 *
std::vector<double> measured = {
    2.57,
    2.6,
    2.82,
    3.24,
    3.73,
    4.45,
    5.37,
    6.17,
    7.3,
    8.75,
    10.89,
    13.34,
    16.72,
    20.97,
    26.11,
    33.21,
    41.18,
    50.79,
    63.92,
    79.64,
    94.48
};
*/


Densitometer::Densitometer(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Densitometer)
{
   setWindowFlags(Qt::Window);
    ui->setupUi(this);

//    img = imread("/mnt/sata3/Fotografia/profiles/batch/v850 gamma default-BW-big.tiff", IMREAD_GRAYSCALE | CV_16UC1 );//Load image;
//    img = imread("/home/andrea/Sviluppo/build-sanetest-Desktop-Debug/stouffer-2.png", IMREAD_GRAYSCALE | CV_16UC1 );//Load image;
//    img = imread("/home/andrea/Sviluppo/build-sanetest-Desktop-Debug/sesto-2.png", IMREAD_GRAYSCALE | CV_16UC1 );//Load image;
    img = imread("/mnt/sata3/Fotografia/da sistemare/test gamma 1.0004.png", IMREAD_GRAYSCALE | CV_16UC1 );// Stouffer



    int type = img.type();
    if (  type != CV_16UC1 )
    {
        exit (-1);
    }

    aim = Mat ( radius * 2, radius * 2, CV_8UC1, Scalar(0));
    circle ( aim, Point(radius, radius), radius, Scalar(255), FILLED );

    calib = tk::spline ( measured, expected, tk::spline::cspline );
}

int startx = 0;
int starty = 0;

void Densitometer::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
            startx = event->globalX();
            starty = event->globalY();
    }
    else
    {
        // pass on other buttons to base class
        //mousePressEvent(event);
    }
}

void Densitometer::mouseMoveEvent(QMouseEvent* event)
{
    if ( rotation[nrotation] == cv::ROTATE_90_COUNTERCLOCKWISE )
    {
        posx -= ( starty - event->globalY() ) / zoom;
        posy += ( startx - event->globalX() ) / zoom;
    }
    else if ( rotation[nrotation] == cv::ROTATE_180 )
    {
        posx -= ( startx - event->globalX() ) / zoom;
        posy -= ( starty - event->globalY() ) / zoom;
    }
    else if ( rotation[nrotation] == cv::ROTATE_90_CLOCKWISE )
    {
        posx += ( starty - event->globalY() ) / zoom;
        posy -= ( startx - event->globalX() ) / zoom;
    }
    else
    {
        posx += ( startx - event->globalX() ) / zoom;
        posy += ( starty - event->globalY() ) / zoom;
    }

    if ( posx < -(img.size().width / 2 ) )
         posx = -(img.size().width / 2 );

    if ( posy < -(img.size().height / 2) )
         posy = -(img.size().height / 2 );

    if ( posx >= (img.size().width / 2))
        posx = (img.size().width / 2) -1;

    if ( posy >= (img.size().height / 2))
        posy = (img.size().height / 2) - 1;

    startx = event->globalX();
    starty = event->globalY();
    draw( posx, posy, ui->image->width(), ui->image->height() );
}

void Densitometer::wheelEvent(QWheelEvent * event)
{
    double zooms [] = { 0.125, .25, .5, 1, 2, 4, 10, 0 };

    const int degrees = event->delta() / 8;
    qDebug() << degrees;

    if ( degrees > 0)
        nzoom ++;
    else
        nzoom --;

    if ( nzoom < 0 )
        nzoom = 0;

    if ( zooms[nzoom] == 0 )
        nzoom--;

    zoom = zooms[nzoom];

    draw( posx, posy, ui->image->width(), ui->image->height() );
}

void Densitometer::keyPressEvent(QKeyEvent *event)
{
    if ( event->key() == Qt::Key_Space )
        nlight ++;
    else if ( event->key() == Qt::Key_Left )
        nrotation ++;
    else if ( event->key() == Qt::Key_Right )
           nrotation --;

    if ( light[nlight] == -1 )
        nlight = 0;

    if ( nrotation < 0 )
        nrotation = 3;

    if ( nrotation > 3 )
        nrotation = 0;

    draw( posx, posy, ui->image->width(), ui->image->height() );
}

void Densitometer::draw( int x, int y, int w, int h )
{
    Point centerImg;
    Point roiTL ( 0, 0 );
    Point roiBR;
    centerImg = Point ( x, y );

    if ( rotation[nrotation] == cv::ROTATE_90_COUNTERCLOCKWISE ||
         rotation[nrotation] == cv::ROTATE_90_CLOCKWISE )
        roiBR = Point ( h, w );
    else
        roiBR = Point ( w, h );

    Mat draw ( roiBR.y, roiBR.x, CV_16UC1, Scalar(50000) );

    Point center ( roiBR.x / 2, roiBR.y / 2 );

    Point imgTL ( img.size().width / 2 + centerImg.x - center.x / zoom, img.size().height / 2 + centerImg.y - center.y / zoom );
    Point imgBR ( img.size().width / 2 + centerImg.x + center.x / zoom, img.size().height / 2 + centerImg.y + center.y / zoom );

    if ( imgTL.x < 0 )
    {
        roiTL.x = -imgTL.x * zoom;
        imgTL.x = 0;
    }

    if ( imgTL.y < 0 )
    {
        roiTL.y = -imgTL.y * zoom;
        imgTL.y = 0;
    }

    if ( imgBR.x > img.size().width )
    {
        roiBR.x -= ( imgBR.x - img.size().width ) * zoom;
        imgBR.x = img.size().width;
    }

    if ( imgBR.y > img.size().height )
    {
        roiBR.y -= ( imgBR.y - img.size().height ) * zoom;
        imgBR.y = img.size().height;
    }

    Rect imgRect ( imgTL, imgBR );
    Rect roiRect ( roiTL, roiBR );

    Mat cropMat ( img ( imgRect ) );
    Mat resizeMat;
    cv::resize( cropMat, resizeMat, cv::Size( roiRect.width, roiRect.height ), 0, 0, INTER_LINEAR_EXACT);

//imshow("cropMat", cropMat );

    Mat roiMat = draw ( roiRect );
    resizeMat.copyTo( roiMat );

//imshow("roiMat", roiMat );
//imshow("draw", draw );

    int radius = aim.size().width / 2;
    Point centerbox ( draw.size().width / 2 - radius, draw.size().height / 2 - radius );
    Point pt;
    float tot = 0;
    int n = 0;
    for (int i = 0; i < aim.rows; ++i)
    {
      for (int j = 0; j < aim.cols; ++j)
      {
        if ( aim.at<unsigned char>(i, j) ) //== Vec3b(255,255,255) )
        {
            pt.x = centerbox.x + i;
            pt.y = centerbox.y + j;
            pixel p = draw.at<pixel>( pt );

            tot += p;
            n++;

            p = LMAX * pow ( p / LMAX, 1 / 1.8 );

            draw.at<pixel>( pt ) = p;
        }
      }
    }

    QString d;

    double l = (( tot / n ) / LMAX ) * 100;
    double lc = calib (l);
    if (tot)
        QTextStream(&d) << "D=" << round ( log10( (100 / lc) ) * 100 ) / 100 << " l=" << round(lc*100)/100 << "% (" << round(l*100)/100 <<"%) " << zoom <<"x";
    else
        d = "-";

    ui->top->setText( d );

    Mat draw8;
    draw.convertTo(draw8, CV_8UC1, 1 / 256.0 * ( ( 1 + light[nlight] ) / 2 ) );

    cv::Mat lookUpTable(1, 256, CV_8UC1);

    for( int i = 0; i < 256; ++i)
        lookUpTable.at<unsigned char>(0,i) = 2.55 * calib ( i / 2.55 );

    cv::LUT( draw8, lookUpTable, draw8 );

    for( int i = 0; i < 256; ++i)
        lookUpTable.at<unsigned char>(0,i) = 255 * pow ( i / 255.0, 1 / light[nlight] );;

    cv::LUT( draw8, lookUpTable, draw8 );

    if ( rotation[nrotation] != -1 )
        cv::rotate( draw8, draw8, rotation[nrotation]);

    QImage imDraw = cvMatToQImage( draw8 );

    QSvgRenderer renderer( QString ( ":/images/mirino.svg" ) );
    QRectF b = renderer.viewBoxF();
    double ratio = b.height()/b.width();
    int width = 400;
    int height = width * ratio;

    // Get QPainter that paints to the image
    QPainter painter(&imDraw);
    renderer.render(&painter, QRect( w / 2 - height - (181), h / 2 - (height / 2) + 1, width, height ));

    QPixmap px = QPixmap::fromImage ( imDraw );
    ui->image->setPixmap ( px );
}

void Densitometer::resizeEvent(QResizeEvent*)
{
    draw( posx, posy, ui->image->width(), ui->image->height() );
}

Densitometer::~Densitometer()
{
    delete ui;
}

