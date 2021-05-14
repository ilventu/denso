#include "denso.h"
#include "ui_denso.h"

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
#include <QFontDatabase>
#include <QSettings>

typedef unsigned short pixel;

#define LMAX        65535.0
#define LUT_SIZE    (LMAX + 1)

int rotations [] = { -1, cv::ROTATE_90_COUNTERCLOCKWISE, cv::ROTATE_180, cv::ROTATE_90_CLOCKWISE };
double zooms [] = { 0.125, .25, .5, 1, 2, 4, 10 };
double lights [] = { 1, 1.5, 2, 3, 4 };

#define MAX_ROTATIONS   (int)(sizeof(rotations)/sizeof(int))-1
#define MAX_ZOOMS       (int)(sizeof(zooms)/sizeof(double))-1
#define MAX_LIGHTS      (int)(sizeof(lights)/sizeof(double))-1

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

std::vector<double> measured = {
2.2565,
2.5935,
3.1845,
3.924,
4.739,
5.584,
6.9125,
8.336,
10.125,
12.46,
15.83,
19.675,
24.96,
31.935,
39.225,
48.805,
61.76,
76.245,
91.56,
96.32,
99.035
};

std::vector<double> measured_pos = {
0.4515,
0.5971,
0.8838,
1.283,
1.829,
2.546,
3.892,
5.263,
6.541,
8.172,
10.54,
13.19,
16.71,
21.21,
26.52,
33.65,
41.83,
51.72,
64.82,
80,
94.22
};

Denso::Denso(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Denso)
{
    ui->setupUi(this);

    setAcceptDrops(true);

    aim = Mat ( radius * 2, radius * 2, CV_8UC1, Scalar(0));
    circle ( aim, Point(radius, radius), radius, Scalar(255), FILLED );

    calib = tk::spline ( measured, expected, tk::spline::cspline );

    int id = QFontDatabase::addApplicationFont(":/fonts/Digital7Mono-B1g5.ttf");
    digitFont = QFontDatabase::applicationFontFamilies(id).at(0);

    QLabel *label = new QLabel("Profile:");
    ui->toolProfile->insertWidget(ui->actionEdit_profiles, label );
    ui->toolProfile->insertWidget(ui->actionEdit_profiles, &profileCombo);

    connect( &profileCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onChangeProfile(int)));

    profileCombo.setFocusPolicy(Qt::NoFocus);
    profileCombo.addItem( "Image raw data" /* , const QVariant &userData = QVariant()) */ );
    profileCombo.addItem( "Negative (Stouffer T2115)" /* , const QVariant &userData = QVariant()) */ );
    profileCombo.addItem( "Positive (Stouffer T2115)" /* , const QVariant &userData = QVariant()) */ );

    wndProfiles = new ProfilesEditor (this);
    readSettings();
    readFile( fileName );
}

Denso::~Denso()
{
    delete ui;
}

void Denso::showEvent( QShowEvent* event )
{
    draw( );
    QMainWindow::showEvent( event );
}

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

void Denso::closeEvent(QCloseEvent *event)
{
    QSettings settings("denso", "status" );
    settings.setValue("geometry", saveGeometry());
    settings.setValue("geometry_profiles", wndProfiles->saveGeometry() );
    settings.setValue("windowState", saveState());
    settings.setValue("nzoom", nzoom );
    settings.setValue("nlight", nlight );
    settings.setValue("posx", posx );
    settings.setValue("posy", posy );
    settings.setValue("lastFile", fileName );
    settings.setValue("nrotation", nrotation );
    settings.setValue("lastProfile", profileCombo.currentIndex() );

    QMainWindow::closeEvent(event);
}

