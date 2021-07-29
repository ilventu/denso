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
}

ProfilesEditor::~ProfilesEditor()
{
    delete ui;
}

void ProfilesEditor::load ( const QString &filename )
{
    QFile qFile  ( filename );

    qFile.open(QIODevice::ReadOnly);
    QByteArray sj = qFile.readAll();
    QJsonDocument doc = QJsonDocument::fromJson( sj );
    profiles.fromJson( doc.object() );
    qFile.close();

    ui->tableProfiles->setRowCount(  profiles.size() );

    QIcon okicon = QIcon::fromTheme("emblem-ok");
    QTableWidgetItem *newItem;
    for ( int i = 0; i < profiles.size(); i++ )
    {
        newItem = new QTableWidgetItem( profiles.at(i)->name );
        newItem->setIcon (okicon);
        ui->tableProfiles->setItem( i, 0, newItem);
    }
}


void ProfilesEditor::on_tableProfiles_cellChanged(int row, int column)
{
}


void ProfilesEditor::on_tableProfiles_cellActivated(int row, int column)
{

}


void ProfilesEditor::on_tableProfiles_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    pCurrent = profiles.at(currentRow);
    refresh();
}

void ProfilesEditor::refresh ()
{
    Profile *p = pCurrent;
    if ( !p )
        return;

    pCurrent = nullptr;

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

    int nRow = p->expected.size() > p->measured.size() ? p->expected.size() : p->measured.size();
    ui->tableValues->setRowCount( nRow + 1 );

    QTableWidgetItem *newItem;
    for ( unsigned int i = 0; i < p->expected.size(); i++)
    {
        seriesExpected->append(i + 1, p->expected.at(i) );
        seriesExpectedD->append(i + 1, log10 ( 100 / p->expected.at(i) ) );
        newItem = new QTableWidgetItem(tr("%1").arg(p->expected.at(i)));
        ui->tableValues->setItem( i , 0, newItem);
    }

    for ( unsigned int i = 0; i < p->measured.size(); i++)
    {
        seriesMeasured->append(i + 1, p->measured.at(i) );
        seriesMeasuredD->append(i + 1, log10 ( 100 / p->measured.at(i) ) );
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

    if (chartView)
        ui->verticalLayout->removeWidget(chartView );
    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    ui->verticalLayout->addWidget(chartView);

    pCurrent = p;
}


void ProfilesEditor::on_tableValues_cellChanged(int row, int column)
{
    if ( pCurrent )
    {
        double value = ui->tableValues->item(row, column)->text().toDouble();;
        std::vector<double> *vec;

        if ( column )
            vec = &(pCurrent->measured);
        else
            vec = &(pCurrent->expected);

        if ( value )
        {
            if ( row < (int)vec->size() )
                (*vec)[row] = value;
            else
                vec->push_back( value );
        }
        else
            if ( row < (int)vec->size() )
                vec->erase ( vec->begin() + row );

        refresh();
        QTableWidgetItem *item = ui->tableValues->item( row, column );
        ui->tableValues->setCurrentCell( row, column, QItemSelectionModel::Select );
        ui->tableValues->scrollToItem( item );
    }
}

