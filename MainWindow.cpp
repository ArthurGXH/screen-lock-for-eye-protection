#include "MainWindow.h"
#include "LockScreenDialog.h"
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include <QPixmap>
#include <QApplication>
#include <QDir>
#include <QCoreApplication>
#include <QSettings>
#include <QMouseEvent>
#include <QScreen>
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QFile>
#include <QFileInfo>
#include <QIntValidator>
#include <QProcess>
#include <QStandardPaths>
#include <QDateTime>
#include <QColorDialog>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// 浮动小窗口类
class FloatWindow : public QWidget
{
    Q_OBJECT

public:
    FloatWindow(QWidget *parent = nullptr) : QWidget(parent), dragging(false), remainingPercent(100)
    {
        // 初始化颜色（默认值）
        m_bgColor = QColor(211, 227, 253);  // #D3E3FD
        m_fontColor = QColor(234, 63, 247); // #EA3FF7
        
        // 修改窗口标志，确保始终在最上层
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::WindowDoesNotAcceptFocus);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);  // 显示时不激活窗口
        setFixedSize(100, 35);
        
        setStyleSheet("QWidget { background-color: transparent; }");
        
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(3, 3, 3, 3);
        
        timeLabel = new QLabel("00:00", this);
        timeLabel->setAlignment(Qt::AlignCenter);
        timeLabel->setStyleSheet(QString("color: %1; font-size: 22px; font-weight: bold; font-family: monospace; background-color: transparent;").arg(m_fontColor.name()));
        
        layout->addWidget(timeLabel);
        
        loadSavedPosition();
        
        // 启动定时器确保窗口在最上层
        topMostTimer = new QTimer(this);
        connect(topMostTimer, &QTimer::timeout, this, &FloatWindow::ensureTopMost);
        topMostTimer->start(1000);  // 每秒检查一次
    }
    
    ~FloatWindow()
    {
        if (topMostTimer) {
            topMostTimer->stop();
        }
    }
    
    void setTime(const QString &time)
    {
        timeLabel->setText(time);
    }
    
    void setRemainingPercent(int percent)
    {
        remainingPercent = qBound(0, percent, 100);
        update();
    }
    
    void setColors(const QColor &bgColor, const QColor &fontColor)
    {
        m_bgColor = bgColor;
        m_fontColor = fontColor;
        update();
        
        if (timeLabel) {
            timeLabel->setStyleSheet(QString("color: %1; font-size: 22px; font-weight: bold; font-family: monospace; background-color: transparent;").arg(fontColor.name()));
        }
    }
    
    void ensureVisibleAndTop()
    {
        if (!isVisible()) {
            show();
        }
        
        // 强制设置窗口标志
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::WindowDoesNotAcceptFocus);
        show();
        
        // 确保在最上层
        raise();
        
#ifdef Q_OS_WIN
        // Windows 特定：使用 SetWindowPos 确保最上层
        HWND hwnd = reinterpret_cast<HWND>(winId());
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
    }
    
signals:
    void doubleClicked();
    void positionChanged();

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        int whiteFillWidth = static_cast<int>(width() * (100 - remainingPercent) / 100.0);
        int radius = 6;
        
        QPainterPath fullPath;
        fullPath.addRoundedRect(rect(), radius, radius);
        
        // 使用保存的背景颜色
        QLinearGradient fullGradient(0, 0, 0, height());
        QColor startColor = m_bgColor;
        QColor endColor = m_bgColor.darker(110);
        fullGradient.setColorAt(0, startColor);
        fullGradient.setColorAt(1, endColor);
        painter.fillPath(fullPath, fullGradient);
        
        if (whiteFillWidth > 0) {
            QRect fillRect(width() - whiteFillWidth, 0, whiteFillWidth, height());
            painter.setClipPath(fullPath);
            painter.fillRect(fillRect, QColor(255, 255, 255, 230));
            painter.setClipping(false);
        }
        
        QPen pen(QColor(100, 100, 100), 1);
        painter.setPen(pen);
        painter.drawPath(fullPath);
    }
    
    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        Q_UNUSED(event);
        emit doubleClicked();
    }
    
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            dragging = true;
            dragPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }
    
    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - dragPosition);
            event->accept();
        }
    }
    
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            dragging = false;
            emit positionChanged();
            event->accept();
        }
    }
    
    void showEvent(QShowEvent *event) override
    {
        QWidget::showEvent(event);
        // 显示时确保在最上层
        QTimer::singleShot(50, this, &FloatWindow::ensureTopMost);
    }
    
private slots:
    void ensureTopMost()
    {
        if (isVisible()) {
            // 确保窗口标志正确
            Qt::WindowFlags flags = windowFlags();
            if (!(flags & Qt::WindowStaysOnTopHint)) {
                setWindowFlags(flags | Qt::WindowStaysOnTopHint);
                show();
            }
            
            // 提升到最上层
            raise();
            
#ifdef Q_OS_WIN
            // Windows 特定：保持最上层
            HWND hwnd = reinterpret_cast<HWND>(winId());
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
        }
    }
    
private:
    void loadSavedPosition()
    {
        QString appDir = QCoreApplication::applicationDirPath();
        QString configPath = appDir + "/config.ini";
        QSettings settings(configPath, QSettings::IniFormat);
        
        settings.beginGroup("FloatWindow");
        int x = settings.value("x", -1).toInt();
        int y = settings.value("y", -1).toInt();
        settings.endGroup();
        
        if (x >= 0 && y >= 0) {
            QScreen *screen = QGuiApplication::primaryScreen();
            if (screen) {
                QRect screenGeometry = screen->availableGeometry();
                if (x >= screenGeometry.left() && x <= screenGeometry.right() - width() &&
                    y >= screenGeometry.top() && y <= screenGeometry.bottom() - height()) {
                    move(x, y);
                    return;
                }
            }
        }
        
        calculateInitialPosition();
    }
    
    void calculateInitialPosition()
    {
        QScreen *screen = QGuiApplication::primaryScreen();
        if (!screen) return;
        
        QRect screenGeometry = screen->availableGeometry();
        int x = screenGeometry.width() - this->width() - 20;
        int y = screenGeometry.bottom() - this->height() - 50;
        move(x, y);
    }
    
    bool dragging;
    QPoint dragPosition;
    QLabel *timeLabel;
    int remainingPercent;
    QTimer *topMostTimer;
    QColor m_bgColor;
    QColor m_fontColor;
};

// 提醒窗口类
class RemindWindow : public QWidget
{
    Q_OBJECT

public:
    RemindWindow(int remainingSeconds, int remainingDelayCount, QWidget *parent = nullptr) 
        : QWidget(parent), remainingSeconds(remainingSeconds), remainingDelayCount(remainingDelayCount)
    {
        // 确保提醒窗口在最上层
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Dialog | Qt::WindowDoesNotAcceptFocus);
        setAttribute(Qt::WA_TranslucentBackground);
        setFixedSize(400, 220);
        
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(25, 25, 25, 25);
        mainLayout->setSpacing(12);
        
