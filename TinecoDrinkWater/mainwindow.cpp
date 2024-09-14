#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QStringList>
#include <QString>
#include <QThread>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QtMath>






MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    resize(850,600); //窗口限制
    setWindowTitle("Tineco");
    SendByte=0;ReceByte=0;
    serial = new QSerialPort(this);



    //IAP进度条

    dialog = new QDialog;             // 创建一个弹窗
    layout = new QVBoxLayout;
    progressBar = new QProgressBar;            // 创建一个进度条
    IAPbtnCancel = new QPushButton("Cancel", dialog);
    progressBar->setRange(0, 100);             // 设置进度条的范围
    layout->addWidget(progressBar);            // 将进度条添加到布局中
    layout->addWidget(IAPbtnCancel);
    dialog->setLayout(layout);                 // 将布局设置到弹窗中
    dialog->setWindowTitle("IAP进度");





   connect(IAPbtnCancel,&QPushButton::clicked,this,&MainWindow::IAPCancel);

   connect(serial,&QSerialPort::readyRead,             //接收数据的连接   这边用定时器的原因是防止接收的数据断断续续
            this,[=](){

        qDebug()<<"timer start";
        timer->start(50);//100ms
        Receivetext.append(serial->readAll()); // 从串口读取数据

    });


    connect(timer,&QTimer::timeout,this,&MainWindow::serialPort_readyRead);

    connect(this,&MainWindow::updateProgress,this,& MainWindow::progressUpdated);

    connect(this,&MainWindow::CanSendUpdata,this,&MainWindow::SendUpDataFunction);

    connect(this,&MainWindow::IAPfinish,this,&MainWindow::IAPfiinishFunction);





}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_ScanBt_clicked()                  //扫描串口
{
    int i,n;
    ui->SerialCB->clear();
    portStringLine.clear();
    foreach(const QSerialPortInfo &info,QSerialPortInfo::availablePorts()) //自动搜索可用串口
        portStringLine +=info.portName();   //自动加入链表中
    n=portStringLine.size();
    for(i=0;i<n;i++)
    {
        serial->setPortName(portStringLine[i]);
        if(!serial->open(QIODevice::ReadWrite))
        {
            portStringLine[i]+="(不可用)";
            QVariant v(0);          //禁用
            ui->SerialCB->setItemData(1, v, Qt::UserRole - 1);
        }
        else
        {
            QVariant v(1|32);       //可用
            ui->SerialCB->setItemData(1, v, Qt::UserRole - 1);
        }
        serial->close();
    }
    ui->SerialCB->addItems(portStringLine);
}

void MainWindow::on_OpenBt_clicked()
{
    if(ui->OpenBt->text()==QString("打开串口"))
    {
        //设置串口名
        serial->setPortName(ui->SerialCB->currentText());
        //设置波特率
        serial->setBaudRate(ui->BaundrateCB->currentText().toInt());
        //设置数据位
        switch(ui->DataCB->currentText().toInt())
        {
        case 5:serial->setDataBits(QSerialPort::Data5);break;
        case 6:serial->setDataBits(QSerialPort::Data6);break;
        case 7:serial->setDataBits(QSerialPort::Data7);break;
        case 8:serial->setDataBits(QSerialPort::Data8);break;
        default:serial->setDataBits(QSerialPort::UnknownDataBits);break;
        }
        //设置奇偶校验位
        switch(ui->CheckCB->currentIndex())
        {
        case 0:serial->setParity(QSerialPort::NoParity);break;
        case 2:serial->setParity(QSerialPort::EvenParity);break;
        case 3:serial->setParity(QSerialPort::OddParity);break;
        default:serial->setParity(QSerialPort::UnknownParity);break;
        }
        //设置停止位
        switch (ui->StopCB->currentIndex())
        {
        case 1:serial->setStopBits(QSerialPort::OneStop);break;
        case 2:serial->setStopBits(QSerialPort::TwoStop);break;
        default:serial->setStopBits(QSerialPort::UnknownStopBits);break;
        }
        //设置流控制
        serial->setFlowControl(QSerialPort::NoFlowControl);

        //打开串口
        if(!serial->open(QIODevice::ReadWrite))
        {
            QMessageBox::about(nullptr,"提示","无法打开串口");//这边空指针用nullptr
            SerialOpenFlag = 0;
            return;
        }
        else
        {
            ui->statusBar->showMessage(tr("串口已打开"));
            SerialOpenFlag = 1;
        }
        //下拉控件失能
        ui->SerialCB->setEnabled(false);
        ui->BaundrateCB->setEnabled(false);
        ui->DataCB->setEnabled(false);
        ui->CheckCB->setEnabled(false);
        ui->StopCB->setEnabled(false);
        ui->ScanBt->setEnabled(false);

        ui->OpenBt->setText(tr("关闭串口"));

    }
    else
    {
        //关闭串口
        serial->close();
        //下拉按键使能
        ui->SerialCB->setEnabled(true);
        ui->BaundrateCB->setEnabled(true);
        ui->DataCB->setEnabled(true);
        ui->CheckCB->setEnabled(true);
        ui->StopCB->setEnabled(true);
        ui->ScanBt->setEnabled(true);

        ui->OpenBt->setText(tr("打开串口"));

        SerialOpenFlag = 0;
    }


}


