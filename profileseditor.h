#ifndef PROFILESEDITOR_H
#define PROFILESEDITOR_H

#include <QMainWindow>

#include <QDialog>
#include <QJsonObject>
#include <QJsonArray>
#include <QtCharts>
#include <QLineSeries>

namespace Ui {
class ProfilesEditor;
}

class Profile : public QObject
{
public:
    QString name;
    bool valid = false;

    std::vector<double> expected;
    std::vector<double> measured;

    QJsonObject toJson ()
    {
        QJsonArray jexpected, jmeasured;
        std::copy ( expected.begin(), expected.end(), std::back_inserter(jexpected));
        std::copy ( measured.begin(), measured.end(), std::back_inserter(jmeasured));

        QJsonObject jobject;
        jobject["name"] = name;
        jobject["expected"] = jexpected;
        jobject["measuerd"] = jmeasured;

        return jobject;
    }

    void fromJson ( const QJsonObject &jobject )
    {
        QJsonArray jexpected, jmeasured;

        name = jobject["name"].toString();
        jexpected = jobject["expected"].toArray();
        jmeasured = jobject["measuerd"].toArray();

        measured.empty();
        expected.empty();
        for (const auto& element : jexpected )
            expected.push_back( element.toDouble() );

        for (const auto& element : jmeasured )
            measured.push_back( element.toDouble() );
    }
};

class Profiles : public QObjectList
{
public:
    Profile *at ( int i )
    {
        return (Profile *)QObjectList::at(i);
    }

    QJsonObject toJson ()
    {
        QJsonArray jprofiles;
        for (const auto& element : *this )
            jprofiles.push_back( ((Profile *)element)->toJson( ) );

        QJsonObject jobject;

        jobject["profiles"] = jprofiles;

        return jobject;
    }

    void fromJson ( const QJsonObject &jobject )
    {
        QJsonArray jprofiles;

        jprofiles = jobject["profiles"].toArray();

        for (const auto& element : jprofiles )
        {
            Profile *p = new Profile;
            p->fromJson(element.toObject());
            push_back( p );
        }
    }
};

class ProfilesEditor : public QMainWindow
{
    Q_OBJECT

    QChartView *chartView = nullptr;
    Profile *pCurrent = nullptr;

    QString l;

    QString profilesFilename;

    void refresh ();

public:
    Profiles profiles;

    explicit ProfilesEditor(QWidget *parent = nullptr);
    ~ProfilesEditor();

    void load ( const QString &filename );
    void setL ( const QString &lpercent) { l = lpercent; };

private slots:
    void on_tableProfiles_cellChanged(int row, int column);

    void on_tableProfiles_cellActivated(int row, int column);

    void on_tableProfiles_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

    void on_tableValues_cellChanged(int row, int column);

    void on_pushButton_4_clicked();

private:
    Ui::ProfilesEditor *ui;

    void onNewProfile();
    void onSave();
};

#endif // PROFILESEDITOR_H