        QLabel *titleLabel = new QLabel("⚠ 即将休息 ⚠", this);
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setStyleSheet("color: #FF9800; font-size: 18px; font-weight: bold; background-color: transparent;");
        
        timeLabel = new QLabel(this);
        timeLabel->setAlignment(Qt::AlignCenter);
        timeLabel->setStyleSheet("color: #FF0000; font-size: 42px; font-weight: bold; font-family: monospace; background-color: transparent;");
        
        delayCountLabel = new QLabel(this);
        delayCountLabel->setAlignment(Qt::AlignCenter);
        delayCountLabel->setStyleSheet("color: #666666; font-size: 12px; background-color: transparent;");
        updateDelayCountLabel();
        
        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->setSpacing(18);
        
        QPushButton *restNowBtn = new QPushButton("立即休息", this);
        restNowBtn->setFixedSize(100, 40);
        restNowBtn->setStyleSheet(
            "QPushButton { background-color: #f44336; color: white; font-size: 14px; font-weight: bold; border-radius: 8px; border: none; }"
            "QPushButton:hover { background-color: #da190b; }"
        );
        
        delay5Btn = new QPushButton("延后5分", this);
        delay5Btn->setFixedSize(100, 40);
        delay5Btn->setStyleSheet(
            "QPushButton { background-color: #2196F3; color: white; font-size: 14px; font-weight: bold; border-radius: 8px; border: none; }"
            "QPushButton:hover { background-color: #1976D2; }"
        );
        
        delay8Btn = new QPushButton("延后8分", this);
        delay8Btn->setFixedSize(100, 40);
        delay8Btn->setStyleSheet(
            "QPushButton { background-color: #4CAF50; color: white; font-size: 14px; font-weight: bold; border-radius: 8px; border: none; }"
            "QPushButton:hover { background-color: #45a049; }"
        );
        
        if (remainingDelayCount <= 0) {
            delay5Btn->setEnabled(false);
            delay8Btn->setEnabled(false);
            delay5Btn->setStyleSheet(
                "QPushButton { background-color: #cccccc; color: #666666; font-size: 14px; font-weight: bold; border-radius: 8px; border: none; }"
            );
            delay8Btn->setStyleSheet(
                "QPushButton { background-color: #cccccc; color: #666666; font-size: 14px; font-weight: bold; border-radius: 8px; border: none; }"
            );
        }
        
        btnLayout->addWidget(restNowBtn);
        btnLayout->addWidget(delay5Btn);
        btnLayout->addWidget(delay8Btn);
        
        mainLayout->addWidget(titleLabel);
        mainLayout->addWidget(timeLabel);
        mainLayout->addWidget(delayCountLabel);
        mainLayout->addLayout(btnLayout);
        
        connect(restNowBtn, &QPushButton::clicked, this, &RemindWindow::onRestNow);
        connect(delay5Btn, &QPushButton::clicked, this, &RemindWindow::onDelay5);
        connect(delay8Btn, &QPushButton::clicked, this, &RemindWindow::onDelay8);
        
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &RemindWindow::updateTime);
        timer->start(1000);
        updateTime();
        
        QRect screenRect = QGuiApplication::primaryScreen()->geometry();
        move(screenRect.center().x() - width()/2, screenRect.center().y() - height()/2);
        
        // 确保窗口在最上层
        raise();
#ifdef Q_OS_WIN
        HWND hwnd = reinterpret_cast<HWND>(winId());
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
    }
    
    ~RemindWindow()
    {
        if (timer) timer->stop();
    }
    
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        QPainterPath path;
        path.addRoundedRect(rect(), 15, 15);
        painter.fillPath(path, QColor(30, 30, 30, 240));
        
        QPen pen(QColor(255, 152, 0), 2);
        painter.setPen(pen);
        painter.drawPath(path);
    }
    
    void showEvent(QShowEvent *event) override
    {
        QWidget::showEvent(event);
        // 显示时确保在最上层
        raise();
        activateWindow();
    }
    
signals:
    void restNow();
    void delay5Minutes();
    void delay8Minutes();
    
private slots:
    void updateTime()
    {
        if (remainingSeconds > 0) {
            remainingSeconds--;
            timeLabel->setText(QString("%1 秒").arg(remainingSeconds));
        } else {
            timer->stop();
            emit restNow();
            close();
        }
    }
    
    void onRestNow()
    {
        timer->stop();
        emit restNow();
        close();
    }
    
    void onDelay5()
    {
        timer->stop();
        emit delay5Minutes();
        close();
    }
    
    void onDelay8()
    {
        timer->stop();
        emit delay8Minutes();
        close();
    }
    
private:
    void updateDelayCountLabel()
    {
        if (remainingDelayCount <= 0) {
            delayCountLabel->setText("⚠ 已无延后机会，请立即休息 ⚠");
        } else {
            delayCountLabel->setText(QString("剩余延后次数: %1").arg(remainingDelayCount));
        }
    }
    
    QLabel *timeLabel;
    QLabel *delayCountLabel;
    QTimer *timer;
    int remainingSeconds;
    int remainingDelayCount;
    QPushButton *delay5Btn;
    QPushButton *delay8Btn;
};

QIcon MainWindow::loadAppIcon()
{
    QIcon appIcon;
    QString appDir = QCoreApplication::applicationDirPath();
    QString iconPath = appDir + "/icon/1.0.png";
    
    if (QFile::exists(iconPath)) {
        QPixmap pixmap;
        if (pixmap.load(iconPath)) {
            QPixmap scaledPixmap = pixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            appIcon = QIcon(scaledPixmap);
        }
    }
    
    if (appIcon.isNull()) {
        QPixmap defaultPixmap(32, 32);
        defaultPixmap.fill(Qt::green);
        appIcon = QIcon(defaultPixmap);
    }
    
    return appIcon;
}

QString MainWindow::getConfigFilePath()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString configPath = appDir + "/config.ini";
    return configPath;
}

void MainWindow::saveFloatWindowPosition(const QPoint &pos)
{
    QString configPath = getConfigFilePath();
    QSettings settings(configPath, QSettings::IniFormat);
    
    settings.beginGroup("FloatWindow");
    settings.setValue("x", pos.x());
    settings.setValue("y", pos.y());
    settings.endGroup();
}

QPoint MainWindow::loadFloatWindowPosition()
{
    QString configPath = getConfigFilePath();
    
    if (!QFile::exists(configPath)) {
        return QPoint(-1, -1);
    }
    
    QSettings settings(configPath, QSettings::IniFormat);
    
    settings.beginGroup("FloatWindow");
    int x = settings.value("x", -1).toInt();
    int y = settings.value("y", -1).toInt();
    settings.endGroup();
    
    if (x >= 0 && y >= 0) {
        return QPoint(x, y);
    }
    
    return QPoint(-1, -1);
}

