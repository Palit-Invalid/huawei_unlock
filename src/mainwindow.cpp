#include "mainwindow.h"
#include "ui_mainwindow.h"



static const char blankString[] = ("N/A");

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_serial(new QSerialPort(this))
    , m_status(new QLabel)
{
    ui->setupUi(this);

    fillPortsInfo();
    showPortInfo(0);
    updateSettings();
    ui->statusbar->addWidget(m_status);

    connect(ui->serialPortListBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::showPortInfo);
    connect(ui->connect_button, SIGNAL(clicked()),
            this, SLOT(openSerialPort()));
    connect(ui->disconnect_button, SIGNAL(clicked()),
            this, SLOT(closeSerialPort()));
    connect(ui->update_button, SIGNAL(clicked()),
            this, SLOT(updateModemInfo()));
    connect(ui->nck_button, SIGNAL(clicked()),
            this, SLOT(encrypt_nck()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showPortInfo(int idx)
{
    const QStringList list = ui->serialPortListBox->itemData(idx).toStringList();
    ui->descriptionLabel->setText(tr("Description: %1").arg(list.count() > 1 ? list.at(1) : tr(blankString)));
    ui->manufacturerLabel->setText(tr("Manufacturer: %1").arg(list.count() > 2 ? list.at(2) : tr(blankString)));
    ui->serialNumberLabel->setText(tr("Serial number: %1").arg(list.count() > 3 ? list.at(3) : tr(blankString)));
    ui->locationLabel->setText(tr("Location: %1").arg(list.count() > 4 ? list.at(4) : tr(blankString)));
    ui->vidLabel->setText(tr("Vendor Identifier: %1").arg(list.count() > 5 ? list.at(5) : tr(blankString)));
    ui->pidLabel->setText(tr("Product Identifier: %1").arg(list.count() > 6 ? list.at(6) : tr(blankString)));
}

void MainWindow::fillPortsInfo()
{
    ui->serialPortListBox->clear();
    const auto infos = QSerialPortInfo::availablePorts();
    QString description;
    QString manufacturer;
    QString serialNumber;
    for (const QSerialPortInfo &info : infos) {
        QStringList list;
        description = info.description();
        manufacturer = info.manufacturer();
        serialNumber = info.serialNumber();
        list << info.portName()
             << (!description.isEmpty() ? description : blankString)
             << (!manufacturer.isEmpty() ? manufacturer : blankString)
             << (!serialNumber.isEmpty() ? serialNumber : blankString)
             << info.systemLocation()
             << (info.vendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : blankString)
             << (info.productIdentifier() ? QString::number(info.productIdentifier(), 16) : blankString);

        ui->serialPortListBox->addItem(list.first(), list);
    }
}

void MainWindow::openSerialPort()
{
    updateSettings();
    m_serial->setPortName(m_currentSettings.name);
    m_serial->setBaudRate(m_currentSettings.baudRate);
    m_serial->setDataBits(m_currentSettings.dataBits);
    m_serial->setParity(m_currentSettings.parity);
    m_serial->setStopBits(m_currentSettings.stopBits);
    m_serial->setFlowControl(m_currentSettings.flowControl);

    if (m_serial->open(QIODevice::ReadWrite)) {
        showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                          .arg(m_currentSettings.name).arg(m_currentSettings.stringBaudRate).arg(m_currentSettings.stringDataBits)
                          .arg(m_currentSettings.stringParity).arg(m_currentSettings.stringStopBits).arg(m_currentSettings.stringFlowControl));
        m_serial->clear();
    } else {
        showStatusMessage(tr("Open error"));
    }

}

void MainWindow::updateSettings()
{
    m_currentSettings.name = ui->serialPortListBox->currentText();

    m_currentSettings.baudRate = 115200;

    m_currentSettings.stringBaudRate = QString::number(m_currentSettings.baudRate);

    m_currentSettings.dataBits = static_cast<QSerialPort::DataBits>(8);
    m_currentSettings.stringDataBits = QString::number(m_currentSettings.dataBits);

    m_currentSettings.parity = static_cast<QSerialPort::Parity>(0);
    m_currentSettings.stringParity = QString::number(m_currentSettings.parity);

    m_currentSettings.stopBits = static_cast<QSerialPort::StopBits>(1);
    m_currentSettings.stringStopBits = QString::number(m_currentSettings.stopBits);

    m_currentSettings.flowControl = static_cast<QSerialPort::FlowControl>(0);
    m_currentSettings.stringFlowControl = QString::number(m_currentSettings.flowControl);
}

void MainWindow::closeSerialPort()
{
    if (m_serial->isOpen()) {
        m_serial->close();
        showStatusMessage(tr("Disconnected"));
    }
}

void MainWindow::send_msg(const QString &msg)
{
    QByteArray writeData = msg.toUtf8();
    const qint64 bytesWritten = m_serial->write(writeData);
    qDebug() << "Send bytes" << bytesWritten << "Message: " << QString(writeData);
}

void MainWindow::parse_imei()
{
    QRegExp re("\\d{15,15}\r\n");   //parse imei
    QByteArray readData;
    while (m_serial->canReadLine()) {
        readData = m_serial->readLine();
        if (re.exactMatch(QString(readData))) {
            imei = QString(readData);
            imei.remove(QRegExp("[\r\n]")); //remove /r and /n in end of string
            ui->imei->setText(imei);
            break;
        }
    }
}

void MainWindow::showStatusMessage(const QString &message)
{
    m_status->setText(message);
}

void MainWindow::parse_status()
{
    QRegExp re("^\\^CARDLOCK: \\d,\\d\\d?,0\r\n");   //parse status
    QByteArray readData;
    while (m_serial->canReadLine()) {
        readData = m_serial->readLine();
        qDebug() << QString(readData);
        if (re.exactMatch(QString(readData))) {
            qDebug() << QString(readData);
            std::sscanf(QString(readData).toLocal8Bit(), "^CARDLOCK: %i, %i, 0\r\n", &status, &attempts_left);
            break;
        }
    }
    switch (status)
    {
        case unblocked:
            ui->status->setText("UNBLOCKED");
            break;
        case blocked:
            ui->status->setText("BLOCKED");
            break;
        case custom:
            ui->status->setText("CUSTOM");
            break;
    }
    ui->attempts->setNum(attempts_left);
}

void MainWindow::updateModemInfo()
{
    if(m_serial->isOpen())
    {
        send_msg("AT^CARDLOCK?\r\n");
        QTimer::singleShot(10, this, [this](){
            parse_status();
            send_msg("AT+CGSN\r\n");
            QTimer::singleShot(10, this, SLOT(parse_imei()));
        });
    }
}

void MainWindow::encrypt_nck()
{
    char* datacard = "hwe620datacard";
    qDebug() << "NCK IMEI " << imei;
    char* imei_char = imei.toLatin1().data();
    encrypt_v1(imei_char, nck, datacard);
    ui->nck->setText(QString(nck));
}