void MainWindow::SendInformationFunction() //发送接口函数
{

        QByteArray bytearray;
        Sendtext=ui->SendEdit->text().toUtf8();
    //        if(ui->checkBox_6->checkState()==Qt::Checked)//后面加\n的
    //            Sendtext += '\n';
        bytearray = QByteArray::fromHex(Sendtext);     //十六进制转字符串
        serial->write(bytearray);
        ui->ReadEdit->clear();
        if(SerialOpenFlag == 0){
    //        QMessageBox::about(nullptr,"提示","串口未打开");//这边空指针用nullptr
            QMessageBox::information(this,"提示", "串口未打开");
        }

}


void MainWindow::serialPort_readyRead(){

    timer->stop();
    data.clear();
    data = Receivetext;
    Receivetext.clear();

    //数据处理
     QString hexString = QString(data.toHex().toUpper()); // 将接收到的数据转换为十六进制字符串
         qDebug() << "Value: " << hexString;
     for(int i = 2; i < hexString.length(); i+=3) // 在字符串中每隔两个字符插入一个空格
     {
         hexString.insert(i, ' ');
     }
     ui->ReadEdit->clear();
     QString lasttext = ui->ReadEdit->text(); // 获取原有文本
     lasttext.append(hexString); // 添加本次接收到的十六进制数据

     ui->ReadEdit->setText(lasttext); // 将文本框中的内容设置为最新的内容


     for (int i = 0; i < data.size(); i++) {
         ReceiveBuf[i] = static_cast<uint8_t>(data[i]);
//         qDebug() << "ReceiveBuf " <<i<<":"<<ReceiveBuf[i] ;
     }

     CommTester_Rx.cnt = 0;
     CommTester_Rx.Checksum = 0;
     ReceiveBufAnalyze();

     /***iap ***/
     qDebug() << "m_bytesSent+1"<<m_bytesSent+1  << "CommTester_Rx.Data[0]"<<CommTester_Rx.Data[0];
     if(CommTester_Rx.CmdH == 0xAA && CommTester_Rx.CmdL == 0x01 && CommTester_Rx.Data[0] == (m_bytesSent+1)){

         m_bytesSent++;
         qDebug() <<"m_bytesSent"<< m_bytesSent;
         sendNextChunk();

         int progress = static_cast<int>((static_cast<double>(m_bytesSent) / (m_totalSize-1)) * 100);
         emit updateProgress(progress);

     }

     /***MCU Version***/
//     for(int i = 0;i<CommTester_Rx.Len;i++){
//         qDebug()<<"CommTester_Rx.Data[i]:"<<CommTester_Rx.Data[i];
//     }
     if(CommTester_Rx.CmdH ==0x88 && CommTester_Rx.CmdL == 0x00){
         MCUVersion.clear();
         for (int i = 0;i<CommTester_Rx.Len;i++) {
             MCUVersion += static_cast<char>(CommTester_Rx.Data[i]);
         }
         qDebug()<<MCUVersion;

     }

}


