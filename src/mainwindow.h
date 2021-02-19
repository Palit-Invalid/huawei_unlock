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
#include <cstdio>

//#include "encrypt_v1.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void showPortInfo(int idx);
    void fillPortsInfo();
    void updateSettings();
    void send_msg(const QString &msg);

    struct Settings {
        QString name;
        qint32 baudRate;
        QString stringBaudRate;
        QSerialPort::DataBits dataBits;
        QString stringDataBits;
        QSerialPort::Parity parity;
        QString stringParity;
        QSerialPort::StopBits stopBits;
        QString stringStopBits;
        QSerialPort::FlowControl flowControl;
        QString stringFlowControl;
    };
    QString imei;
    char* nck;
    enum status_modem {
        blocked = 1,
        unblocked   = 2,
        custom    = 3
    } status;
    int attempts_left;

private:
    void showStatusMessage(const QString &message);

    Ui::MainWindow *ui;
    QSerialPort *m_serial = nullptr;
    Settings m_currentSettings;
    QLabel *m_status = nullptr;

private slots:
    void openSerialPort();
    void closeSerialPort();
    void parse_imei();
    void parse_status();
    void updateModemInfo();
    void encrypt_nck();
};
#endif // MAINWINDOW_H