void MainWindow::saveTimeSettings()
{
    if (m_isUpdatingSettings) return;
    
    QString configPath = getConfigFilePath();
    QSettings settings(configPath, QSettings::IniFormat);
    
    settings.beginGroup("TimerSettings");
    settings.setValue("workMinutes", workMinutesEdit->text().toInt());
    settings.setValue("workSeconds", workSecondsEdit->text().toInt());
    settings.setValue("restMinutes", restMinutesEdit->text().toInt());
    settings.setValue("restSeconds", restSecondsEdit->text().toInt());
    settings.endGroup();
}

void MainWindow::loadTimeSettings()
{
    QString configPath = getConfigFilePath();
    
    if (!QFile::exists(configPath)) {
        m_isUpdatingSettings = true;
        workMinutesEdit->setText("75");
        workSecondsEdit->setText("0");
        restMinutesEdit->setText("1");
        restSecondsEdit->setText("0");
        m_isUpdatingSettings = false;
        return;
    }
    
    QSettings settings(configPath, QSettings::IniFormat);
    
    settings.beginGroup("TimerSettings");
    int workMin = settings.value("workMinutes", 75).toInt();
    int workSec = settings.value("workSeconds", 0).toInt();
    int restMin = settings.value("restMinutes", 1).toInt();
    int restSec = settings.value("restSeconds", 0).toInt();
    settings.endGroup();
    
    m_isUpdatingSettings = true;
    workMinutesEdit->setText(QString::number(workMin));
    workSecondsEdit->setText(QString::number(workSec));
    restMinutesEdit->setText(QString::number(restMin));
    restSecondsEdit->setText(QString::number(restSec));
    m_isUpdatingSettings = false;
}

void MainWindow::saveColorSettings()
{
    QString configPath = getConfigFilePath();
    QSettings settings(configPath, QSettings::IniFormat);
    
    settings.beginGroup("FloatWindowColors");
    settings.setValue("bgColor", floatBgColor.name());
    settings.setValue("fontColor", floatFontColor.name());
    settings.endGroup();
}

void MainWindow::loadColorSettings()
{
    QString configPath = getConfigFilePath();
    QSettings settings(configPath, QSettings::IniFormat);
    
    settings.beginGroup("FloatWindowColors");
    QString bgColorStr = settings.value("bgColor", "#D3E3FD").toString();
    QString fontColorStr = settings.value("fontColor", "#EA3FF7").toString();
    settings.endGroup();
    
    floatBgColor = QColor(bgColorStr);
    floatFontColor = QColor(fontColorStr);
    
    // 更新预览
    bgColorPreview->setStyleSheet(QString("border: 1px solid #cccccc; border-radius: 4px; background-color: %1;").arg(bgColorStr));
    fontColorPreview->setStyleSheet(QString("border: 1px solid #cccccc; border-radius: 4px; background-color: %1;").arg(fontColorStr));
    
    // 应用颜色到浮动窗口
    applyFloatWindowColors();
}

void MainWindow::applyFloatWindowColors()
{
    if (floatWindow) {
        floatWindow->setColors(floatBgColor, floatFontColor);
    }
}

// 使用启动文件夹方式替代注册表
void MainWindow::saveAutoStartSetting()
{
    QString appPath = QCoreApplication::applicationFilePath();
    QString startupPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + "/Startup";
    QString shortcutPath = startupPath + "/RestReminder.lnk";
    
    bool enable = autoStartCheckBox->isChecked();
    
    if (enable) {
        // 创建启动快捷方式
        QFile::remove(shortcutPath);
        
        // 使用 PowerShell 创建快捷方式
        QString psScript = QString(
            "$WScriptShell = New-Object -ComObject WScript.Shell; "
            "$Shortcut = $WScriptShell.CreateShortcut('%1'); "
            "$Shortcut.TargetPath = '%2'; "
            "$Shortcut.WorkingDirectory = '%3'; "
            "$Shortcut.Save()"
        ).arg(shortcutPath).arg(appPath).arg(QCoreApplication::applicationDirPath());
        
        QProcess::startDetached("powershell", QStringList() << "-Command" << psScript);
    } else {
        // 删除启动快捷方式
        QFile::remove(shortcutPath);
    }
}

void MainWindow::loadAutoStartSetting()
{
    QString startupPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + "/Startup";
    QString shortcutPath = startupPath + "/RestReminder.lnk";
    bool isAutoStart = QFile::exists(shortcutPath);
    
    m_isUpdatingSettings = true;
    autoStartCheckBox->setChecked(isAutoStart);
    m_isUpdatingSettings = false;
}

void MainWindow::resetDelayCount()
{
    delayCount = 0;
}

