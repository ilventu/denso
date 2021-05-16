#include "profileseditor.h"
#include "ui_profileseditor.h"

#include <QtCharts>
#include <QLineSeries>

ProfilesEditor::ProfilesEditor(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ProfilesEditor)
{
    ui->setupUi(this);
    ui->tableValues->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    Profile *p = new Profile;

    p->name = "V850 Pro (Negative)";
    p->expected = {    0.0891,    0.1259,    0.1778,    0.2512,    0.3548,    0.5012,    0.7079,    1,
                       1.413,     1.995,     2.818,     3.981,     5.623,     7.943,     11.22,     15.85,
                       22.39,     31.62,     44.67,     63.1,      89.13    };

    p->measured = {    2.2565,    2.5935,    3.1845,    3.924,     4.739,     5.584,     6.9125,     8.336,
                       10.125,    12.46,     15.83,     19.675,    24.96,     31.935,    39.225,     48.805,
                       61.76,     76.245,    91.56,     96.32,     99.035    };

    p->measured = {    0.4515,    0.5971,    0.8838,    1.283,     1.829,     2.546,     3.892,      5.263,
                       6.541,     8.172,     10.54,     13.19,     16.71,     21.21,     26.52,      33.65,
                       41.83,     51.72,     64.82,     80,        94.22    };

    std::sort(p->expected.begin(), p->expected.end(), std::greater<double>());
    std::sort(p->measured.begin(), p->measured.end(), std::greater<double>());

    QLineSeries *seriesExpected = new QLineSeries();
    QLineSeries *seriesMeasured = new QLineSeries();
    QLineSeries *seriesExpectedD = new QLineSeries();
    QLineSeries *seriesMeasuredD = new QLineSeries();

    seriesExpected->setName( "Expected L" );
    seriesMeasured->setName( "Measured L" );
    seriesExpectedD->setName( "Expected D" );
    seriesMeasuredD->setName( "Measured D" );

    QIcon okicon = QIcon::fromTheme("emblem-ok");
    QTableWidgetItem *newItem;
    newItem = new QTableWidgetItem("V850 Pro (Positive)");
    newItem->setIcon (okicon);
    ui->tableProfiles->setRowCount( 1 );
    ui->tableProfiles->setItem( 0, 0, newItem);

    ui->tableValues->setRowCount( p->expected.size() );

    QValueAxis *axisX = new QValueAxis;
    axisX->setTickCount( p->expected.size() );
    axisX->setLabelFormat("%d");
    axisX->setRange( 1, p->expected.size() );

    QValueAxis *axisY = new QValueAxis;
    axisY->setTickCount( 11 );
    axisY->setLabelFormat("%d%%");
    axisY->setRange( 0, 100 );

    QValueAxis *axisYD = new QValueAxis;
    axisYD->setTickCount( 11 );
    axisYD->setMinorTickCount ( 1 );
    axisYD->setLabelFormat("%.2f");
    axisYD->setMax( 4 );

    for ( unsigned int i = 0; i < p->expected.size(); i++)
    {
        seriesExpected->append(i + 1, p->expected.at(i) );
        seriesMeasured->append(i + 1, p->measured.at(i) );

        seriesExpectedD->append(i + 1, log10 ( 100 / p->expected.at(i) ) );
        seriesMeasuredD->append(i + 1, log10 ( 100 / p->measured.at(i) ) );

        newItem = new QTableWidgetItem(tr("%1").arg(p->expected.at(i)));
        ui->tableValues->setItem( i , 0, newItem);
        newItem = new QTableWidgetItem(tr("%1").arg(p->measured.at(i)));
        ui->tableValues->setItem( i , 1, newItem);
    }

    QChart *chart = new QChart();
    //chart->setTheme(QChart::QChart::ChartThemeBlueIcy);
    //chart->legend()->hide();

    chart->addAxis(axisYD, Qt::AlignRight);
    chart->addAxis(axisY, Qt::AlignLeft);
    chart->addAxis(axisX, Qt::AlignBottom);

    chart->addSeries(seriesExpected);
    chart->addSeries(seriesMeasured);
    chart->addSeries(seriesExpectedD);
    chart->addSeries(seriesMeasuredD);

    seriesExpected->attachAxis(axisX);
    seriesExpected->attachAxis(axisY);
    seriesMeasured->attachAxis(axisX);
    seriesMeasured->attachAxis(axisY);
    seriesExpectedD->attachAxis(axisX);
    seriesExpectedD->attachAxis(axisYD);
    seriesMeasuredD->attachAxis(axisX);
    seriesMeasuredD->attachAxis(axisYD);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    ui->verticalLayout->addWidget(chartView);

/*
    QSettings settings ("denso", "profiles" );
    QFile qFile  ( settings.fileName() );

    p->name = "V850 Pro (Positive)";
    profiles.append( p );
    QJsonDocument doc(profiles.toJson());
    QString json ( doc.toJson ( QJsonDocument::Indented ) );
    qFile.open(QIODevice::WriteOnly);
    QTextStream out(&qFile); out << json;
    qFile.close();
*/
}

ProfilesEditor::~ProfilesEditor()
{
    delete ui;
}
