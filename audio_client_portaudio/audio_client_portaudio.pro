#-------------------------------------------------
#
# Project created by QtCreator 2023-02-01T20:55:57
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += network
QT += multimedia
QT += core-private

TARGET = audio_client_portaudio
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp

HEADERS += \
    main.h




win32: {
 INCLUDEPATH += $$PWD/portaudio/include/
# LIBS += -L$$PWD/portaudio/x64 -lportaudio.dll
 LIBS += -L$$PWD/portaudio/x86 -lportaudio.dll
}

#win32:contains(QMAKE_HOST.arch, x86_64) {
# message (PROCESSOR_x64)
# INCLUDEPATH += $$PWD/portaudio/include/
# LIBS += -L$$PWD/portaudio/x64 -lportaudio.dll
#}
#else {
#  message (PROCESSOR_x32)
#  INCLUDEPATH += $$PWD/portaudio/include/
#  LIBS += -L$$PWD/portaudio/x86 -lportaudio.dll
#}

android {
 INCLUDEPATH += $$PWD/portaudio/include/
 LIBS += -L$$PWD/portaudio/armeabi-v7a -lportaudio
 LIBS += -L$$PWD/portaudio/arm64-v8a -lportaudio
 ANDROID_EXTRA_LIBS = $$PWD/portaudio/armeabi-v7a/libportaudio.so
 ANDROID_EXTRA_LIBS += $$PWD/portaudio/arm64-v8a/libportaudio.so
}

#CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    images.qrc

android {
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

    DISTFILES += \
        android/AndroidManifest.xml \
        android/build.gradle \
        android/res/values/libs.xml
}


win32:{
    RC_ICONS = $$PWD/123.ico
    VERSION = 1.0.0
#    QMAKE_TARGET_COMPANY = 
#    QMAKE_TARGET_PRODUCT = 
#    QMAKE_TARGET_DESCRIPTION = 
#    QMAKE_TARGET_COPYRIGHT = 
}