void MainWindow::playVoice(const QString &fileName)
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString voicePath = appDir + "/voice/" + fileName;
    
    if (QFile::exists(voicePath)) {
        if (voicePlayer) {
            voicePlayer->stop();
            delete voicePlayer;
        }
        
        voicePlayer = new QMediaPlayer(this);
        voicePlayer->setMedia(QUrl::fromLocalFile(voicePath));
        voicePlayer->play();
    } else {
        qDebug() << "Voice file not found:" << voicePath;
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , workTimer(new QTimer(this))
    , restTimer(new QTimer(this))
    , countdownTimer(new QTimer(this))
    , floatTimer(new QTimer(this))
    , remindTimer(new QTimer(this))
    , isWorking(true)
    , lockDialog(nullptr)
    , trayIcon(nullptr)
    , trayMenu(nullptr)
    , floatWindow(nullptr)
    , remindWindow(nullptr)
    , isTimerStarted(false)
    , m_isUpdatingSettings(false)
    , delayCount(0)
    , voicePlayer(nullptr)
{
    setupUI();
    loadTimeSettings();
    loadAutoStartSetting();
    loadColorSettings();
    createSystemTray();
    
    QIcon appIcon = loadAppIcon();
    this->setWindowIcon(appIcon);
    
    connect(workTimer, &QTimer::timeout, this, &MainWindow::onWorkFinished);
    connect(restTimer, &QTimer::timeout, this, &MainWindow::onRestFinished);
    connect(countdownTimer, &QTimer::timeout, this, &MainWindow::updateCountdown);
    connect(floatTimer, &QTimer::timeout, this, &MainWindow::updateFloatWindow);
    connect(remindTimer, &QTimer::timeout, this, &MainWindow::showRemindWindow);
    
    QTimer::singleShot(0, this, &MainWindow::hideToTray);
    
    startTimerAutomatically();
}

MainWindow::~MainWindow()
{
    if (lockDialog) {
        lockDialog->close();
        delete lockDialog;
    }
    if (floatWindow) {
        delete floatWindow;
    }
    if (remindWindow) {
        delete remindWindow;
    }
    if (voicePlayer) {
        voicePlayer->stop();
        delete voicePlayer;
    }
}

void MainWindow::hideToTray()
{
    this->hide();
    this->setWindowFlags(this->windowFlags() | Qt::Tool);
    this->show();
    this->hide();
}

void MainWindow::startTimerAutomatically()
{
    QTimer::singleShot(500, this, [this]() {
        if (!isTimerStarted) {
            startStopTimer();
        }
    });
}

void MainWindow::setupUI()
{
    setWindowTitle("休息提醒工具-V1.19");
    setFixedSize(550, 680);  // 增加高度以容纳颜色设置控件
    setStyleSheet("QMainWindow { background-color: #f5f5f5; }");
    
    setWindowFlags(windowFlags() | Qt::Tool);
    
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    QFont globalFont = this->font();
    globalFont.setPointSize(globalFont.pointSize() + 2);
    this->setFont(globalFont);
    
    QGroupBox *settingBox = new QGroupBox("时间设置", this);
    settingBox->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 2px solid #dcdcdc; border-radius: 8px; margin-top: 10px; background-color: white; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px 0 5px; color: #2196F3; }"
    );
    
    QFont settingBoxFont = settingBox->font();
    settingBoxFont.setPointSize(settingBoxFont.pointSize() + 2);
    settingBox->setFont(settingBoxFont);
    
    QVBoxLayout *settingLayout = new QVBoxLayout(settingBox);
    settingLayout->setSpacing(15);
    settingLayout->setContentsMargins(20, 20, 20, 20);
    
    QHBoxLayout *workLayout = new QHBoxLayout();
    QLabel *workLabel = new QLabel("休息时间间隔:", this);
    workLabel->setStyleSheet("color: #333333; font-weight: bold;");
    QFont labelFont = workLabel->font();
    labelFont.setPointSize(labelFont.pointSize() + 2);
    workLabel->setFont(labelFont);
//    workLabel->setFixedWidth(150);
    
    workMinutesEdit = new QLineEdit(this);
    workMinutesEdit->setFixedWidth(65);
    workMinutesEdit->setAlignment(Qt::AlignCenter);
    workMinutesEdit->setValidator(new QIntValidator(0, 999, this));
    workMinutesEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdcdc; border-radius: 5px; padding: 5px; background-color: white; }"
        "QLineEdit:focus { border: 2px solid #2196F3; }"
    );
    
    QLabel *workUnit = new QLabel("分", this);
    workUnit->setStyleSheet("color: #333333; font-weight: bold;");
    workUnit->setFont(labelFont);
    
    workSecondsEdit = new QLineEdit(this);
    workSecondsEdit->setFixedWidth(65);
    workSecondsEdit->setAlignment(Qt::AlignCenter);
    workSecondsEdit->setValidator(new QIntValidator(0, 59, this));
    workSecondsEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdcdc; border-radius: 5px; padding: 5px; background-color: white; }"
        "QLineEdit:focus { border: 2px solid #2196F3; }"
    );
    
    QLabel *workSecUnit = new QLabel("秒", this);
    workSecUnit->setStyleSheet("color: #333333; font-weight: bold;");
    workSecUnit->setFont(labelFont);
    
    workLayout->addWidget(workLabel);
    workLayout->addWidget(workMinutesEdit);
    workLayout->addWidget(workUnit);
    workLayout->addSpacing(10);
    workLayout->addWidget(workSecondsEdit);
    workLayout->addWidget(workSecUnit);
    workLayout->addStretch();
    
    QHBoxLayout *restLayout = new QHBoxLayout();
    QLabel *restLabel = new QLabel("休息时间长度:", this);
    restLabel->setStyleSheet("color: #333333; font-weight: bold;");
    restLabel->setFont(labelFont);
//    restLabel->setFixedWidth(150);
    
    restMinutesEdit = new QLineEdit(this);
    restMinutesEdit->setFixedWidth(65);
    restMinutesEdit->setAlignment(Qt::AlignCenter);
    restMinutesEdit->setValidator(new QIntValidator(0, 999, this));
    restMinutesEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdcdc; border-radius: 5px; padding: 5px; background-color: white; }"
        "QLineEdit:focus { border: 2px solid #2196F3; }"
    );
    
    QLabel *restMinUnit = new QLabel("分", this);
    restMinUnit->setStyleSheet("color: #333333; font-weight: bold;");
    restMinUnit->setFont(labelFont);
    
    restSecondsEdit = new QLineEdit(this);
    restSecondsEdit->setFixedWidth(65);
    restSecondsEdit->setAlignment(Qt::AlignCenter);
    restSecondsEdit->setValidator(new QIntValidator(0, 59, this));
    restSecondsEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdcdc; border-radius: 5px; padding: 5px; background-color: white; }"
        "QLineEdit:focus { border: 2px solid #2196F3; }"
    );
    
    QLabel *restSecUnit = new QLabel("秒", this);
    restSecUnit->setStyleSheet("color: #333333; font-weight: bold;");
    restSecUnit->setFont(labelFont);
    
    restLayout->addWidget(restLabel);
    restLayout->addWidget(restMinutesEdit);
    restLayout->addWidget(restMinUnit);
    restLayout->addSpacing(10);
    restLayout->addWidget(restSecondsEdit);
    restLayout->addWidget(restSecUnit);
    restLayout->addStretch();
    
    settingLayout->addLayout(workLayout);
    settingLayout->addLayout(restLayout);
    
    QHBoxLayout *autoStartLayout = new QHBoxLayout();
    autoStartCheckBox = new QCheckBox("开机自动启动", this);
    autoStartCheckBox->setStyleSheet("QCheckBox { spacing: 8px; color: #333333; font-weight: bold; }");
    QFont checkBoxFont = autoStartCheckBox->font();
    checkBoxFont.setPointSize(checkBoxFont.pointSize() + 2);
    checkBoxFont.setBold(true);
    autoStartCheckBox->setFont(checkBoxFont);
    autoStartLayout->addWidget(autoStartCheckBox);
    autoStartLayout->addStretch();
    settingLayout->addLayout(autoStartLayout);
    
    QLabel *tipLabel = new QLabel("提示：双击倒计时小窗口可打开主窗口\n休息前1分钟会弹出提醒窗口\n每个工作周期最多可延后3次", this);
    QFont tipFont = tipLabel->font();
    tipFont.setPointSize(tipFont.pointSize() + 1);
    tipLabel->setFont(tipFont);
    tipLabel->setStyleSheet("color: #999999; background-color: #f9f9f9; padding: 8px; border-radius: 5px;");
    tipLabel->setAlignment(Qt::AlignCenter);
    settingLayout->addWidget(tipLabel);
    
    // ===== 颜色设置区域（使用QGridLayout实现完美对齐） =====
    QGroupBox *colorBox = new QGroupBox("倒计时窗口颜色设置", this);
    colorBox->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 2px solid #dcdcdc; border-radius: 8px; margin-top: 10px; background-color: white; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px 0 5px; color: #2196F3; }"
    );
    colorBox->setFont(settingBoxFont);
    
    QGridLayout *colorLayout = new QGridLayout(colorBox);
    colorLayout->setSpacing(10);
    colorLayout->setContentsMargins(15, 15, 15, 15);
    
    // 第一行：背景颜色
    QLabel *bgLabel = new QLabel("背景颜色:", this);
    bgLabel->setStyleSheet("color: #333333; font-weight: bold;");
    bgLabel->setFont(labelFont);
    bgLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);  // 右对齐
    
    bgColorPreview = new QLabel(this);
    bgColorPreview->setFixedSize(30, 25);
    bgColorPreview->setStyleSheet("border: 1px solid #cccccc; border-radius: 4px; background-color: #D3E3FD;");
    
    bgColorBtn = new QPushButton("选择背景颜色", this);
