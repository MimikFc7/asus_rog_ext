# asus_rog_ext
Asus ROG_EXT Fron panel protocol

# Вступление

Был у меня реобас от асуса, Asus Rog Front Panel как на картинке asus_rog_front_base_dual-bay_gaming_panel_3d.jpg
Шло время, я сменил материнку, и как-то на него забил, так как на новой материнской плате нет такого разъема.

Недавно наткнулся на него и решил прикрутить, так как я чаще сижу в Linux, была сделана попытка спросить у асуса как это подключить
На что был ласково послан в дальнее пешее, тем самым Асус меня сильно опечалил.

На просторах иннтернета нарыл ссылку на последнюю прошивку от асуса для данного реобаса. 

https://www.asus.com/me-en/SupportOnly/ROG%20Front%20Base/HelpDesk_Download/


Как выяснилось, ROG_EXT это обычный USB разем. подключается спокойно и определяется как положено.

На просторах интрнета нашел такие картинки, с описанием пинов, вдруг кому-то надо будет.

//тут фотки как подключить

20200215_023012.jpg

20200215_023037.jpg

Дальше было интересно, устройство определилось, но как с ним работать, было не ясно. 

скачав прошивку, смотрим, оказывается прошивальщик написан на C#, декомпилим смотрим. что есть набор команд, запрос версии и рабочий режим.

методом тыка и перебора, были вычленены нужные команды


# Описание

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


