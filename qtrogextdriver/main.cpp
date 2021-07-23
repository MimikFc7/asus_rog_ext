#include <QCoreApplication>
#include <libusb-1.0/libusb.h>
#include <usbg/function/hid.h>
#include <QDebug>
#include <QFile>
#include <QThread>
#include <qdatetime.h>

#include <linux/types.h>
#include <linux/hiddev.h>
#include <linux/hid.h>

#define CPU_FREQ_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"
#define CPU_TEMP_PATH "/sys/devices/platform/coretemp.0/hwmon/hwmon3/temp1_input"
#define CPU_RPM_PATH "/sys/class/hwmon/hwmon4/fan1_input"
#define CPU_RPM_MAX 3000


enum COMMANDTYPE : uint8_t{
    READ = 1,
    WRITE = 2,
    GET_VERSION = 16
};


//10 - включить выключить экран? (3) статус включен, 2 выключен!
//11 - данные есть но что там не понять

/*96 - на экране снизу есть интересные написи, это они, 0 - скрыыть, 1 - FCTON 2 - SPORT, 3 - FCTON SPORT, 4 - RACO, 5 - FCTON  RACO, 6 - SPORT  RACO, 7 - FCTON SPORT  RACO, 8 - SHOOTING, 9 - SHOOTING FCTON, 10 - SHOOTING SPORT
       11 - SHOOTING FCTON SPORT  долго перебирать 31 - показать все значки*/

//32 - CPU TEMP
//64 - CPU RPM
//49 cpu freq надо делить на  16000 в мегагерцах с точкой
//80 - часы минуты минуты 0x0d11 - собирать надо как 2 байта в хексе текущего времени

/*
                            uchar* lpReportBuffer2 = new uchar[8];
                            memset(lpReportBuffer2, 0x00,8);
                            ROG_PACKET * packet1 = new ROG_PACKET();
                            packet1->command2 = WRITE;
                            packet1->address = CPU_SCALETYPE;
                            packet1->value =  10;
                            memcpy(lpReportBuffer2, packet1,8);
                            //настройка для скейла процессора выбрал себе по умолчанию 10

*/


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
    DISPLAY_ON_OF = 10, //0x0100 - включить, 0x0000 - выключить, тут есть редимы, похоже на битовую маску, включают два значка REC и CPU^
    DISPLAY_BOTTOM_LINE_ICONS = 96, //тут очень много вариантов, я пока не понял может битовая маска
    CPU_TEMP = 32,
    CPU_RPM = 64,
    CPU_RPM_PERC = 74, //0x0300 0x03 (процентная шкала от 1 до 10 (0x0a)
    CPU_SCALETYPE = 48, // 0 (не понятно) 1 - делитель 10, 40000 / 10 = 4 Ghz, вариантов много 10 самый удобный (0x0a00)
    CPU_FREQ = 49, //cpu freq надо делить на  16000 в мегагерцах с точкой //странно но снва все изменилось теперь делитель 2,5 вероятно где-то есть настройка делимости
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


int writeData( libusb_device_handle *devhandler, unsigned char *lpReportBuffer1){
   return libusb_control_transfer(devhandler,LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_OUT, 0x09, 0x0301 , 0, lpReportBuffer1, 8, 0); // SET REPORT WORKED
}

int readData( libusb_device_handle *devhandler, unsigned char *lpReportBuffer1){
    return libusb_control_transfer(devhandler,LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_IN, 0x01, 0x0301 , 0, lpReportBuffer1, 8, 0); // SET REPORT WORKED
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

    writeData(devhandler,lpReportBuffer1);
    readData(devhandler,lpReportBuffer1);
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
    QCoreApplication a(argc, argv);

    libusb_context * ctx = nullptr;
    libusb_init(&ctx);

    libusb_device **list = nullptr;

    if(list != nullptr){
       libusb_free_device_list(list,0);
    }

   ssize_t count = libusb_get_device_list(nullptr,&list);
   libusb_device_handle *devhandler = nullptr;

    for(int i = 0; i < count; i++){
        libusb_device *dev = list[i];
        libusb_device_descriptor desc = {0};
        if(dev != nullptr) {
            int rc = libusb_get_device_descriptor(dev, &desc);
            if(rc == 0){

                if((int)desc.idVendor == 0x1770 && (int)desc.idProduct == 0xef35){
                    int usbset = libusb_open(dev,&devhandler);
                    libusb_reset_device(devhandler);
                    if( usbset == LIBUSB_SUCCESS){
                        if( libusb_kernel_driver_active(devhandler,0) ){
                            libusb_detach_kernel_driver(devhandler,0);
                        }
                        libusb_claim_interface(devhandler, 0);
                        libusb_set_configuration(devhandler,0);
                        libusb_clear_halt(devhandler,0x03);                        
                        libusb_clear_halt(devhandler,0x82);                     

                        writeCommand(CPU_SCALETYPE,10,devhandler, false);

                        uint16_t num =  ( readFileValue(CPU_TEMP_PATH) / 1000 ) ;
                        writeCommand(CPU_TEMP,num,devhandler, true);

                        num = readFileValue(CPU_RPM_PATH);

                        writeCommand(CPU_RPM,num*10,devhandler, true);
                        writeCommand(CPU_RPM_PERC,((num * 100)/  CPU_RPM_MAX) / 10,devhandler);

                        num= (readFileValue(CPU_FREQ_PATH) / 1000);
                        writeCommand(CPU_FREQ,num,devhandler, true);


                        DISPLAYICONS showAllIcons;

                        showAllIcons.MUSIC = 1;
                        showAllIcons.SPORT = 1;
                        showAllIcons.RACING = 1;
                        showAllIcons.FICTION = 1;
                        showAllIcons.SHOOTING = 1;


                        writeCommand(DISPLAY_BOTTOM_LINE_ICONS,(uint8_t)showAllIcons,devhandler, false);

                        CURRENTTIME time;
                        time.hour =  QDateTime::currentDateTime().time().hour() ;
                        time.min = QDateTime::currentDateTime().time().minute();

                        writeCommand(TIME,(uint16_t)time,devhandler);
                        exit(0);

                    }
                }

            }
    }
    }


    return a.exec();
}
