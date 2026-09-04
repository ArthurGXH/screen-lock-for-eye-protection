QT += core widgets svg multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

SOURCES += \
    main.cpp \
    MainWindow.cpp \
    LockScreenDialog.cpp

HEADERS += \
    MainWindow.h \
    LockScreenDialog.h

# Windows 特定配置
win32 {
    LIBS += -luser32 -lgdi32
}

# 确保 moc 能正确处理 FloatWindow 类
CONFIG += c++11

# 添加资源文件
RC_FILE = resource.rc

# 或者使用 Windows 资源文件
win32:RC_FILE = resource.rc
