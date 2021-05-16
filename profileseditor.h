#ifndef PROFILESEDITOR_H
#define PROFILESEDITOR_H

#include <QMainWindow>

#include <QDialog>
#include <QJsonObject>
#include <QJsonArray>

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

public:
    explicit ProfilesEditor(QWidget *parent = nullptr);
    ~ProfilesEditor();

    Profiles profiles;

private:
    Ui::ProfilesEditor *ui;
};

#endif // PROFILESEDITOR_H
