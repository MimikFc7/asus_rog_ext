#include <QCoreApplication>
#include <libusb-1.0/libusb.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

#include <QDebug>
#include <QFile>
#include <QThread>
#include <QTimer>
#include <QDateTime>
#include <QLoggingCategory>
#include <QProcess>

#define CPU_FREQ_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"
#define CPU_TEMP_PATH "/sys/devices/platform/coretemp.0/hwmon/hwmon3/temp1_input"
#define CPU_RPM_PATH "/sys/class/hwmon/hwmon4/fan1_input"
#define CPU2_RPM_PATH "/sys/class/hwmon/hwmon4/fan2_input"
#define CHA1_RPM_PATH "/sys/class/hwmon/hwmon4/fan4_input"
#define CHA2_RPM_PATH "/sys/class/hwmon/hwmon4/fan5_input"
#define CHA3_RPM_PATH "/sys/class/hwmon/hwmon4/fan7_input"

#define CPU_RPM_MAX 3000
#define CHA_RPM_MAX 2000

static QTimer *usbhandler = nullptr;;
static libusb_device_handle *asusRogBaseUsbDevice = nullptr;

enum COMMANDTYPE : uint8_t{
    READ = 1,
    WRITE = 2,
    GET_VERSION = 16
};


//Для видеокарт NVIDIA
//выполнять под рутом
//nvidia-xconfig -a --cool-bits=28 --allow-empty-initial-configuration                               - First to enable overclocking (One time thing)
//nvidia-settings -a '[gpu:0]/GPUFanControlState=1                                                   - Secondly enable GPU fan control to manual mode
//nvidia-settings -a '[fan]/GPUTargetFanSpeed=<some number>                                          - Lastly set the fan speed
//nvidia-smi --query-gpu=temperature.gpu,fan.speed  --format=csv,noheader,nounits                    - Call gpu temp
//nvidia-settings --terse --query [fan:0]/GPUCurrentFanSpeedRPM                                      - Call gpu RPM

struct DISPLAYICONS{

    unsigned FICTION:1;
    unsigned SPORT:1;
    unsigned RACING:1;
    unsigned SHOOTING:1;
    unsigned MUSIC:1;
    unsigned icon5:1; //не найдено
    unsigned icon6:1; //не найдено
    unsigned icon7:1; //не найдено

    operator int() const{ return ( FICTION << 0) |  (SPORT << 1  ) |  (RACING << 2  ) |  (SHOOTING << 3  ) |  (MUSIC << 4  ) ; }

} __attribute__((packed));


struct CURRENTTIME {

    uint8_t hour;
    uint8_t min;

    operator int() const{ return ( hour << 0) |  (min << 8 ) ; }

} __attribute__((packed));


struct DISPLAYFLAGS{

    unsigned ON:1;
    unsigned REQ:1;
    unsigned CPUBOOST:1;
    unsigned FLAG:1;   //не распознал, вероятно резерв
    unsigned FLAG1:1;  //не распознал, вероятно резерв
    unsigned FLAG2:1;  //не распознал, вероятно резерв
    unsigned FLAG3:1;  //не распознал, вероятно резерв
    unsigned FLAG4:1;  //не распознал, вероятно резерв

} __attribute__((packed));

enum ADDRESS_TYPE : uint8_t{
    INIT = 1, //сброс, принимает знаения, но не понятно в одном случае загорается USB во втором нет (переключение режима? )
    SETWORKMODE = 3, //переключение режимов, нашел!!! IDA решает =)) 0xAA - рабочий режим, 0x00 режим USB
    DISPLAY_ON_OF = 10, // DISPLAYFLAGS - Битовая маска экранов
    DISPLAY_BOTTOM_LINE_ICONS = 96, //DISPLAYICONS - битовая маска показа значков
    CPU_TEMP = 32,
    MB_TEMP = 33,
    CPU_RPM = 64,
    CPU_RPM2 = 65,
    CHASIS_RPM1 = 66,
    CHASIS_RPM2 = 67,
    CHASIS_RPM3 = 68,

    CPU_RPM_PERC = 74, //0x0300 0x03 (процентная шкала от 1 до 10 (0x0a)
    CPU_RPM_PERC2 = 75, //0x0300 0x03 (процентная шкала от 1 до 10 (0x0a)
    CHA_RPM_PERC1 = 76, //0x0300 0x03 (процентная шкала от 1 до 10 (0x0a)
    CHA_RPM_PERC2 = 77, //0x0300 0x03 (процентная шкала от 1 до 10 (0x0a)
    CHA_RPM_PERC3 = 78, //0x0300 0x03 (процентная шкала от 1 до 10 (0x0a)

