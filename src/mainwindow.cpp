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
    ui->statusbar->addWidget(m_status);

    connect(ui->serialPortListBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::showPortInfo);
    connect(ui->connect_button, SIGNAL(clicked()),
            this, SLOT(openSerialPort()));
    connect(ui->disconnect_button, SIGNAL(clicked()),
            this, SLOT(closeSerialPort()));
    connect(ui->unlockButton, SIGNAL(clicked()), this, SLOT(unlock()));
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

void MainWindow::showModemInfo()
{
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
        default:
            ui->status->setText(blankString);
            break;
    }
    ui->attempts->setNum(attempts_left);
    ui->nck->setText(nck);
    ui->imei->setText(imei);
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
    m_serial->setPortName(ui->serialPortListBox->currentText());
    m_serial->setBaudRate(115200);
    m_serial->setDataBits(static_cast<QSerialPort::DataBits>(8));
    m_serial->setParity(static_cast<QSerialPort::Parity>(0));
    m_serial->setStopBits(static_cast<QSerialPort::StopBits>(1));
    m_serial->setFlowControl(static_cast<QSerialPort::FlowControl>(0));

    if (m_serial->open(QIODevice::ReadWrite))
    {
        showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                          .arg(m_serial->portName()).arg(m_serial->baudRate()).arg(m_serial->dataBits())
                          .arg(m_serial->parity()).arg(m_serial->stopBits()).arg(m_serial->flowControl()));
        updateModemInfo();
    } else
    {
        showStatusMessage(tr("Open error"));
    }
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
        if (re.exactMatch(QString(readData)))
        {
            qDebug() << "ANSWER:\t" << QString(readData);
            imei = QString(readData);
            imei.remove(QRegExp("[\r\n]")); //remove /r and /n in end of string
            qDebug() << "PARSE:\t" << imei;
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
        if (re.exactMatch(QString(readData)))
        {
            qDebug() << "ANSWER:\t" << QString(readData);
            std::sscanf(QString(readData).toLocal8Bit(), "^CARDLOCK: %i, %i, 0\r\n", &status, &attempts_left);
            break;
        }
    }
}

void MainWindow::updateModemInfo()
{
    if(m_serial->isOpen())
    {
        send_msg("AT+CGSN\r\n");
        QTimer::singleShot(10, this, [this]()
        {
            parse_imei();
            encryptNck(imei, nck);
            send_msg("AT^CARDLOCK?\r\n");
            QTimer::singleShot(10, this, [this]()
            {
                parse_status();
                showModemInfo();
            });
         });
    }
}

void MainWindow::encryptNck(const QString &imei, QString &nck)
{
    char nckbuf[40];    //buffer for NCK code
    char imeibuf[16];   //buffer for IMEI
    strncpy(imeibuf, imei.toLocal8Bit().data(), 15);

    encrypt_v1(imeibuf, nckbuf, "hwe620datacard");
    nck = QString::fromLocal8Bit(nckbuf);
    qDebug() << "NCK:\t" << nck;
}

void MainWindow::unlock()
{
    send_msg(QString("AT^CARDLOCK=\"%1\"").arg(1));
    updateModemInfo();
}
