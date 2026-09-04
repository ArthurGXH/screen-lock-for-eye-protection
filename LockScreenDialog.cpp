#include "LockScreenDialog.h"
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QCloseEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDebug>
#include <QDesktopWidget>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>

LockScreenDialog::LockScreenDialog(int restSeconds, QWidget *parent)
    : QDialog(parent)
    , timer(nullptr)
    , slideshowTimer(nullptr)
    , restSeconds(restSeconds)
    , currentImageIndex(0)
    , isFlashing(false)
{
    qDebug() << "LockScreenDialog 构造函数，休息秒数:" << restSeconds;
    
    messageText = "请休息";
    tipText = "🔒 系统已锁定 - 请放松眼睛和身体 🔒";
    
    loadBackgroundImages();
    setupUI();
    updateTimeDisplay();
    
    // 安装事件过滤器到应用程序
    qApp->installEventFilter(this);
}

LockScreenDialog::~LockScreenDialog()
{
    qDebug() << "LockScreenDialog 析构函数";
    if (timer) {
        timer->stop();
        delete timer;
        timer = nullptr;
    }
    if (slideshowTimer) {
        slideshowTimer->stop();
        delete slideshowTimer;
        slideshowTimer = nullptr;
    }
    // 移除事件过滤器
    qApp->removeEventFilter(this);
}

void LockScreenDialog::loadImageList()
{
    // 获取可执行文件所在目录
    QString exePath = QCoreApplication::applicationDirPath();
    // 图片目录路径: exe目录/image
    QString imageDirPath = exePath + "/image";
    
    QDir imageDir(imageDirPath);
    
    if (!imageDir.exists()) {
        qDebug() << "图片目录不存在:" << imageDirPath;
        return;
    }
    
    // 获取所有图片文件（支持常见格式）
    QStringList filters;
    filters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.gif";
    QFileInfoList fileInfoList = imageDir.entryInfoList(filters, QDir::Files);
    
    if (fileInfoList.isEmpty()) {
        qDebug() << "图片目录中没有找到图片文件:" << imageDirPath;
        return;
    }
    
    qDebug() << "找到" << fileInfoList.size() << "张图片";
    
    // 加载所有图片
    backgroundImages.clear();
    for (const QFileInfo &fileInfo : fileInfoList) {
        QString imagePath = fileInfo.absoluteFilePath();
        QPixmap pixmap;
        if (pixmap.load(imagePath)) {
            backgroundImages.append(pixmap);
            qDebug() << "成功加载图片:" << imagePath;
        } else {
            qDebug() << "加载图片失败:" << imagePath;
        }
    }
    
    // 如果有图片，设置当前图片为第一张
    if (!backgroundImages.isEmpty()) {
        currentImageIndex = 0;
        backgroundImage = backgroundImages[currentImageIndex];
        
        // 启动幻灯片计时器（每6秒切换一张）
        if (!slideshowTimer) {
            slideshowTimer = new QTimer(this);
            connect(slideshowTimer, &QTimer::timeout, this, &LockScreenDialog::switchBackgroundImage);
        }
        slideshowTimer->start(6000); // 6秒切换一次
        qDebug() << "幻灯片计时器已启动，每6秒切换一次";
    }
}

void LockScreenDialog::loadBackgroundImages()
{
    loadImageList();
    
    // 如果没有加载到任何图片，尝试加载默认图片 1.jpg
    if (backgroundImages.isEmpty()) {
        QString exePath = QCoreApplication::applicationDirPath();
        QString imagePath = exePath + "/image/1.jpg";
        
        QFileInfo imageFile(imagePath);
        if (imageFile.exists() && imageFile.isFile()) {
            QPixmap pixmap;
            if (pixmap.load(imagePath)) {
                backgroundImages.append(pixmap);
                backgroundImage = pixmap;
                qDebug() << "成功加载默认背景图片:" << imagePath;
            } else {
                qDebug() << "加载默认背景图片失败:" << imagePath;
                backgroundImage = QPixmap();
            }
        } else {
            qDebug() << "默认背景图片不存在:" << imagePath;
            backgroundImage = QPixmap();
        }
    }
}

