#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QPushButton>

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#define COMM_TESTER_RX_FRAME_HEAD (0xF2)


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    int Times,portTime,lineEditData;




private:


    Ui::MainWindow *ui;
    QSerialPort *serial;                    //串口端口
    int SerialOpenFlag = 0;                 //串口是否打开标志位
    QTimer *timer = new QTimer(this);       //定时器

    long int SendByte,ReceByte;             //发送、接收字符数
    QStringList portStringLine;             //端口链表
    QByteArray Sendtext,Receivetext;        //发送、接收缓存区
    QByteArray IAPReceive,IAPSend;
    QString MCUVersion;

    //IAP变量
    QString fileName;                      //文件名
    QByteArray fileData;
    int binSize;                           //bin文件大小
    int UpgrateStatus;

    QByteArray data;
    uint8_t ReceiveBuf[100];

    QProgressBar *progressBar;             // 创建一个进度条
    QDialog *dialog;                       // 创建一个弹窗
    QVBoxLayout *layout;
    QPushButton *IAPbtnCancel;               // 创建一个按钮对象
    int IAPPercent = 0;
    int SubThreadCreat = 0;



    int m_totalSize;
    int m_bytesSent;
    QByteArray s_buffer;
    char m_checksum;

    typedef enum {
      CommRxState_Head,
      CommRxState_CmdH,
      CommRxState_CmdL,
      CommRxState_Len,
      CommRxState_Data,
      CommRxState_Checksum,
    } CommTester_Rx_State_t;

    typedef struct {
      CommTester_Rx_State_t State;
      uint8_t Checksum;
      uint16_t Index;
      uint8_t Data[100];  // 数据域
      uint8_t cnt;
      uint8_t CmdH;
      uint8_t CmdL;
      uint8_t Len;

    } CommTester_Rx_t;

    CommTester_Rx_t  CommTester_Rx = {};

    void SendInformationFunction();
    void sendNextChunk();
    bool canUpgrade(const QString &currentVersion, const QString &targetVersion); //比较版本号是否可以升级
    void ReceiveBufAnalyze();




signals:
    void CanSendUpdata(void);
    void SendDataToThread(QString data);
    void IAPfinish();
    void updateProgress(int Percent);



private slots:
    void progressUpdated(int percentage);    //IAP进度更新
    void IAPCancel();

    void serialPort_readyRead();            //串口接收

    void SendUpDataFunction();              //IAP专用发送函数
//    void TimerEvent();                      //定时发送、更新串口
    void on_ScanBt_clicked();                 //扫描串口
    void on_OpenBt_clicked();                 //打开串口键按下
    void on_BurnRB_clicked();                 //烧录模式按钮
    void on_TestRB_clicked();                 //测试模式按钮
    void on_NormalRB_clicked();               //正常模式按钮
    void on_OpenFileBt_clicked();             //打开文件按键
    void on_SendHexFileBt_clicked();          //IAP按钮
    void IAPfiinishFunction();                //IAP完成相关事宜



};

#endif // MAINWINDOW_H