void MainWindow::ReceiveBufAnalyze(){
//    qDebug() <<CommTester_Rx.cnt<<"**"<<data.size();
    while(CommTester_Rx.cnt < data.size()){
        switch(CommTester_Rx.State){
        case CommRxState_Head:
            if (COMM_TESTER_RX_FRAME_HEAD != ReceiveBuf[CommTester_Rx.cnt]) {
                CommTester_Rx.State = CommRxState_Head;
//                qDebug() << "CommTester_Rx.State = CommRxState_Head";
                return;
            } else {
//                qDebug() << "CommTester_Rx.State = CommRxState_CmdH";
                CommTester_Rx.cnt++;
                CommTester_Rx.State = CommRxState_CmdH;
            }
            break;
        case CommRxState_CmdH:
//             qDebug() << "Value: " << ReceiveBuf[CommTester_Rx.cnt];
              CommTester_Rx.CmdH = ReceiveBuf[CommTester_Rx.cnt];
              CommTester_Rx.Checksum += ReceiveBuf[CommTester_Rx.cnt];
              CommTester_Rx.cnt++;
              CommTester_Rx.State = CommRxState_CmdL;

              break;

        case CommRxState_CmdL:
//            qDebug() << "Value: " << ReceiveBuf[CommTester_Rx.cnt];
              CommTester_Rx.CmdL = ReceiveBuf[CommTester_Rx.cnt];
              CommTester_Rx.Checksum += ReceiveBuf[CommTester_Rx.cnt];
              CommTester_Rx.cnt++;
              CommTester_Rx.State = CommRxState_Len;
              break;

        case CommRxState_Len:
//            qDebug() << "Value: " << ReceiveBuf[CommTester_Rx.cnt];
              CommTester_Rx.Len = ReceiveBuf[CommTester_Rx.cnt];
              CommTester_Rx.Checksum += ReceiveBuf[CommTester_Rx.cnt];
              CommTester_Rx.cnt++;
              if (0 == CommTester_Rx.Len)  // 数据域长度为0
              {
                CommTester_Rx.State = CommRxState_Checksum;

              } else {
                CommTester_Rx.State = CommRxState_Data;
              }
              break;

        case CommRxState_Data:
//            qDebug() << "Value: " << ReceiveBuf[CommTester_Rx.cnt];
              CommTester_Rx.Data[CommTester_Rx.Index++] = ReceiveBuf[CommTester_Rx.cnt];  // 记录数据部分
              CommTester_Rx.Checksum += ReceiveBuf[CommTester_Rx.cnt];
              CommTester_Rx.cnt++;
              if (CommTester_Rx.Index >= CommTester_Rx.Len) {
                CommTester_Rx.State = CommRxState_Checksum ;
              }
              break;

        case CommRxState_Checksum:
//            qDebug() << "Value: " << ReceiveBuf[CommTester_Rx.cnt];
              if (CommTester_Rx.Checksum != ReceiveBuf[CommTester_Rx.cnt]) {
                  qDebug()<<"接收数据异常，checksum异常";

                CommTester_Rx.cnt = 0;
                CommTester_Rx.Index = 0;
                CommTester_Rx.Checksum = 0;
                CommTester_Rx.State = CommRxState_Head;
                return;
              } else {
                  qDebug()<<"接收数据正常";
                CommTester_Rx.cnt = 0;
                CommTester_Rx.Index = 0;
                CommTester_Rx.Checksum = 0;
                CommTester_Rx.State = CommRxState_Head;
                return;
              }
              break;

         default:
              break;
        }

    }



}


void MainWindow::on_BurnRB_clicked()
{
    ui->SendEdit->clear();
    ui->SendEdit->setText("F1 AA 00 01 01 AC");
    SendInformationFunction();
}

void MainWindow::on_TestRB_clicked()
{
    ui->SendEdit->clear();
    ui->SendEdit->setText("F1 AA 00 01 00 AB");
    SendInformationFunction();
}

void MainWindow::on_NormalRB_clicked()
{
    ui->SendEdit->clear();
    ui->SendEdit->setText("F1 AA 00 01 02 AD");
    SendInformationFunction();
}

void MainWindow::on_OpenFileBt_clicked()
{
    fileName = QFileDialog::getOpenFileName(this,"Open File",QDir::currentPath(), "Hex文件 (*.hex);;Binary文件 (*.bin);;所有文件(*.*)");
    ui->filePathLineEdit->setText (fileName);
    if(fileName.isEmpty())
    {
        QMessageBox::information(this,"Error Message", "Please Select a Text File");
        return;
    }
    QFileInfo *pcsfileInfo = new QFileInfo(fileName);
    binSize =  static_cast<int>(pcsfileInfo->size());
    delete pcsfileInfo;


    ui->SendEdit->clear();
    ui->SendEdit->setText("F1 88 00 01 00 89");
    SendInformationFunction();

}

void MainWindow::on_SendHexFileBt_clicked()
{

    //获取此次IAP的版本号
    QFileInfo fileInfo(fileName);
    qDebug()<<"fileName:"<<fileName;
    QString IAPVersion = fileInfo.fileName();
    QString BinVersion;
    QString BinVersionNum;
    QRegularExpression re("([A-Za-z0-9_]+)_(\\d+\\.\\d+\\.\\d+\\.\\d+)");
    QRegularExpressionMatch IAPmatch = re.match(IAPVersion);
    if (IAPmatch.hasMatch()) {
        BinVersion = IAPmatch.captured(0); // Whole match  不含checksum的名称
        BinVersionNum = IAPmatch.captured(2); // Capturing group 2 - version number
        qDebug() << "BinVersion: " << BinVersion;
        qDebug() << "BinversionNum: " << BinVersionNum;
    }

    //获取MCU版本号

    qDebug() << "MCUVersion: " << MCUVersion;
    QRegularExpressionMatch MCUmatch = re.match(MCUVersion);
    QString MCUVersionNum = MCUmatch.captured(2);
    qDebug() << "MCUVersionNum: " << MCUVersionNum;

    //比较两个版本是否一样
    bool CanUpgrade = canUpgrade(MCUVersionNum,BinVersionNum);

    if(CanUpgrade == true){

        UpgrateStatus = 1;
        ui->statusBar->showMessage(tr("正在更新"));
        qDebug() << "CanSendUpdata" ;
        emit CanSendUpdata();

    }else {
       QMessageBox::information(this,"提示","请使用合适、规范的文件名及Hex文件进行升级");//这边空指针用nullptr
       return;
    }

}

