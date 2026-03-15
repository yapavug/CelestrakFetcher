#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QTextStream>
#include <QStringList>
#include <QStandardItemModel>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    networkManager = new QNetworkAccessManager(this);
    setupDatabase();
    loadFromDatabase();
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &MainWindow::onReplyFinished);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_fetch_clicked()
{
    ui->plainTextEdit_log->appendPlainText("Начинаем загрузку...");

    // QUrl url("https://celestrak.org/NORAD/elements/gp.php?GROUP=active&FORMAT=csv");
    QUrl url("https://celestrak.org/pub/satcat.csv");
    QNetworkRequest request(url);
    networkManager->get(request);
}

QStringList parseCSVLine(const QString &line)
{
    QStringList result;
    bool inQuotes = false;
    QString current;

    for (int i = 0; i < line.length(); ++i) {
        QChar c = line[i];
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ',' && !inQuotes) {
            result << current.trimmed();
            current.clear();
        } else {
            current += c;
        }
    }
    result << current.trimmed(); // последнее поле
    return result;
}

void MainWindow::onReplyFinished(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QString data = QString::fromUtf8(reply->readAll());
        ui->plainTextEdit_log->appendPlainText("ПЕРВЫЕ 500 СИМВОЛОВ ОТВЕТА:");
        ui->plainTextEdit_log->appendPlainText(data.left(500));
        ui->plainTextEdit_log->appendPlainText("Загружено байт: " + QString::number(data.size()));

        // Парсим CSV
        QTextStream stream(&data);
        QStringList lines;
        while (!stream.atEnd()) {
            lines << stream.readLine();
        }

        if (lines.isEmpty()) {
            ui->plainTextEdit_log->appendPlainText("Файл пуст");
            return;
        }

        // Первая строка - заголовки
        QString headerLine = lines.first();
        QStringList headers = headerLine.split(',');
        lines.removeFirst(); // убираем заголовки из данных

        // Создаем модель для таблицы
        QStandardItemModel *model = new QStandardItemModel(lines.size(), headers.size(), this);
        model->setHorizontalHeaderLabels(headers);

        // Заполняем модель данными
        for (int row = 0; row < lines.size(); ++row) {
            QStringList fields = parseCSVLine(lines[row]); // нужна функция для правильного парсинга
            for (int col = 0; col < fields.size(); ++col) {
                QStandardItem *item = new QStandardItem(fields[col]);
                model->setItem(row, col, item);
            }
        }

        QList<QStringList> allData;
        for (const QString &line : lines) {
            allData << parseCSVLine(line);
        }
        saveToDatabase(headers, allData);

        loadFromDatabase();
        ui->plainTextEdit_log->appendPlainText("Таблица обновлена");
        ui->plainTextEdit_log->appendPlainText("Загружено строк: " + QString::number(lines.size()));

    } else {
        ui->plainTextEdit_log->appendPlainText("Ошибка: " + reply->errorString());
    }
    reply->deleteLater();
}

void MainWindow::setupDatabase()
{
    // Создаем папку data если её нет
    QDir dir;
    if (!dir.exists("data")) {
        dir.mkdir("data");
    }

    // Подключаемся к БД
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("data/satellites.db");

    if (!db.open()) {
        ui->plainTextEdit_log->appendPlainText("Ошибка открытия БД: " + db.lastError().text());
        return;
    }

    // Создаем таблицу, если её нет
    QSqlQuery query;
    QString createTable = "CREATE TABLE IF NOT EXISTS satellites ("
                          "norad_id INTEGER PRIMARY KEY,"
                          "name TEXT,"
                          "country TEXT,"
                          "launch_date TEXT,"
                          "raw_data TEXT,"
                          "last_updated DATETIME DEFAULT CURRENT_TIMESTAMP)";

    if (!query.exec(createTable)) {
        ui->plainTextEdit_log->appendPlainText("Ошибка создания таблицы: " + query.lastError().text());
    } else {
        ui->plainTextEdit_log->appendPlainText("База данных готова");
    }
}

// void MainWindow::saveToDatabase(const QStringList &headers, const QList<QStringList> &data)
// {
//     if (!db.isOpen()) {
//         ui->plainTextEdit_log->appendPlainText("БД не открыта");
//         return;
//     }

//     // Находим индексы нужных колонок
//     int noradIdx = headers.indexOf("NORAD_CAT_ID");
//     int nameIdx = headers.indexOf("OBJECT_NAME");
//     int countryIdx = headers.indexOf("COUNTRY");
//     int launchIdx = headers.indexOf("LAUNCH_DATE");

//     if (noradIdx == -1) {
//         ui->plainTextEdit_log->appendPlainText("Не найдена колонка NORAD_CAT_ID");
//         return;
//     }

//     db.transaction(); // Начинаем транзакцию для скорости

//     QSqlQuery query;
//     query.prepare("INSERT OR REPLACE INTO satellites (norad_id, name, country, launch_date, raw_data) "
//                   "VALUES (?, ?, ?, ?, ?)");