//    bgColorBtn->setFixedWidth(180);
    bgColorBtn->setStyleSheet(
        "QPushButton { background-color: #e3f2fd; color: #1976D2; border: 1px solid #bbdefb; border-radius: 5px; padding: 5px 12px; font-weight: bold; }"
        "QPushButton:hover { background-color: #bbdefb; }"
    );
    
    colorLayout->addWidget(bgLabel, 0, 0);
    colorLayout->addWidget(bgColorPreview, 0, 1);
    colorLayout->addWidget(bgColorBtn, 0, 2);
    colorLayout->setColumnStretch(3, 1);  // 第3列拉伸
    
    // 第二行：字体颜色
    QLabel *fontLabel = new QLabel("字体颜色:", this);
    fontLabel->setStyleSheet("color: #333333; font-weight: bold;");
    fontLabel->setFont(labelFont);
    fontLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);  // 右对齐
    
    fontColorPreview = new QLabel(this);
    fontColorPreview->setFixedSize(30, 25);
    fontColorPreview->setStyleSheet("border: 1px solid #cccccc; border-radius: 4px; background-color: #EE8AF8;");
    
    fontColorBtn = new QPushButton("选择字体颜色", this);
//    fontColorBtn->setFixedWidth(180);
    fontColorBtn->setStyleSheet(
        "QPushButton { background-color: #fce4ec; color: #c62828; border: 1px solid #f8bbd0; border-radius: 5px; padding: 5px 12px; font-weight: bold; }"
        "QPushButton:hover { background-color: #f8bbd0; }"
    );
    
    colorLayout->addWidget(fontLabel, 1, 0);
    colorLayout->addWidget(fontColorPreview, 1, 1);
    colorLayout->addWidget(fontColorBtn, 1, 2);
    
    // 第三行：恢复默认按钮（居中）
    restoreDefaultBtn = new QPushButton("恢复默认颜色", this);
//    restoreDefaultBtn->setFixedWidth(180);
    restoreDefaultBtn->setStyleSheet(
        "QPushButton { background-color: #fff3e0; color: #e65100; border: 1px solid #ffccbc; border-radius: 5px; padding: 5px 12px; font-weight: bold; }"
        "QPushButton:hover { background-color: #ffccbc; }"
    );
    
    // 创建一个水平布局来居中按钮
    QHBoxLayout *restoreLayout = new QHBoxLayout();
    restoreLayout->addSpacing(150);  // 添加固定间距，使按钮左偏移到合适位置
    restoreLayout->addWidget(restoreDefaultBtn);
    restoreLayout->addStretch();
    
    colorLayout->addLayout(restoreLayout, 2, 0, 1, 3);  // 跨3列
    
    settingLayout->addWidget(colorBox);
    // ===== 颜色设置区域结束 =====
    
    QGroupBox *statusBox = new QGroupBox("状态", this);
    statusBox->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 2px solid #dcdcdc; border-radius: 8px; margin-top: 10px; background-color: white; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px 0 5px; color: #2196F3; }"
    );
    statusBox->setFont(settingBoxFont);
    statusBox->setFixedHeight(120);
    
    QVBoxLayout *statusLayout = new QVBoxLayout(statusBox);
    statusLayout->setContentsMargins(10, 5, 10, 5);
    statusLayout->setSpacing(5);
    
    statusLabel = new QLabel("未开始", this);
    QFont statusFont = statusLabel->font();
    statusFont.setPointSize(statusFont.pointSize() + 2);
    statusFont.setBold(true);
    statusLabel->setFont(statusFont);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: #FF9800; padding: 5px;");
    
    countdownLabel = new QLabel("00:00", this);
    QFont countdownFont = countdownLabel->font();
    countdownFont.setPointSize(countdownFont.pointSize() + 8);
    countdownFont.setBold(true);
    countdownFont.setFamily("monospace");
    countdownLabel->setFont(countdownFont);
    countdownLabel->setAlignment(Qt::AlignCenter);
    countdownLabel->setStyleSheet("color: #2196F3; padding: 5px;");
    
    statusLayout->addWidget(statusLabel);
    statusLayout->addWidget(countdownLabel);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    
    startStopBtn = new QPushButton("开始计时", this);
    startStopBtn->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; border: none; border-radius: 8px; padding: 10px; font-weight: bold; }"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:pressed { background-color: #3d8b40; }"
    );
    QFont btnFont = startStopBtn->font();
    btnFont.setPointSize(btnFont.pointSize() + 2);
    btnFont.setBold(true);
    startStopBtn->setFont(btnFont);
    startStopBtn->setFixedHeight(45);
    
    restartBtn = new QPushButton("重新开始计时", this);
    restartBtn->setStyleSheet(
        "QPushButton { background-color: #FF9800; color: white; border: none; border-radius: 8px; padding: 10px; font-weight: bold; }"
        "QPushButton:hover { background-color: #F57C00; }"
        "QPushButton:pressed { background-color: #E65100; }"
    );
    restartBtn->setFont(btnFont);
    restartBtn->setFixedHeight(45);
    restartBtn->setEnabled(false);
    
    buttonLayout->addWidget(startStopBtn);
    buttonLayout->addWidget(restartBtn);
    
    mainLayout->addWidget(settingBox);
    mainLayout->addWidget(statusBox);
    mainLayout->addLayout(buttonLayout);
    
    connect(startStopBtn, &QPushButton::clicked, this, &MainWindow::startStopTimer);
    connect(restartBtn, &QPushButton::clicked, this, &MainWindow::restartTimer);
    connect(autoStartCheckBox, &QCheckBox::stateChanged, this, &MainWindow::onAutoStartChanged);
    
    connect(workMinutesEdit, &QLineEdit::textChanged, this, &MainWindow::onWorkMinutesChanged);
    connect(workSecondsEdit, &QLineEdit::textChanged, this, &MainWindow::onWorkSecondsChanged);
    connect(restMinutesEdit, &QLineEdit::textChanged, this, &MainWindow::onRestMinutesChanged);
    connect(restSecondsEdit, &QLineEdit::textChanged, this, &MainWindow::onRestSecondsChanged);
    
    // 连接颜色相关信号
    connect(bgColorBtn, &QPushButton::clicked, this, &MainWindow::onSelectBgColor);
    connect(fontColorBtn, &QPushButton::clicked, this, &MainWindow::onSelectFontColor);
    connect(restoreDefaultBtn, &QPushButton::clicked, this, &MainWindow::onRestoreDefaultColors);
}