    CPU_SCALETYPE = 48, // 0 (не понятно) 1 - делитель 10, 40000 / 10 = 4 Ghz, вариантов много 10 самый удобный (0x0a00)
    CPU_FREQ = 49, //cpu freq, для корректного отображения надо задать делитель  CPU_SCALETYPE (1) оптимально
    TIME = 80 //время бется на 2 байта часы и минуты, 0x1233 = 12:33 по умолчанию формат 12 часовой, но по кнопке переключается на 24
};


struct ROG_PACKET{

    uint8_t command1 = 1;
    COMMANDTYPE command2 = READ;
    ADDRESS_TYPE  address = CPU_TEMP;
    uint16_t value = 0;    
    uint8_t value2 = 0; //not determinated
    uint8_t value3 = 0; //not determinated
    uint8_t value4 = 0; //not determinated

} __attribute__((packed));


static void handler(int s) {
    syslog (LOG_NOTICE, " qtrogextdriver called: SIGQUIT");
    if(usbhandler != nullptr){
        usbhandler->stop();
        delete usbhandler;

        if(asusRogBaseUsbDevice != nullptr){
            libusb_close(asusRogBaseUsbDevice);
        }
    }
    exit(EXIT_SUCCESS);
}

static void skeleton_daemon()
{
    pid_t pid;
    pid = fork();

    if (pid < 0){
        exit(EXIT_FAILURE);
    }

    if (pid > 0){
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0)
        exit(EXIT_FAILURE);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    pid = fork();

    if (pid < 0)
        exit(EXIT_FAILURE);

    if (pid > 0){
        exit(EXIT_SUCCESS);
    }

    umask(0);
    chdir("/");

    signal(SIGQUIT, handler);
    signal(SIGSTOP, handler);
    signal(SIGTERM, handler);
    signal(SIGCHLD, handler);

    /* Open the log file */
    openlog ("qtrogextdriver", LOG_PID, LOG_DAEMON);
}



int writeData( libusb_device_handle *devhandler, unsigned char *lpReportBuffer1){
   return libusb_control_transfer(devhandler,LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_OUT, 0x09, 0x0301 , 0, lpReportBuffer1, 8, 0); // SET REPORT WORKED
}

int readData( libusb_device_handle *devhandler, unsigned char *lpReportBuffer1){
    return libusb_control_transfer(devhandler,LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_IN, 0x01, 0x0301 , 0, lpReportBuffer1, 8, 0); // GET REPORT WORKED
}


void writeCommand( ADDRESS_TYPE command, uint16_t value, libusb_device_handle *devhandler, bool littleEndian = false){

    uchar* lpReportBuffer1 = new uchar[8];
    memset(lpReportBuffer1, 0x00,8);

    ROG_PACKET * packet = new ROG_PACKET();
    packet->command2 = WRITE;
    packet->address = command;
    if(littleEndian){ value = (value>>8) | (value<<8); }
    packet->value = value;
    memcpy(lpReportBuffer1, packet,8);

    int usb_state = -10000;

    usb_state =  writeData(devhandler,lpReportBuffer1);
    if(usb_state < 0){
        exit(1);
    }
    usb_state = -10000;
    usb_state = readData(devhandler,lpReportBuffer1);
    if(usb_state < 0){
        exit(1);
    }
    delete packet;
    delete []  lpReportBuffer1;
}

long readFileValue(QString file_path){

    long num = 0;
    QFile temp(file_path);
    temp.open(QFile::ReadOnly);
    QByteArray data = temp.readAll();
    num = (data.replace("\n","").toLong());
    temp.close();
    return num;
}


