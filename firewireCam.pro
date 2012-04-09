INCLUDEPATH += ../../libs/OpenCV/include/ ../../libs/boost/ ../../libs/rgbe/include/
LIBS += -L../../libs/OpenCV/lib/ -L../../libs/rgbe/lib/ -lopencv_highgui -lopencv_core -lopencv_imgproc -lrgbe

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