void MainWindow::restartTimer()
{
    if (!isTimerStarted) {
        return;
    }
    
    resetAllTimers();
    
    isWorking = true;
    
    int workMin = workMinutesEdit->text().toInt();
    int workSec = workSecondsEdit->text().toInt();
    int restMin = restMinutesEdit->text().toInt();
    int restSec = restSecondsEdit->text().toInt();
    
    workSeconds = workMin * 60 + workSec;
    restSeconds = restMin * 60 + restSec;
    currentSeconds = workSeconds;
    
    resetDelayCount();
    
    workTimer->start(workSeconds * 1000);
    countdownTimer->start(1000);
    floatTimer->start(100);
    
    if (workSeconds > 60) {
        remindTimer->start((workSeconds - 60) * 1000);
    } else if (workSeconds > 0) {
        QTimer::singleShot(100, this, &MainWindow::showRemindWindow);
    }
    
    statusLabel->setText("工作间隔中");
    statusLabel->setStyleSheet("color: #4CAF50; padding: 5px; font-weight: bold;");
    updateCountdown();
    
    if (floatWindow) {
        floatWindow->ensureVisibleAndTop();
    } else {
        showFloatWindow();
    }
}

void MainWindow::onFloatWindowMoved()
{
    if (floatWindow) {
        saveFloatWindowPosition(floatWindow->pos());
    }
}

void MainWindow::onWorkMinutesChanged(const QString &text)
{
    Q_UNUSED(text);
    if (!m_isUpdatingSettings && !isTimerStarted) {
        saveTimeSettings();
    }
}

void MainWindow::onWorkSecondsChanged(const QString &text)
{
    Q_UNUSED(text);
    if (!m_isUpdatingSettings && !isTimerStarted) {
        saveTimeSettings();
    }
}

void MainWindow::onRestMinutesChanged(const QString &text)
{
    Q_UNUSED(text);
    if (!m_isUpdatingSettings && !isTimerStarted) {
        saveTimeSettings();
    }
}

void MainWindow::onRestSecondsChanged(const QString &text)
{
    Q_UNUSED(text);
    if (!m_isUpdatingSettings && !isTimerStarted) {
        saveTimeSettings();
    }
}

void MainWindow::resetAllTimers()
{
    if (workTimer && workTimer->isActive()) workTimer->stop();
    if (restTimer && restTimer->isActive()) restTimer->stop();
    if (countdownTimer && countdownTimer->isActive()) countdownTimer->stop();
    if (floatTimer && floatTimer->isActive()) floatTimer->stop();
    if (remindTimer && remindTimer->isActive()) remindTimer->stop();
    
    if (remindWindow) {
        remindWindow->blockSignals(true);
        remindWindow->close();
        remindWindow->deleteLater();
        remindWindow = nullptr;
    }
}

void MainWindow::startStopTimer()
{
    if (!isTimerStarted) {
        resetDelayCount();
        
        bool ok1, ok2, ok3, ok4;
        int workMin = workMinutesEdit->text().toInt(&ok1);
        int workSec = workSecondsEdit->text().toInt(&ok2);
        int restMin = restMinutesEdit->text().toInt(&ok3);
        int restSec = restSecondsEdit->text().toInt(&ok4);
        
        if (!ok1 || !ok2 || !ok3 || !ok4) {
            QMessageBox::warning(this, "错误", "请输入有效的时间数字");
            return;
        }
        
        int totalWorkSeconds = workMin * 60 + workSec;
        int totalRestSeconds = restMin * 60 + restSec;
        
        if (totalWorkSeconds < 5) {
            QMessageBox::warning(this, "错误", "休息时间间隔至少需要5秒");
            return;
        }
        
        if (totalRestSeconds < 5) {
            QMessageBox::warning(this, "错误", "休息时间长度至少需要5秒");
            return;
        }
        
        workSeconds = totalWorkSeconds;
        restSeconds = totalRestSeconds;
        currentSeconds = workSeconds;
        isWorking = true;
        isTimerStarted = true;
        
        resetAllTimers();
        
        workTimer->start(workSeconds * 1000);
        countdownTimer->start(1000);
        floatTimer->start(100);
        
        if (workSeconds > 60) {
            remindTimer->start((workSeconds - 60) * 1000);
        } else if (workSeconds > 0) {
            QTimer::singleShot(100, this, &MainWindow::showRemindWindow);
        }
        
        startStopBtn->setText("终止计时");
        startStopBtn->setStyleSheet(
            "QPushButton { background-color: #f44336; color: white; border: none; border-radius: 8px; padding: 10px; font-weight: bold; }"
            "QPushButton:hover { background-color: #da190b; }"
        );
        
        restartBtn->setEnabled(true);
        restartBtn->setStyleSheet(
            "QPushButton { background-color: #FF9800; color: white; border: none; border-radius: 8px; padding: 10px; font-weight: bold; }"
            "QPushButton:hover { background-color: #F57C00; }"
        );
        
        workMinutesEdit->setEnabled(false);
        workSecondsEdit->setEnabled(false);
        restMinutesEdit->setEnabled(false);
        restSecondsEdit->setEnabled(false);
        
        autoStartCheckBox->setEnabled(true);
        
        statusLabel->setText("工作间隔中");
        statusLabel->setStyleSheet("color: #4CAF50; padding: 5px; font-weight: bold;");
        updateCountdown();
        
        showFloatWindow();
    } else {
        resetAllTimers();
        isTimerStarted = false;
        
        startStopBtn->setText("开始计时");
        startStopBtn->setStyleSheet(
            "QPushButton { background-color: #4CAF50; color: white; border: none; border-radius: 8px; padding: 10px; font-weight: bold; }"
            "QPushButton:hover { background-color: #45a049; }"
        );
        
        restartBtn->setEnabled(false);
        restartBtn->setStyleSheet(
            "QPushButton { background-color: #FF9800; color: white; border: none; border-radius: 8px; padding: 10px; font-weight: bold; opacity: 0.5; }"
        );
        
        workMinutesEdit->setEnabled(true);
        workSecondsEdit->setEnabled(true);
        restMinutesEdit->setEnabled(true);
        restSecondsEdit->setEnabled(true);
        autoStartCheckBox->setEnabled(true);
        
        statusLabel->setText("已停止");
        statusLabel->setStyleSheet("color: #FF9800; padding: 5px; font-weight: bold;");
        countdownLabel->setText("00:00");
        
        if (lockDialog) {
            lockDialog->close();
            delete lockDialog;
            lockDialog = nullptr;
        }
        
        if (floatWindow) {
            floatWindow->close();
            delete floatWindow;
            floatWindow = nullptr;
        }
    }
}

