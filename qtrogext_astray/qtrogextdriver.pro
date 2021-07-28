QT += gui widgets

CONFIG += c++14

CONFIG -= app_bundle

CONFIG += -O0

QMAKE_LFLAGS += -pthread

#QMAKE_LFLAGS += -static-libstdc++ -static-libgcc -static #для статичной сборки

QMAKE_CXXFLAGS += -O0
DEFINES += QT_DEPRECATED_WARNINGS


SOURCES += \
        main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


LIBS += -lusb-1.0


##Для статичной сборки надо будет много статичных либ собрать
#unix:!macx: LIBS += -L$$PWD/../../../../../../../opt/icu/icu/icu4c/source/lib/ -licui18n

#INCLUDEPATH += $$PWD/../../../../../../../opt/icu/icu/icu4c/source/common
#DEPENDPATH += $$PWD/../../../../../../../opt/icu/icu/icu4c/source/common

#unix:!macx: PRE_TARGETDEPS += $$PWD/../../../../../../../opt/icu/icu/icu4c/source/lib/libicui18n.a

#unix:!macx: LIBS += -L$$PWD/../../../../../../../opt/dbus/dbus1/dbus-1.12.10/dbus/.libs/ -ldbus-1

#INCLUDEPATH += $$PWD/../../../../../../../opt/dbus/dbus1/dbus-1.12.10/dbus/.libs
#DEPENDPATH += $$PWD/../../../../../../../opt/dbus/dbus1/dbus-1.12.10/dbus/.libs

#unix:!macx: PRE_TARGETDEPS += $$PWD/../../../../../../../opt/dbus/dbus1/dbus-1.12.10/dbus/.libs/libdbus-1.a


#unix:!macx: LIBS += -L$$PWD/../../../../../../../opt/libuv/libuv/.libs/ -luv

#INCLUDEPATH += $$PWD/../../../../../../../opt/libuv/libuv/include
#DEPENDPATH += $$PWD/../../../../../../../opt/libuv/libuv/include

#unix:!macx: PRE_TARGETDEPS += $$PWD/../../../../../../../opt/libuv/libuv/.libs/libuv.a



#unix:!macx: LIBS += -L/home/work/libs/libusb/libusb/.libs/ -lusb-1.0

#INCLUDEPATH += /home/work/libs/libusb/libusb
#DEPENDPATH += /home/work/libs/libusb/libusb

#unix:!macx: PRE_TARGETDEPS += /home/work/libs/libusb/libusb/.libs/libusb-1.0.a