//     for (const QStringList &row : data) {
//         if (row.size() <= noradIdx) continue;

//         bool ok;
//         int noradId = row[noradIdx].toInt(&ok);
//         if (!ok) continue;

//         QString name = (nameIdx != -1 && nameIdx < row.size()) ? row[nameIdx] : "";
//         QString country = (countryIdx != -1 && countryIdx < row.size()) ? row[countryIdx] : "";
//         QString launchDate = (launchIdx != -1 && launchIdx < row.size()) ? row[launchIdx] : "";
//         QString rawData = row.join(",");

//         int noradIdx = headers.indexOf("NORAD_CAT_ID");    // остается
//         int nameIdx = headers.indexOf("OBJECT_NAME");       // было OBJECT_NAME
//         int countryIdx = headers.indexOf("OWNER");          // ВМЕСТО COUNTRY
//         int launchIdx = headers.indexOf("LAUNCH_DATE");     // остается
//         query.addBindValue(rawData);

//         if (!query.exec()) {
//             ui->plainTextEdit_log->appendPlainText("Ошибка вставки: " + query.lastError().text());
//         }
//     }

//     db.commit(); // Подтверждаем транзакцию
//     ui->plainTextEdit_log->appendPlainText("Сохранено записей: " + QString::number(data.size()));
// }

void MainWindow::saveToDatabase(const QStringList &headers, const QList<QStringList> &data)
{
    if (!db.isOpen()) {
        ui->plainTextEdit_log->appendPlainText("БД не открыта");
        return;
    }

    // Находим индексы нужных колонок (для нового файла satcat.csv)
    int noradIdx = headers.indexOf("NORAD_CAT_ID");
    int nameIdx = headers.indexOf("OBJECT_NAME");
    int ownerIdx = headers.indexOf("OWNER");           // вместо COUNTRY
    int launchIdx = headers.indexOf("LAUNCH_DATE");

    // Для отладки - выведи индексы
    ui->plainTextEdit_log->appendPlainText(QString("Индексы: NORAD=%1, Name=%2, Owner=%3, Launch=%4")
                                               .arg(noradIdx).arg(nameIdx).arg(ownerIdx).arg(launchIdx));

    if (noradIdx == -1) {
        ui->plainTextEdit_log->appendPlainText("Не найдена колонка NORAD_CAT_ID");
        return;
    }

    db.transaction();

    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO satellites (norad_id, name, country, launch_date, raw_data) "
                  "VALUES (?, ?, ?, ?, ?)");

    int successCount = 0;
    for (const QStringList &row : data) {
        if (row.size() <= noradIdx) continue;

        bool ok;
        int noradId = row[noradIdx].toInt(&ok);
        if (!ok) continue;

        QString name = (nameIdx != -1 && nameIdx < row.size()) ? row[nameIdx] : "";
        QString owner = (ownerIdx != -1 && ownerIdx < row.size()) ? row[ownerIdx] : "";  // owner
        QString launchDate = (launchIdx != -1 && launchIdx < row.size()) ? row[launchIdx] : "";
        QString rawData = row.join(",");

        query.addBindValue(noradId);
        query.addBindValue(name);
        query.addBindValue(owner);      // добавляем owner (в таблице поле называется country)
        query.addBindValue(launchDate);
        query.addBindValue(rawData);

        if (!query.exec()) {
            ui->plainTextEdit_log->appendPlainText("Ошибка вставки для ID " + QString::number(noradId) + ": " + query.lastError().text());
        } else {
            successCount++;
        }
    }

    db.commit();
    ui->plainTextEdit_log->appendPlainText("Сохранено записей: " + QString::number(successCount) + " из " + QString::number(data.size()));
}

void MainWindow::loadFromDatabase()
{
    if (!db.isOpen()) {
        ui->plainTextEdit_log->appendPlainText("БД не открыта");
        return;
    }

    QSqlQuery query("SELECT norad_id, name, country, launch_date FROM satellites ORDER BY norad_id");

    if (!query.exec()) {
        ui->plainTextEdit_log->appendPlainText("Ошибка запроса: " + query.lastError().text());
        return;
    }

    // Создаем модель для таблицы
    QStandardItemModel *model = new QStandardItemModel(0, 4, this);
    QStringList headers = {"NORAD ID", "Name", "Country", "Launch Date"};
    model->setHorizontalHeaderLabels(headers);

    int row = 0;
    while (query.next()) {
        model->insertRow(row);
        model->setData(model->index(row, 0), query.value(0).toInt());
        model->setData(model->index(row, 1), query.value(1).toString());
        model->setData(model->index(row, 2), query.value(2).toString());
        model->setData(model->index(row, 3), query.value(3).toString());
        row++;
    }

    ui->tableView_satellites->setModel(model);
    ui->plainTextEdit_log->appendPlainText("Загружено из БД: " + QString::number(row) + " записей");
}
