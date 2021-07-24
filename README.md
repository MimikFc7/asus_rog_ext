## asus_rog_ext
Asus ROG_EXT Fron panel protocol

### Вступление

Был у меня реобас от асуса, Asus Rog Front Panel как на картинке 
![port](https://github.com/MimikFc7/asus_rog_ext/blob/main/asus_rog_front_base_dual-bay_gaming_panel_3d.jpg)

asus_rog_front_base_dual-bay_gaming_panel_3d.jpg
Шло время, я сменил материнку, и как-то на него забил, так как на новой материнской плате нет такого разъема.

Недавно наткнулся на него и решил прикрутить, так как я чаще сижу в Linux, была сделана попытка спросить у асуса как это подключить
На что был ласково послан в дальнее пешее, тем самым Асус меня сильно опечалил.

На просторах иннтернета нарыл ссылку на последнюю прошивку от асуса для данного реобаса. 

https://www.asus.com/me-en/SupportOnly/ROG%20Front%20Base/HelpDesk_Download/


Как выяснилось, ROG_EXT это обычный USB разем. подключается спокойно и определяется как положено.

На просторах интрнета нашел такие картинки, с описанием пинов, вдруг кому-то надо будет.

//тут фотки как подключить

![port](https://github.com/MimikFc7/asus_rog_ext/blob/main/20200215_023012.jpg)
![port](https://github.com/MimikFc7/asus_rog_ext/blob/main/20200215_023037.jpg)



Дальше было интересно, устройство определилось, но как с ним работать, было не ясно. 

скачав прошивку, смотрим, оказывается прошивальщик написан на C#, декомпилим смотрим. что есть набор команд, запрос версии и рабочий режим.

методом тыка и перебора, были вычленены нужные команды


### Сборка

для сборки есть 2 пути.

* 1. Статика, для статики, надо купить(ха-ха) лицензию у QT но мы же не на продажу, поэтому можно. 
       Для статики придется собрать много либ в статику и залинковать их, если будет такая нужда, пишите в Issues - помогу.
* 2. Простой режим, качаем динамическую QT 5.11 и выше и либу libusb 1.0, собственно все.


### Для сборки динамики ( по замечанию, opachgi )
       qmake qtrogextdriver.pro
       make


### Установка
    Для установки, надо в systemd установить файлик qtrogextdriver.service предваритель отредактировав, указать путь расположения файла
    ExecStart=/home/programs/system/qtrogextdriver start
    ExecStop=/home/programs/system/qtrogextdriver stop
    
    /home/programs/system/qtrogextdriver - путь до сервиса после компиляции
    
### Необходимые утилиты
* lm_sensors qmake gcc g++ 

### Для видеокарт NVIDIA
       На просторах интернета найдены вот такие опции, которые нам крайне помогли
##### Включить режим ручного управления картой (оверклок)
*       nvidia-xconfig -a --cool-bits=28 --allow-empty-initial-configuration
##### Устанавливаем режим управления куллерами (ручной)
*       nvidia-settings -a '[gpu:0]/GPUFanControlState=1                      
##### Устанавливаем скорость вентилятора, ставится в процентах, some number от 0 до 100
*       nvidia-settings -a '[fan]/GPUTargetFanSpeed=<some number>
       
##### Запрос температуры видеокарты также можно запросить процент вращения куллера fan.speed
*       nvidia-smi --query-gpu=temperature.gpu
       
##### Запрос скорости вращения вентиляторов
*       nvidia-settings --terse --query [fan:0]/GPUCurrentFanSpeedRPM         


### Описание

       #define CPU_FREQ_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq" 
       Путь до вывода текущей частоты процессора (вероятно этот путь может быть другим)
       
       #define CPU_TEMP_PATH "/sys/devices/platform/coretemp.0/hwmon/hwmon3/temp1_input"
       Путь до вывода текущей температуры процессора (вероятно этот путь может быть другим)
       
       #define CPU_RPM_PATH "/sys/class/hwmon/hwmon4/fan1_input"
       Путь до вывода текущей скорости куллера процессора (вероятно этот путь может быть другим)
       
       #define CPU_RPM_MAX 3000

    enum COMMANDTYPE : uint8_t{
        READ = 1,
        WRITE = 2,
        GET_VERSION = 16
    };

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
        INIT = 1,                         //сброс, принимает знаения, но не понятно в одном случае загорается USB во втором нет (переключение режима? )
        DISPLAY_ON_OF = 10,               //битовые режимы DISPLAYFLAGS 
        DISPLAY_BOTTOM_LINE_ICONS = 96,   // битовая маска DISPLAYICONS
        CPU_TEMP = 32,
        CPU_RPM = 64,
        CPU_RPM_PERC = 74,                //0x0300 0x03 (процентная шкала от 1 до 10 (0x0a)
        CPU_SCALETYPE = 48,               // 0 (не понятно) 1 - делитель 10, 40000 / 10 = 4 Ghz, вариантов много 10 самый удобный (0x0a00)
        CPU_FREQ = 49,                    //cpu freq надо делить на  делитель CPU_SCALETYPE
        TIME = 80                         //время бъется на 2 байта часы и минуты, 0x0d21 = 13:33 по умолчанию формат 12 часовой, но по кнопке переключается на 24
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


