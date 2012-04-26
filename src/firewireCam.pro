INCLUDEPATH += ../../../libs/OpenCV/include/ ../../../libs/boost/ ../../../libs/rgbe/include/ ../../../libs/lcms/include/
LIBS += -L../../../libs/OpenCV/lib/darwin/ -L../../../libs/rgbe/lib/darwin/ -L../../../libs/boost/bin.v2/libs/thread/darwin -L../../../libs/lcms/lib/darwin/ -lopencv_highgui -lopencv_core -lopencv_imgproc -lrgbe -lboost_thread -llcms2

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