void Denso::readSettings()
{
    QSettings settings("denso", "status" );
    restoreGeometry(settings.value("geometry").toByteArray());
    wndProfiles->restoreGeometry(settings.value("geometry_profiles").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    fileName = settings.value("lastFile").toString();
    setZoom ( settings.value("nzoom", 3).toInt() );
    setLight( settings.value("nlight", 1).toInt() );
    posx = settings.value("posx").toInt();
    posy = settings.value("posy").toInt();
    setRotation ( settings.value("nrotation", 1).toInt() );
    profileCombo.setCurrentIndex ( settings.value("lastProfile").toInt() );
}

void Denso::onChangeProfile (int index)
{
    std::vector<double> defaultvals = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100  };

    if ( index == 2 )
        calib = tk::spline ( measured_pos, expected, tk::spline::cspline );
    else if ( index == 1 )
        calib = tk::spline ( measured, expected, tk::spline::cspline );
    else
        calib = tk::spline ( defaultvals, defaultvals, tk::spline::cspline );

    draw( );
}

void Denso::dragEnterEvent(QDragEnterEvent *event)
{
//    if ( event->mimeData()->hasFormat("image/*") || event->mimeData()->hasFormat("image/tiff") )
        event->acceptProposedAction();
}

void Denso::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();

    // check for our needed mime type, here a file or a list of files
    if (mimeData->hasUrls())
    {
        QList<QUrl> urlList = mimeData->urls();

        // extract the local paths of the files
        if ( urlList.size() )
        {
              readFile( urlList.at(0).toLocalFile() );
              event->acceptProposedAction();
        }
    }
}

void Denso::readFile( const QString &file )
{
    Mat newimg = imread ( file.toStdString(), IMREAD_GRAYSCALE | CV_16UC1 );
    if ( !newimg.empty() && newimg.type() == CV_16UC1 )
    {
        img = newimg;
        fileName = file;
        draw( );
    }
}

int startx = 0;
int starty = 0;

void Denso::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
            startx = event->globalX();
            starty = event->globalY();
            setCursor(Qt::DragMoveCursor);
    }
    else
    {
        // pass on other buttons to base class
        //mousePressEvent(event);
    }
}


void Denso::mouseReleaseEvent(QMouseEvent *event)
{
    setCursor(Qt::ArrowCursor);
}

void Denso::mouseMoveEvent(QMouseEvent* event)
{
    double zoom = zooms[nzoom];

    if ( rotations[nrotation] == cv::ROTATE_90_COUNTERCLOCKWISE )
    {
        posx -= ( starty - event->globalY() ) / zoom;
        posy += ( startx - event->globalX() ) / zoom;
    }
    else if ( rotations[nrotation] == cv::ROTATE_180 )
    {
        posx -= ( startx - event->globalX() ) / zoom;
        posy -= ( starty - event->globalY() ) / zoom;
    }
    else if ( rotations[nrotation] == cv::ROTATE_90_CLOCKWISE )
    {
        posx += ( starty - event->globalY() ) / zoom;
        posy -= ( startx - event->globalX() ) / zoom;
    }
    else
    {
        posx += ( startx - event->globalX() ) / zoom;
        posy += ( starty - event->globalY() ) / zoom;
    }

    startx = event->globalX();
    starty = event->globalY();

    draw( );
}

void Denso::wheelEvent(QWheelEvent * event)
{
    QPoint degrees = event->angleDelta() / 8;

    if ( degrees.y() > 0)
        on_actionZoom_in_triggered ();
    else
        on_actionZoom_out_triggered();
}