void MainWindow::showRemindWindow()
{
    if (!isTimerStarted) return;
    if (remindWindow) return;
    if (!workTimer || !workTimer->isActive()) return;
    
    int remaining = workTimer->remainingTime() / 1000;
    int remainingDelayChances = MAX_DELAY_COUNT - delayCount;
    
    if (remaining <= 60 && remaining > 0 && remainingDelayChances > 0) {
        // 播放提醒语音
        playVoice("即将锁屏休息，需要延后吗？.mp3");
        
        remindWindow = new RemindWindow(remaining, remainingDelayChances, nullptr);
        
        connect(remindWindow, SIGNAL(restNow()), this, SLOT(onRestNow()), Qt::QueuedConnection);
        connect(remindWindow, SIGNAL(delay5Minutes()), this, SLOT(onDelay5Minutes()), Qt::QueuedConnection);
        connect(remindWindow, SIGNAL(delay8Minutes()), this, SLOT(onDelay8Minutes()), Qt::QueuedConnection);
        
        remindWindow->show();
        remindWindow->raise();
        remindWindow->activateWindow();
    }
}

void MainWindow::onRestNow()
{
    if (remindTimer && remindTimer->isActive()) remindTimer->stop();
    
    if (remindWindow) {
        remindWindow->blockSignals(true);
        remindWindow->close();
        remindWindow->deleteLater();
        remindWindow = nullptr;
    }
    
    if (workTimer && workTimer->isActive()) {
        workTimer->stop();
        onWorkFinished();
    }
}

void MainWindow::onDelay5Minutes()
{
    if (delayCount >= MAX_DELAY_COUNT) {
        if (workTimer && workTimer->isActive()) {
            workTimer->stop();
            onWorkFinished();
        }
        return;
    }
    
    // 播放延后5分钟语音
    playVoice("延后5分钟.mp3");
    
    if (remindTimer && remindTimer->isActive()) remindTimer->stop();
    
    if (remindWindow) {
        remindWindow->blockSignals(true);
        remindWindow->close();
        remindWindow->deleteLater();
        remindWindow = nullptr;
    }
    
    int currentRemaining = 0;
    if (workTimer && workTimer->isActive()) {
        currentRemaining = workTimer->remainingTime() / 1000;
        if (currentRemaining < 0) currentRemaining = 0;
        workTimer->stop();
    }
    
    int newRemaining = currentRemaining + 300;
    
    workSeconds = newRemaining;
    delayCount++;
    
    if (workTimer) workTimer->start(newRemaining * 1000);
    
    if (remindTimer) {
        if (newRemaining > 60) {
            remindTimer->start((newRemaining - 60) * 1000);
        } else if (newRemaining > 0) {
            QTimer::singleShot(100, this, &MainWindow::showRemindWindow);
        }
    }
    
    updateFloatWindow();
    updateCountdown();
    statusLabel->setText("工作间隔中");
}

void MainWindow::onDelay8Minutes()
{
    if (delayCount >= MAX_DELAY_COUNT) {
        if (workTimer && workTimer->isActive()) {
            workTimer->stop();
            onWorkFinished();
        }
        return;
    }
    
    // 播放延后8分钟语音
    playVoice("延后8分钟.mp3");
    
    if (remindTimer && remindTimer->isActive()) remindTimer->stop();
    
    if (remindWindow) {
        remindWindow->blockSignals(true);
        remindWindow->close();
        remindWindow->deleteLater();
        remindWindow = nullptr;
    }
    
    int currentRemaining = 0;
    if (workTimer && workTimer->isActive()) {
        currentRemaining = workTimer->remainingTime() / 1000;
        if (currentRemaining < 0) currentRemaining = 0;
        workTimer->stop();
    }
    
    int newRemaining = currentRemaining + 480;
    
    workSeconds = newRemaining;
    delayCount++;
    
    if (workTimer) workTimer->start(newRemaining * 1000);
    
    if (remindTimer) {
        if (newRemaining > 60) {
            remindTimer->start((newRemaining - 60) * 1000);
        } else if (newRemaining > 0) {
            QTimer::singleShot(100, this, &MainWindow::showRemindWindow);
        }
    }
    
    updateFloatWindow();
    updateCountdown();
    statusLabel->setText("工作间隔中");
}

void MainWindow::onAutoStartChanged(int state)
{
    if (!m_isUpdatingSettings) {
        saveAutoStartSetting();
    }
}

void MainWindow::showFloatWindow()
{
    if (!isTimerStarted) return;
    
    if (!floatWindow) {
        floatWindow = new FloatWindow();
        // 应用保存的颜色
        floatWindow->setColors(floatBgColor, floatFontColor);
        connect(floatWindow, SIGNAL(doubleClicked()), this, SLOT(showNormalWindow()));
        connect(floatWindow, SIGNAL(positionChanged()), this, SLOT(onFloatWindowMoved()));
        
        // 确保窗口可见并在最上层
        floatWindow->ensureVisibleAndTop();
    } else {
        // 如果窗口已存在但不可见，重新显示
        if (!floatWindow->isVisible()) {
            floatWindow->ensureVisibleAndTop();
        } else {
            floatWindow->show();
            floatWindow->raise();
            
#ifdef Q_OS_WIN
            HWND hwnd = reinterpret_cast<HWND>(floatWindow->winId());
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
        }
    }
    updateFloatWindow();
}

void MainWindow::updateFloatWindow()
{
    if (!floatWindow || !isTimerStarted) return;
    
    // 确保浮动窗口可见
    if (!floatWindow->isVisible()) {
        floatWindow->ensureVisibleAndTop();
    }
    
    QString timeStr;
    int remainingSeconds = 0;
    int totalSeconds = 0;
    
    if (workTimer && workTimer->isActive() && isWorking) {
        remainingSeconds = workTimer->remainingTime() / 1000;
        if (remainingSeconds < 0) remainingSeconds = 0;
        totalSeconds = workSeconds;
        
        int minutes = remainingSeconds / 60;
        int seconds = remainingSeconds % 60;
        timeStr = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    } else if (restTimer && restTimer->isActive() && !isWorking) {
        remainingSeconds = restTimer->remainingTime() / 1000;
        if (remainingSeconds < 0) remainingSeconds = 0;
        totalSeconds = restSeconds;
        
        int minutes = remainingSeconds / 60;
        int seconds = remainingSeconds % 60;
        timeStr = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    } else {
        timeStr = countdownLabel->text();
        floatWindow->setTime(timeStr);
        floatWindow->setRemainingPercent(100);
        return;
    }
    
    int percent = 0;
    if (totalSeconds > 0) {
        percent = (remainingSeconds * 100) / totalSeconds;
    }
    
    floatWindow->setTime(timeStr);
    floatWindow->setRemainingPercent(percent);
}

void MainWindow::createSystemTray()
{
    trayIcon = new QSystemTrayIcon(this);
    QIcon appIcon = loadAppIcon();
    trayIcon->setIcon(appIcon);
    trayIcon->setToolTip("休息提醒工具");
    
    trayMenu = new QMenu(this);
    QAction *showAction = trayMenu->addAction("打开主窗口");
    QAction *quitAction = trayMenu->addAction("退出");
    
    connect(showAction, &QAction::triggered, this, &MainWindow::showNormalWindow);
    connect(quitAction, &QAction::triggered, this, &MainWindow::quitApplication);
    
    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();
    
    connect(trayIcon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            showNormalWindow();
        }
    });
}

