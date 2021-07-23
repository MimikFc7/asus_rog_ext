QT -= gui
QT -= gui

CONFIG += c++14

CONFIG -= app_bundle

CONFIG += -O0

QMAKE_LFLAGS += -pthread

QMAKE_CXXFLAGS += -O0
DEFINES += QT_DEPRECATED_WARNINGS


SOURCES += \
        main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


LIBS += -lusb-1.0