void Denso::draw( )
{
    if ( !ui->image->isVisible() )
        return;

    if ( img.empty() )
    {
        ui->image->setPixmap ( QPixmap() );
        return;
    }

    if ( posx < -(img.size().width / 2 ) )
         posx = -(img.size().width / 2 );

    if ( posy < -(img.size().height / 2) )
         posy = -(img.size().height / 2 );

    if ( posx >= (img.size().width / 2))
        posx = (img.size().width / 2) -1;

    if ( posy >= (img.size().height / 2))
        posy = (img.size().height / 2) - 1;

    int x = posx;
    int y = posy;
    int w = ui->image->width();
    int h = ui->image->height();

    int n = 0;
    double tot = 0;
    double totc = 0;
//    double totd = 0;
    double zoom = zooms[nzoom];

    Mat draw8;
    {
        Point centerImg;
        Point roiTL ( 0, 0 );
        Point roiBR;
        centerImg = Point ( x, y );

        if ( rotations[nrotation] == cv::ROTATE_90_COUNTERCLOCKWISE ||
             rotations[nrotation] == cv::ROTATE_90_CLOCKWISE )
            roiBR = Point ( h, w );
        else
            roiBR = Point ( w, h );

        Mat draw ( roiBR.y, roiBR.x, CV_16UC1, Scalar(LMAX * .98 ) );

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
        double lc;

        for (int i = 0; i < aim.rows; ++i)
        {
          for (int j = 0; j < aim.cols; ++j)
          {
            if ( aim.at<unsigned char>(i, j) ) //== Vec3b(255,255,255) )
            {
                pt.x = centerbox.x + i;
                pt.y = centerbox.y + j;
                pixel p = draw.at<pixel>( pt );
                lc = calib( p / LMAX * 100 );

                tot += p / LMAX * 100;
                totc += lc;
//                totd += log10 ( 100 / lc );
                n++;

                p = LMAX * pow ( p / LMAX, 1 / 1.8 );

                draw.at<pixel>( pt ) = p;
            }
          }
        }

        draw.convertTo(draw8, CV_8UC1, 1 / 256.0 ); // * ( ( 1 + lights[nlight] ) / 2 ) );
    }

    cv::Mat lookUpTable(1, 256, CV_8UC1);

    for( int i = 0; i < 256; ++i)
        lookUpTable.at<unsigned char>(0,i) = 255 * pow ( i / 255.0, 1 / lights[nlight] );

    cv::LUT( draw8, lookUpTable, draw8 );

    for( int i = 0; i < 256; ++i)
        lookUpTable.at<unsigned char>(0,i) = 2.55 * calib ( i / 2.55 );

    cv::LUT( draw8, lookUpTable, draw8 );

    if ( rotations[nrotation] != -1 )
        cv::rotate( draw8, draw8, rotations[nrotation]);

    cvtColor(draw8, draw8, cv::COLOR_GRAY2BGRA);
    QImage imDraw = cvMatToQImage( draw8 );

    QSvgRenderer renderer( QString ( ":/images/mirino.svg" ) );
    QRectF b = renderer.viewBoxF();
    double ratio = b.height()/b.width();
    int width = 400;
    int height = width * ratio;

    // Get QPainter that paints to the image
    QPainter painter(&imDraw);
    painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing );

    QRect mirino ( w / 2 - width + 70, h / 2 - (height / 2) + 1, width, height );
    QPoint pointD ( mirino.left() + 105, mirino.top() + 125 );
    QPoint pointL ( mirino.left() + 107, mirino.top() + 145 );
    QPoint pointZ ( mirino.left() + 173, mirino.top() + 145 );

    renderer.render(&painter, mirino);

    double density = log10 ( 100 / ( totc / n ) );

    QString s;

    painter.setPen( QColor ( 0xd2, 0x3d, 0x17, .65 * 255 ) );
    painter.setFont( QFont (digitFont, 54 ));
    s = QString::number(round ( density * 100 ) / 100, 'f', 2); // s.sprintf("%0.2f", density );
    DrawNumber ( painter, pointD, s );

    painter.setPen( QColor ( 0xba, 0xb0, 0x13, .65 * 255 ) );
    painter.setFont( QFont (digitFont, 14 ));
    s = QString::number(totc / n, 'f', 4); // s.sprintf("%0.4f", totc / n );
    if ( s.length() < 6 ) s = "_" + s;

    if ( s.at(0) == '0' )
        s = s.mid(1);

    s = s.left(5);

    QString s2;
    for ( int i = 0; i < s.length(); i ++  )
        if ( s.at(i).isDigit() && ( !i || s.at(i-1) != '.'  )  )
            s2 += QString ("~") + s.at(i);
        else
            s2 += s.at(i);

    s2 = s2 + "%";

    DrawNumber ( painter, pointL, s2 );

    if ( zoom != 1 )
    {
        painter.setPen( QColor ( 0xba, 0xa9, 0xa9, .65 * 255 ) );
        painter.setFont( QFont (digitFont, 14 ));
        if ( zoom >= 10 )
            s = "~_~" + QString::number( (int) zoom ) + "x"; // s.sprintf(" %dx", (int) zoom );
        else if ( zoom > 1 )
            s = "~_~_" + QString::number( (int) zoom ) + "x"; // s.sprintf("  %dx", (int) zoom );
        else if ( zoom == .125 )
            s = ".1~25x";
        else
            s = "~_." + QString::number( (int) (zoom * 100) ) + "x"; //             s.sprintf(" .%dx", (int) (zoom * 100) );

        DrawNumber ( painter, pointZ, s );
    }
    else
        s = "~_~__X";

    DrawNumber ( painter, pointZ, s );

    QPixmap px = QPixmap::fromImage ( imDraw );
    ui->image->setPixmap ( px );
}