void LockScreenDialog::switchBackgroundImage()
{
    if (backgroundImages.isEmpty()) {
        return;
    }
    
    // 切换到下一张图片
    currentImageIndex = (currentImageIndex + 1) % backgroundImages.size();
    backgroundImage = backgroundImages[currentImageIndex];
    
    // 清除缓存的缩放图片，强制重新缩放
    scaledBackground = QPixmap();
    
    qDebug() << "切换背景图片，当前索引:" << currentImageIndex;
    
    // 触发重绘
    update();
}

void LockScreenDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    
    // 窗口大小改变时重新缩放背景图片
    if (!backgroundImage.isNull()) {
        scaledBackground = backgroundImage.scaled(this->size(), 
                                                   Qt::KeepAspectRatioByExpanding, 
                                                   Qt::SmoothTransformation);
    }
    update(); // 触发重绘
}

void LockScreenDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    
    // 绘制背景
    if (!backgroundImage.isNull()) {
        // 如果缩放后的图片尺寸不对，重新缩放
        if (scaledBackground.isNull() || scaledBackground.size() != this->size()) {
            scaledBackground = backgroundImage.scaled(this->size(), 
                                                       Qt::KeepAspectRatioByExpanding, 
                                                       Qt::SmoothTransformation);
        }
        
        // 计算绘制位置（居中）
        int x = (this->width() - scaledBackground.width()) / 2;
        int y = (this->height() - scaledBackground.height()) / 2;
        
        painter.drawPixmap(x, y, scaledBackground);
    } else {
        // 如果没有背景图片，使用纯黑色背景
        painter.fillRect(this->rect(), Qt::black);
    }
    
    // 绘制文字（无背景）
    drawTexts(painter);
}

void LockScreenDialog::drawTexts(QPainter &painter)
{
    int windowWidth = this->width();
    int windowHeight = this->height();
    
    // 计算中心Y坐标
    int centerY = windowHeight / 2;
    
    // 1. 绘制"请休息"标签（无背景）
    QFont messageFont("Microsoft YaHei", 28, QFont::Bold);
    painter.setFont(messageFont);
    
    // 根据剩余时间决定颜色
    QColor messageColor;
    if (restSeconds <= 3 && isFlashing) {
        messageColor = QColor(255, 87, 34); // 橙色闪烁
    } else {
        messageColor = QColor(255, 152, 0); // 橙色 #FF9800
    }
    
    // 测量文字大小
    QRect messageRect = painter.fontMetrics().boundingRect(messageText);
    int messageWidth = messageRect.width();
    int messageHeight = messageRect.height();
    
    // 添加文字阴影效果，使文字在亮色背景下也能看清
    painter.setPen(QColor(0, 0, 0, 200));
    painter.drawText((windowWidth - messageWidth) / 2 + 2, 
                     centerY - messageHeight - 20 + 2, 
                     messageText);
    
    // 绘制主要文字
    painter.setPen(messageColor);
    painter.drawText((windowWidth - messageWidth) / 2, 
                     centerY - messageHeight - 20, 
                     messageText);
    
    // 2. 绘制倒计时标签（无背景）
    QFont timeFont("Arial", 32, QFont::Bold);
    painter.setFont(timeFont);
    
    // 根据剩余时间决定颜色
    QColor timeColor;
    if (restSeconds <= 3) {
        timeColor = QColor(255, 0, 0); // 红色
    } else if (restSeconds <= 5) {
        timeColor = QColor(255, 152, 0); // 橙色
    } else {
        timeColor = QColor(76, 175, 80); // 绿色 #4CAF50
    }
    
    // 测量文字大小
    QRect timeRect = painter.fontMetrics().boundingRect(timeText);
    int timeWidth = timeRect.width();
    int timeHeight = timeRect.height();
    
    // 添加文字阴影效果
    painter.setPen(QColor(0, 0, 0, 200));
    painter.drawText((windowWidth - timeWidth) / 2 + 2, 
                     centerY + timeHeight / 3 + 2, 
                     timeText);
    
    // 绘制主要文字
    painter.setPen(timeColor);
    painter.drawText((windowWidth - timeWidth) / 2, 
                     centerY + timeHeight / 3, 
                     timeText);
    
    // 3. 绘制提示标签（无背景）
    QFont tipFont("Microsoft YaHei", 16);
    painter.setFont(tipFont);
    
    // 测量文字大小
    QRect tipRect = painter.fontMetrics().boundingRect(tipText);
    int tipWidth = tipRect.width();
    int tipHeight = tipRect.height();
    
    // 添加文字阴影效果
    painter.setPen(QColor(0, 0, 0, 200));
    painter.drawText((windowWidth - tipWidth) / 2 + 2, 
                     centerY + tipHeight + 50 + 2, 
                     tipText);
    
    // 绘制主要文字
    painter.setPen(QColor(204, 204, 204)); // #CCCCCC
    painter.drawText((windowWidth - tipWidth) / 2, 
                     centerY + tipHeight + 50, 
                     tipText);
}

