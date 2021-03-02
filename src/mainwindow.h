#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QString>
#include <QDebug>
#include <QRegExp>
#include <QTimer>
#include <QLabel>

#include "encrypt_v1.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum status_modem {
        blocked = 1,
        unblocked   = 2,
        custom    = 3
    } status = custom;

    QString imei = "N/A";
    QString nck = "N/A";
    int attempts_left = -1;

    void showPortInfo(int idx);
    void fillPortsInfo();
    void send_msg(const QString &msg);
    void encryptNck(const QString &imei, QString &nck);
    void showModemInfo();


private:
    void showStatusMessage(const QString &message);

    Ui::MainWindow *ui;
    QSerialPort *m_serial = nullptr;
    QLabel *m_status = nullptr;

private slots:
    void openSerialPort();
    void closeSerialPort();
    void parse_imei();
    void parse_status();
    void updateModemInfo();
    void unlock();
};
#endif // MAINWINDOW_H
