#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QSqlDatabase>

struct SatelliteData {
    int noradId;           // NORAD ID
    QString name;          // Имя спутника
    QString country;       // Страна
    QString launchDate;    // Дата запуска (может быть пустой)
    // Добавим остальные поля по мере необходимости
};

class QNetworkReply;
class QNetworkAccessManager;
// class QSqlDatabase;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_fetch_clicked();
    void onReplyFinished(QNetworkReply *reply);

    void on_pushButton_fetch_tle_clicked();

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;
    QSqlDatabase db;
    QSqlDatabase dbTle;       // новая БД для TLE

    void setupDatabase();
    void setupTleDatabase();  // новая
    void saveToDatabase(const QStringList &headers, const QList<QStringList> &data);
    void saveTleToDatabase(const QStringList &headers, const QList<QStringList> &data); // новая
    void loadFromDatabase();
    void loadTleFromDatabase(); // новая
};
#endif // MAINWINDOW_H