int main(int argc, char *argv[])
{

   /* syslog (LOG_ERR, " qtrogextdriver called start or stop ");
    if(strcmp(argv[1], "stop") == 0){

        if(usbhandler != nullptr){
            usbhandler->stop();
            delete usbhandler;

            if(asusRogBaseUsbDevice != nullptr){
                libusb_close(asusRogBaseUsbDevice);
            }
        }
    }*/

   // skeleton_daemon();

    QLoggingCategory::setFilterRules("*.debug=true");

    QCoreApplication a(argc, argv);
    //if(argc > 1)
    {
        //if(strcmp(argv[1], "start") == 0)
        {
            libusb_init(nullptr);
                    asusRogBaseUsbDevice = libusb_open_device_with_vid_pid(nullptr,0x1770,0xef35); //libusb_open(dev,&asusRogBaseUsbDevice);
                    if( asusRogBaseUsbDevice != nullptr){
                        syslog (LOG_NOTICE, " qtrogextdriver found usb device");
                            libusb_reset_device(asusRogBaseUsbDevice);
                            if( libusb_kernel_driver_active(asusRogBaseUsbDevice,0) ){
                                libusb_detach_kernel_driver(asusRogBaseUsbDevice,0);
                            }
                            libusb_claim_interface(asusRogBaseUsbDevice, 0);
                            libusb_set_configuration(asusRogBaseUsbDevice,0);
                            libusb_clear_halt(asusRogBaseUsbDevice,0x03);
                            libusb_clear_halt(asusRogBaseUsbDevice,0x82);

                            writeCommand(CPU_SCALETYPE,10,asusRogBaseUsbDevice, false);

                            usbhandler = new QTimer();
                            usbhandler->connect( usbhandler, &QTimer::timeout,[=](){

                            writeCommand(SETWORKMODE,0xaa,asusRogBaseUsbDevice, false);                            

                            uint16_t num =  ( readFileValue(CPU_TEMP_PATH) / 1000 ) ;
                            writeCommand(CPU_TEMP,num,asusRogBaseUsbDevice, true);


                            num = readFileValue(CPU_RPM_PATH);
                            writeCommand(CPU_RPM,num,asusRogBaseUsbDevice, true);
                            writeCommand(CPU_RPM_PERC,((num * 100)/  CPU_RPM_MAX) / 10,asusRogBaseUsbDevice);


                            num = readFileValue(CPU2_RPM_PATH);
                            writeCommand(CPU_RPM2,num,asusRogBaseUsbDevice, true);
                            writeCommand(CPU_RPM_PERC2,((num * 100)/  CPU_RPM_MAX) / 10,asusRogBaseUsbDevice);


                            num = readFileValue(CHA1_RPM_PATH);
                            writeCommand(CHASIS_RPM1,num,asusRogBaseUsbDevice, true);
                            writeCommand(CHA_RPM_PERC1,((num * 100)/  CHA_RPM_MAX) / 10,asusRogBaseUsbDevice);


                            num = readFileValue(CHA2_RPM_PATH);
                            writeCommand(CHASIS_RPM2,num,asusRogBaseUsbDevice, true);
                            writeCommand(CHA_RPM_PERC2,((num * 100)/  CHA_RPM_MAX) / 10,asusRogBaseUsbDevice);


                            //Это только для видях NVIDIA
                            QProcess getGpuTemp;
                            QStringList arguments;
                            arguments << "--query-gpu=temperature.gpu" <<  "--format=csv,noheader,nounits";
                            getGpuTemp.start("nvidia-smi",arguments,QProcess::ReadOnly);
                            getGpuTemp.waitForFinished();

                            QByteArray gpuTemp_result = getGpuTemp.readAllStandardOutput();
                            getGpuTemp.close();
                            arguments.clear();
                            arguments << "--terse" <<  "--query" <<"[fan:0]/GPUCurrentFanSpeedRPM";
                            getGpuTemp.start("nvidia-settings",arguments,QProcess::ReadOnly);
                            getGpuTemp.waitForFinished();

                            gpuTemp_result.clear();
                            gpuTemp_result = getGpuTemp.readAllStandardOutput();
                            num =  gpuTemp_result.toInt();
                            getGpuTemp.close();

                            writeCommand(CHASIS_RPM3,num,asusRogBaseUsbDevice, true);
                            writeCommand(CHA_RPM_PERC3,((num * 100)/  CPU_RPM_MAX) / 10,asusRogBaseUsbDevice);
                            // Конец только nvidia




                            num= (readFileValue(CPU_FREQ_PATH) / 1000);
                            writeCommand(CPU_FREQ,num,asusRogBaseUsbDevice, true);


                            DISPLAYICONS showAllIcons;
                            showAllIcons.MUSIC = 1;
                            showAllIcons.SPORT = 1;
                            showAllIcons.RACING = 1;
                            showAllIcons.FICTION = 1;
                            showAllIcons.SHOOTING = 1;
                            showAllIcons.icon5 = 1;
                            showAllIcons.icon6 = 1;
                            showAllIcons.icon7 = 1;

                            writeCommand(DISPLAY_BOTTOM_LINE_ICONS,(uint8_t)showAllIcons,asusRogBaseUsbDevice, false);


                            CURRENTTIME time;
                            time.hour =  QDateTime::currentDateTime().time().hour() ;
                            time.min = QDateTime::currentDateTime().time().minute();

                            writeCommand(TIME,(uint16_t)time,asusRogBaseUsbDevice);

                        });

                        usbhandler->setInterval(5000);
                        usbhandler->start();

                    }else{
                            syslog (LOG_NOTICE, " qtrogextdriver LIBUSB_ERROR open usb device");
                    }
                }


    }

    return a.exec();
}
