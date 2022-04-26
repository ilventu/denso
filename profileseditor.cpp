#include "profileseditor.h"
#include "ui_profileseditor.h"

#include <algorithm>

#include <QtCharts>
#include <QLineSeries>

std::vector<double> expected = {
0,
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


ProfilesEditor::ProfilesEditor(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ProfilesEditor)
{
    ui->setupUi(this);
    ui->tableValues->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(ui->actionNew_profile, &QAction::triggered, [=]() {
      onNewProfile();
    });

    connect(ui->actionSave, &QAction::triggered, [=]() {
      onSave();
    });

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

        std::sort(profiles.at(i)->expected.begin(), profiles.at(i)->expected.end(), std::greater<double>());
        std::sort(profiles.at(i)->measured.begin(), profiles.at(i)->measured.end(), std::greater<double>());
    }

    profilesFilename = filename;
}

void ProfilesEditor::onNewProfile()
{
    Profile *newProfile = new Profile;
    newProfile->expected = expected;
    std::sort(newProfile->expected.begin(), newProfile->expected.end(), std::greater<double>());
    newProfile->name = "New";

    profiles.push_back(newProfile);

    QIcon okicon = QIcon::fromTheme("emblem-ok");
    QTableWidgetItem *newItem;
    ui->tableProfiles->insertRow ( ui->tableProfiles->rowCount() );
    newItem = new QTableWidgetItem( newProfile->name );
    newItem->setIcon (okicon);
    ui->tableProfiles->setItem( ui->tableProfiles->rowCount() - 1, 0, newItem);
}

void ProfilesEditor::onSave()
{
    QFile qFile  ( profilesFilename );

    qFile.open(QIODevice::WriteOnly);
    QJsonDocument doc;
    doc.setObject(  profiles.toJson() );
    QByteArray sj = doc.toJson(QJsonDocument::Indented).toStdString().c_str();
    qFile.write( sj.data(), sj.size() );
    qFile.close();
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

    ui->tableValues->blockSignals(true);
    unsigned int nRow = pCurrent->expected.size() > pCurrent->measured.size() ? pCurrent->expected.size() : pCurrent->measured.size() - 1;
    ui->tableValues->setRowCount( 0 );
    ui->tableValues->setRowCount( nRow );

    for ( unsigned int i = 0; i < nRow; i++)
    {
        if ( i < pCurrent->expected.size() )
            ui->tableValues->setItem( i , 0, new QTableWidgetItem(tr("%1").arg(pCurrent->expected.at(i))) );
        else
            ui->tableValues->setItem( i , 0, new QTableWidgetItem( "0" ) );

        if ( i < pCurrent->measured.size() )
            ui->tableValues->setItem( i , 1, new QTableWidgetItem(tr("%1").arg(pCurrent->measured.at(i))) );
        else
            ui->tableValues->setItem( i , 1, new QTableWidgetItem( "0" ) );
    }

    ui->tableValues->blockSignals(false);
    refresh();
}

void ProfilesEditor::refresh ()
{
    Profile *p = pCurrent;
    if ( !p )
        return;

    pCurrent = nullptr;

    unsigned int size = p->expected.size() - 1;

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
    axisX->setTickCount( size  );
    axisX->setLabelFormat("%d");
    axisX->setRange( 1, size );

    QValueAxis *axisY = new QValueAxis;
    axisY->setTickCount( 11 );
    axisY->setLabelFormat("%d%%");
    axisY->setRange( 0, 100 );

    QValueAxis *axisYD = new QValueAxis;
    axisYD->setTickCount( 11 );
    axisYD->setMinorTickCount ( 1 );
    axisYD->setLabelFormat("%.2f");
    axisYD->setMax( 4 );

    for ( unsigned int i = 0; i < size; i++)
    {
        seriesExpected->append(i + 1, p->expected.at(i) );
        seriesExpectedD->append(i + 1, log10 ( 100 / p->expected.at(i) ) );
    }

    for ( unsigned int i = 0; i < std::min(size, (unsigned int)p->measured.size()); i++)
    {
        seriesMeasured->append(i + 1, p->measured.at(i) );
        seriesMeasuredD->append(i + 1, log10 ( 100 / p->measured.at(i) ) );
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
    int rows = ui->tableValues->rowCount();

    pCurrent->expected.clear();
    pCurrent->measured.clear();

    QTableWidgetItem *item;

    for ( int i = 0; i < rows; i++ )
    {
        item = ui->tableValues->item ( i, 0 );
        if ( item )
            pCurrent->expected.push_back( item->text().toDouble() );

        item = ui->tableValues->item ( i, 1 );
        if ( item )
            pCurrent->measured.push_back( item->text().toDouble() );
    }

    refresh();
}


void ProfilesEditor::on_pushButton_4_clicked()
{
    QModelIndexList indexList = ui->tableValues->selectionModel()->selectedIndexes();
    int row;
    QTableWidgetItem *newItem;
    foreach (QModelIndex index, indexList)
    {
        row = index.row();
        newItem = new QTableWidgetItem(l);
        ui->tableValues->setItem( row , 1, newItem);

        QTableWidgetItem *item = ui->tableValues->item( row + 1, 1 );
        ui->tableValues->setCurrentCell( row + 1, 1, QItemSelectionModel::ClearAndSelect );
        ui->tableValues->scrollToItem( item );
    }
}