void LockScreenDialog::showEvent(QShowEvent *event)
{
    qDebug() << "LockScreenDialog showEvent，剩余秒数:" << restSeconds;
    QDialog::showEvent(event);
    
    // 确保窗口全屏并置顶
    this->showFullScreen();
    this->raise();
    this->activateWindow();
    this->setFocus();
    this->setFocusPolicy(Qt::StrongFocus);
    
    // 设置窗口为模态
    this->setModal(true);
    
    // 启动倒计时计时器
    if (!timer) {
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &LockScreenDialog::updateCountdown);
        timer->start(1000);
        qDebug() << "倒计时计时器已启动";
    }
    
    // 启动幻灯片计时器（如果还没有启动且有图片）
    if (slideshowTimer && !slideshowTimer->isActive() && backgroundImages.size() > 1) {
        slideshowTimer->start(6000);
        qDebug() << "幻灯片计时器已启动";
    }
}

void LockScreenDialog::closeEvent(QCloseEvent *event)
{
    qDebug() << "LockScreenDialog closeEvent";
    
    if (timer) {
        timer->stop();
    }
    
    if (slideshowTimer) {
        slideshowTimer->stop();
    }
    
    event->accept();
}

void LockScreenDialog::mousePressEvent(QMouseEvent *event)
{
    // 忽略所有鼠标事件，不让用户点击
    event->ignore();
}

void LockScreenDialog::mouseReleaseEvent(QMouseEvent *event)
{
    event->ignore();
}

void LockScreenDialog::mouseMoveEvent(QMouseEvent *event)
{
    event->ignore();
}

void LockScreenDialog::keyPressEvent(QKeyEvent *event)
{
    // 允许 Ctrl+Alt+Shift+Q 强制退出（用于调试）
    if (event->modifiers() == (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier) 
        && event->key() == Qt::Key_Q) {
        qDebug() << "强制退出";
        close();
    }
    // 忽略其他所有键盘事件
    event->ignore();
}

void LockScreenDialog::keyReleaseEvent(QKeyEvent *event)
{
    event->ignore();
}

bool LockScreenDialog::eventFilter(QObject *obj, QEvent *event)
{
    // 如果对话框已经关闭，不再过滤事件
    if (!this->isVisible()) {
        return QDialog::eventFilter(obj, event);
    }
    
    // 阻止所有窗口的鼠标和键盘事件（除了锁屏对话框本身）
    if (obj != this) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::Wheel:
            // 阻止其他窗口的输入事件
            return true;
        default:
            break;
        }
    }
    
    return QDialog::eventFilter(obj, event);
}

void LockScreenDialog::setupUI()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setModal(true);
    setAttribute(Qt::WA_TranslucentBackground, false);
    
    // 获取屏幕大小
    QRect screenRect = QGuiApplication::primaryScreen()->geometry();
    this->resize(screenRect.size());
}

void LockScreenDialog::updateCountdown()
{
    if (restSeconds > 0) {
        restSeconds--;
        updateTimeDisplay();
        qDebug() << "倒计时:" << restSeconds << "秒";
        
        // 处理闪烁效果
        if (restSeconds <= 3) {
            isFlashing = !isFlashing; // 每秒切换闪烁状态
        } else {
            isFlashing = false;
        }
        
        update(); // 触发重绘
    } else {
        qDebug() << "倒计时结束，关闭对话框";
        if (timer) {
            timer->stop();
        }
        if (slideshowTimer) {
            slideshowTimer->stop();
        }
        close();
    }
}

void LockScreenDialog::updateTimeDisplay()
{
    int minutes = restSeconds / 60;
    int seconds = restSeconds % 60;
    
    timeText = QString("%1:%2")
                        .arg(minutes, 2, 10, QChar('0'))
                        .arg(seconds, 2, 10, QChar('0'));
}