void Denso::DrawNumber ( QPainter &painter, QPoint point, const QString &text )
{
    QPen penFG = painter.pen();
    QPen penBG = QColor ( 0x00, 0x00, 0x00, .20 * 255 );
    int dotl = QFontMetrics(painter.font()).size(Qt::TextSingleLine, ".").width() / 3;
    int chrl = QFontMetrics(painter.font()).size(Qt::TextSingleLine, "8").width();

    for ( int i = 0; i < text.length(); i++ )
    {
        QChar c = text.at (i);
        if ( c == '.' )
        {
            painter.setPen( penFG );
            point.setX( point.x() - dotl );
            painter.drawText( point, c );
            point.setX( point.x() + dotl * 2 );
        } else if ( c == '~' )
        {
            painter.setPen( penBG );
            point.setX( point.x() - dotl );
            painter.drawText( point, "." );
            point.setX( point.x() + dotl * 2 );
        } else if ( c == '_' )
        {
            painter.setPen( penBG );
            painter.drawText( point, "8" );
            point.setX( point.x() + chrl );
        } else if ( c == 'X' )
        {
            painter.setPen( penBG );
            painter.drawText( point, "x" );
            point.setX( point.x() + chrl );
        } else if ( c.isDigit() )
        {
            painter.setPen( penBG );
            painter.drawText( point, "8" );
            painter.setPen( penFG );
            painter.drawText( point, c );
            point.setX( point.x() + chrl );
        }
        else
        {
            painter.setPen( penFG );
            painter.drawText( point, c );
            point.setX( point.x() + chrl );
        }
    }
}

void Denso::resizeEvent(QResizeEvent*evt)
{
    draw( );
    QMainWindow::resizeEvent(evt);
}

void Denso::setRotation ( int n )
{
    if ( n < 0 )
        n = MAX_ROTATIONS;

    if ( n > MAX_ROTATIONS )
        n = 0;

    nrotation = n;
}

void Denso::setLight ( int n )
{
    if ( n < 0 )
        n = 0;

    if ( n > MAX_LIGHTS )
        n = 0;

    nlight = n;
}

void Denso::setZoom ( int n )
{
    if ( n < 0 )
        n = 0;

    if ( n > MAX_ZOOMS )
        n = MAX_ZOOMS;

    nzoom = n;
}

void Denso::on_actionBacklight_triggered()
{
    setLight ( nlight + 1 );
    draw( );
}

void Denso::on_actionRotate_left_triggered()
{
    setRotation ( nrotation + 1 );
    draw( );
}

void Denso::on_actionRotate_right_triggered()
{
    setRotation ( nrotation - 1 );
    draw( );
}

void Denso::on_actionZoom_in_triggered()
{
    setZoom ( nzoom + 1 );
    draw( );
}

void Denso::on_actionZoom_out_triggered()
{
    setZoom ( nzoom - 1 );
    draw( );
}

void Denso::keyPressEvent(QKeyEvent *event)
{
    QMainWindow::keyPressEvent( event );
}

void Denso::on_actionEdit_profiles_triggered()
{
    wndProfiles->show();
}