bool MainWindow::canUpgrade(const QString &currentVersion, const QString &targetVersion) {
    QStringList currentVersionList = currentVersion.split(".");
    QStringList targetVersionList = targetVersion.split(".");

    if (currentVersionList.size() != targetVersionList.size()) {
        return false; // 如果版本号长度不一样，直接返回不升级
    }

    for (int i = 0; i < currentVersionList.size(); i++) {
        if (currentVersionList[i].toInt() > targetVersionList[i].toInt()) {
            return false; // 不能降级
        } else if (currentVersionList[i].toInt() <= targetVersionList[i].toInt()) {
            return true; // 可以升级
        }
    }

    return true; // 如果版本号相同也可以升级
}

void MainWindow::SendUpDataFunction(void){
    qDebug() << "SendUpDataFunction" ;
    QFile* file = new QFile;
    fileData.clear();
    file->setFileName(fileName);
    //获取是什么类型文件bin还是hex
    QFileInfo fileInfo(fileName);
    QString DocumentType = fileInfo.suffix();
    QTextStream in(file);

    if(file->open(QIODevice::ReadOnly) == true){
        qDebug() << "File opened successfully";

        if(DocumentType.toLower() == "hex"){

            while (!in.atEnd()) {
                QString line = in.readLine();
                // 假设hex文件的每一行格式为:HHAAAATTHHHH
                // 其中HH为地址，TT为类型，HHH为数据
                QString DeleteLine = line.mid(7, 2);
                if(DeleteLine == "00"){
                    QString data = line.mid(9, line.length() - 11); // 去除地址后的数据
                    fileData.append(QByteArray::fromHex(data.toLatin1())); // 将hex字符串转换为字节数组
                }else{}

            }

        }else if((DocumentType.toLower() == "bin")){
            fileData = file->readAll();
        }else {}

//        QFile outFile("C:\\Users\\hanqi.zhang\\Desktop\\outputFile.bin");
//        if(outFile.open(QIODevice::WriteOnly)) {
//            outFile.write(fileData);
//            outFile.close();

//        }
        qDebug() << "fileDataSize: "<<fileData.size();

    }
    file->close();


    if(fileData.size()%1024 == 0)
    {
        m_totalSize = fileData.size()/1024;
    }
    else {
        m_totalSize = fileData.size()/1024+1;
    }
    m_bytesSent = 0;

    sendNextChunk();

}

void MainWindow::progressUpdated(int percentage){

    // 设置进度条的当前值
    if(percentage>0 && percentage<100){
        dialog->setWindowModality(Qt::ApplicationModal);
        dialog->show();
    }else if (percentage>=100 ||percentage<0) {
        dialog->close();
        qDebug()<<"dialog close";
    }
    progressBar->setValue(percentage);
    qDebug()<<"百分比"<<percentage;


}

void MainWindow::IAPCancel(){
    qDebug()<<"Go to IAP Cancel";
    dialog->close();

}


void MainWindow::sendNextChunk() {

    if (m_bytesSent < m_totalSize) {
        qDebug()<<"m_totalSize"<< m_totalSize;
        s_buffer = fileData.mid(m_bytesSent*1024,1024);//最后一帧会自动补零
        s_buffer.resize(1024);
//        m_buffer.prepend("\x01");
        s_buffer.prepend(static_cast<char>(m_bytesSent+1));
        qDebug()<<"帧号"<<QByteArray::number(m_bytesSent + 1);
        s_buffer.prepend('\xFF');
        s_buffer.prepend('\x01');
        s_buffer.prepend('\xAA');
        s_buffer.prepend('\xF1');

        m_checksum = 0;
        qDebug()<<"s_buffer.size():"<<s_buffer.size();
        for (int j = 1; j < s_buffer.size(); j++) {
            m_checksum += s_buffer.at(j);
        }
        // 将总和添加到QByteArray的末尾
        s_buffer.append(m_checksum);
        serial->write(s_buffer);
        s_buffer.fill(0);//清空buf，防止最后一帧数据不对


        qDebug()<<"开始发送数据";
    } else {
        emit IAPfinish();

    }
}

void MainWindow::IAPfiinishFunction(){
    qDebug()<<"IAP已完成";
    m_bytesSent = 0;
    m_totalSize = 0;

}






