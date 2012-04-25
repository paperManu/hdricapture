INCLUDEPATH += ../../../libs/OpenCV/include/ ../../../libs/boost/ ../../../libs/rgbe/include/
LIBS += -L../../../libs/OpenCV/lib/darwin/ -L../../../libs/rgbe/lib/darwin/ -L../../../libs/boost/bin.v2/libs/thread/darwin -lopencv_highgui -lopencv_core -lopencv_imgproc -lrgbe -lboost_thread

TEMPLATE = app
CONFIG += console
CONFIG -= qt

SOURCES += main.cpp \
    hdribuilder.cpp \
    camera.cpp \
    chromedsphere.cpp

HEADERS += \
    hdribuilder.h \
    camera.h \
    chromedsphere.h
