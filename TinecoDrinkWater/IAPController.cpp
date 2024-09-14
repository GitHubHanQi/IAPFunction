#include "IAPController.h"
#include <QtMath>

IAPController::IAPController(QSerialPort* serialPort, QByteArray fileData, QObject* parent): QThread(parent)
{
    m_serialPort = serialPort;
    m_fileData = fileData;


//    IAPTimerSend = new QTimer();
//    IAPTimer = new QTimer();
//    connect(IAPTimer,&QTimer::timeout,this,&IAPController::onReadyRead);
//    connect(IAPTimerSend,&QTimer::timeout,this,&IAPController::onBytesWritten);

//    IAPTimer->setInterval(20);
//    IAPTimerSend->setInterval(100);

    connect(m_serialPort, &QSerialPort::bytesWritten, this, &IAPController::onBytesWritten);
    connect(m_serialPort, &QSerialPort::readyRead, this, &IAPController::onReadyRead);

//   connect(this, &IAPController::DealSerialData, this, &IAPController::onReadyRead);


}



void IAPController::startIAP() {

    m_totalSize = qCeil(m_fileData.size()/1024);
    m_bytesSent = 0;

    onStartIAP();

}

void IAPController::onStartIAP() {

    sendNextChunk();
    qDebug()<<"准备发送1k数据";
}

void IAPController::sendNextChunk() {
    if (m_bytesSent < m_totalSize) {
        m_buffer = m_fileData.mid(m_bytesSent*1024,1024);
        m_buffer.prepend(QByteArray::number(m_bytesSent + 1)); // 将 FrameSend 转换为 const char* 类型并在前面添加到 chunk 中
        m_buffer.prepend('\xFF');
        m_buffer.prepend('\x01');
        m_buffer.prepend('\xAA');
        m_buffer.prepend('\xF1');

        m_checksum = 0;
        for (int j = 1; j < m_buffer.size(); j++) {
            m_checksum += m_buffer.at(j);
        }
        // 将总和添加到QByteArray的末尾
        m_buffer.append(m_checksum);
        m_serialPort->write(m_buffer);
        qDebug()<<"开始发送数据";
    } else {
        emit finished();
    }
}

void IAPController::onBytesWritten() {
    qDebug()<<"onBytesWritten";

    if (m_serialPort->bytesToWrite() == 0) {
        // 数据已全部写入，执行相应的操作
        qDebug()<<"数据已全部写入，执行相应的操作";
    }

}

void IAPController::onReadyRead() {
    qDebug()<<"读数据";

    QString response = SerialData;
    if (response == "F2AA010100AC" ||response == "F2AA010101AD" ) { // 假设ACK表示成功接收
        int progress = static_cast<int>((static_cast<double>(m_bytesSent) / m_totalSize) * 100);
        emit updateProgress(progress);

        m_bytesSent++;

        sendNextChunk();
    }
}


