#include <QApplication>
#include <QSharedMemory>
#include <QMessageBox>
#include <QTimer>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 设置应用程序图标（用于任务栏和窗口）
    QApplication::setWindowIcon(QIcon(":/icon/1.ico"));  // 从资源文件加载

    a.setQuitOnLastWindowClosed(false);  // 关闭窗口时不退出程序

    // 设置应用程序名称，用于共享内存
    a.setApplicationName("RestReminder");
    a.setApplicationVersion("1.12");
    
    // 创建共享内存，用于检测是否已有实例在运行
    QSharedMemory sharedMemory("RestReminder_SingleInstance_Key");
    
    if (sharedMemory.attach()) {
        // 如果能够附加到共享内存，说明已有实例在运行
        // 尝试激活已有实例的主窗口
        QMessageBox::information(nullptr, "提示", "程序已经在运行中！");
        return 0;
    }
    
    // 创建共享内存，锁定以表示当前实例正在运行
    if (!sharedMemory.create(1)) {
        QMessageBox::critical(nullptr, "错误", "无法创建共享内存，程序可能已经崩溃。");
        return 1;
    }

    MainWindow w;
//    w.show();
    return a.exec();
}