void MainWindow::showNormalWindow()
{
    this->showNormal();
    this->raise();
    this->activateWindow();
}

void MainWindow::quitApplication()
{
    resetAllTimers();
    
    if (lockDialog) {
        lockDialog->close();
        delete lockDialog;
        lockDialog = nullptr;
    }
    
    if (floatWindow) {
        floatWindow->close();
        delete floatWindow;
        floatWindow = nullptr;
    }
    
    if (voicePlayer) {
        voicePlayer->stop();
    }
    
    QApplication::quit();
}

void MainWindow::updateCountdown()
{
    if ((!workTimer || !workTimer->isActive()) && (!restTimer || !restTimer->isActive())) {
        return;
    }
    
    int remainingSeconds = 0;
    if (isWorking && workTimer && workTimer->isActive()) {
        remainingSeconds = workTimer->remainingTime() / 1000;
        if (remainingSeconds < 0) remainingSeconds = 0;
    } else if (!isWorking && restTimer && restTimer->isActive()) {
        remainingSeconds = restTimer->remainingTime() / 1000;
        if (remainingSeconds < 0) remainingSeconds = 0;
    }
    
    int minutes = remainingSeconds / 60;
    int seconds = remainingSeconds % 60;
    QString timeStr = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    countdownLabel->setText(timeStr);
    
    if (floatWindow && isTimerStarted) {
        floatWindow->setTime(timeStr);
    }
}

void MainWindow::onWorkFinished()
{
    if (!isWorking) return;
    
    isWorking = false;
    
    int workMin = workMinutesEdit->text().toInt();
    int workSec = workSecondsEdit->text().toInt();
    int originalWorkSeconds = workMin * 60 + workSec;
    
    currentSeconds = restSeconds;
    
    if (workTimer && workTimer->isActive()) workTimer->stop();
    if (countdownTimer && countdownTimer->isActive()) countdownTimer->stop();
    if (remindTimer && remindTimer->isActive()) remindTimer->stop();
    
    if (remindWindow) {
        remindWindow->blockSignals(true);
        remindWindow->close();
        remindWindow->deleteLater();
        remindWindow = nullptr;
    }
    
    if (floatWindow) {
        floatWindow->hide();
    }
    
    showLockScreen();
    
    if (restTimer && restTimer->isActive()) restTimer->stop();
    if (restTimer) restTimer->start(restSeconds * 1000);
    if (countdownTimer) countdownTimer->start(1000);
    
    statusLabel->setText("休息中");
    statusLabel->setStyleSheet("color: #F44336; padding: 5px; font-weight: bold;");
    updateCountdown();
}

void MainWindow::onRestFinished()
{
    if (isWorking) return;
    
    int workMin = workMinutesEdit->text().toInt();
    int workSec = workSecondsEdit->text().toInt();
    workSeconds = workMin * 60 + workSec;
    
    int restMin = restMinutesEdit->text().toInt();
    int restSec = restSecondsEdit->text().toInt();
    restSeconds = restMin * 60 + restSec;
    
    currentSeconds = workSeconds;
    isWorking = true;
    
    if (restTimer && restTimer->isActive()) restTimer->stop();
    if (countdownTimer && countdownTimer->isActive()) countdownTimer->stop();
    
    if (lockDialog) {
        lockDialog->close();
        delete lockDialog;
        lockDialog = nullptr;
    }
    
    resetDelayCount();
    
    if (isTimerStarted) {
        showFloatWindow();
    }
    
    if (workTimer && workTimer->isActive()) workTimer->stop();
    if (workTimer) workTimer->start(workSeconds * 1000);
    if (countdownTimer) countdownTimer->start(1000);
    
    if (remindTimer && remindTimer->isActive()) remindTimer->stop();
    if (workSeconds > 60) {
        if (remindTimer) remindTimer->start((workSeconds - 60) * 1000);
    } else if (workSeconds > 0) {
        QTimer::singleShot(100, this, &MainWindow::showRemindWindow);
    }
    
    statusLabel->setText("工作间隔中");
    statusLabel->setStyleSheet("color: #4CAF50; padding: 5px; font-weight: bold;");
    updateCountdown();
}

void MainWindow::showLockScreen()
{
    if (floatWindow) {
        floatWindow->hide();
    }
    
    this->hide();
    
    int restMin = restMinutesEdit->text().toInt();
    int restSec = restSecondsEdit->text().toInt();
    int totalRestSeconds = restMin * 60 + restSec;
    
    if (lockDialog) {
        lockDialog->deleteLater();
        lockDialog = nullptr;
    }
    
    lockDialog = new LockScreenDialog(totalRestSeconds, nullptr);
    lockDialog->setAttribute(Qt::WA_DeleteOnClose);
    
    if (lockDialog) {
        lockDialog->showFullScreen();
        lockDialog->raise();
        lockDialog->activateWindow();
    }
}

void MainWindow::onSelectBgColor()
{
    QColor color = QColorDialog::getColor(floatBgColor, this, "选择背景颜色");
    if (color.isValid()) {
        floatBgColor = color;
        bgColorPreview->setStyleSheet(QString("border: 1px solid #cccccc; border-radius: 4px; background-color: %1;").arg(color.name()));
        saveColorSettings();
        applyFloatWindowColors();
    }
}

void MainWindow::onSelectFontColor()
{
    QColor color = QColorDialog::getColor(floatFontColor, this, "选择字体颜色");
    if (color.isValid()) {
        floatFontColor = color;
        fontColorPreview->setStyleSheet(QString("border: 1px solid #cccccc; border-radius: 4px; background-color: %1;").arg(color.name()));
        saveColorSettings();
        applyFloatWindowColors();
    }
}

void MainWindow::onRestoreDefaultColors()
{
    floatBgColor = QColor("#D3E3FD");
    floatFontColor = QColor("#EA3FF7");
    
    bgColorPreview->setStyleSheet("border: 1px solid #cccccc; border-radius: 4px; background-color: #D3E3FD;");
    fontColorPreview->setStyleSheet("border: 1px solid #cccccc; border-radius: 4px; background-color: #EA3FF7;");
    
    saveColorSettings();
    applyFloatWindowColors();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (isTimerStarted) {
        hideToTray();
        event->ignore();
    } else {
        QMessageBox::StandardButton reply = QMessageBox::question(this, "退出", "确定要退出程序吗？",
                                                                  QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            quitApplication();
            event->accept();
        } else {
            event->ignore();
        }
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        if (this->windowState() & Qt::WindowMinimized) {
            if (isTimerStarted) {
                hideToTray();
            } else {
                hideToTray();
            }
            event->ignore();
            return;
        }
    }
    QMainWindow::changeEvent(event);
}

#include "MainWindow.moc"


