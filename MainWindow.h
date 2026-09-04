#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QDebug>
#include <QCheckBox>
#include <QSettings>
#include <QMediaPlayer>
#include <QColorDialog>

class LockScreenDialog;
class FloatWindow;
class RemindWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void startStopTimer();
    void restartTimer();
    void updateCountdown();
    void onWorkFinished();
    void onRestFinished();
    void showNormalWindow();
    void quitApplication();
    void onAutoStartChanged(int state);
    void showFloatWindow();
    void updateFloatWindow();
    void showRemindWindow();
    void onRestNow();
    void onDelay5Minutes();
    void onDelay8Minutes();
    void onWorkMinutesChanged(const QString &text);
    void onWorkSecondsChanged(const QString &text);
    void onRestMinutesChanged(const QString &text);
    void onRestSecondsChanged(const QString &text);
    void onFloatWindowMoved();
    void onSelectBgColor();
    void onSelectFontColor();
    void onRestoreDefaultColors();

private:
    void setupUI();
    void createSystemTray();
    void showLockScreen();
    void resetAllTimers();
    void resetDelayCount();
    QIcon loadAppIcon();
    void startTimerAutomatically();
    void saveTimeSettings();
    void loadTimeSettings();
    void saveAutoStartSetting();
    void loadAutoStartSetting();
    QString getConfigFilePath();
    void saveFloatWindowPosition(const QPoint &pos);
    QPoint loadFloatWindowPosition();
    void hideToTray();
    void setupAutoStartShortcut();
    void playVoice(const QString &fileName);
    void loadColorSettings();
    void saveColorSettings();
    void applyFloatWindowColors();

    QTimer *workTimer;
    QTimer *restTimer;
    QTimer *countdownTimer;
    QTimer *floatTimer;
    QTimer *remindTimer;
    
    QLineEdit *workMinutesEdit;
    QLineEdit *workSecondsEdit;
    QLineEdit *restMinutesEdit;
    QLineEdit *restSecondsEdit;
    QLabel *statusLabel;
    QLabel *countdownLabel;
    QPushButton *startStopBtn;
    QPushButton *restartBtn;
    QCheckBox *autoStartCheckBox;
    
    // 颜色设置控件
    QPushButton *bgColorBtn;
    QPushButton *fontColorBtn;
    QPushButton *restoreDefaultBtn;
    QLabel *bgColorPreview;
    QLabel *fontColorPreview;
    
    int workSeconds;
    int restSeconds;
    int currentSeconds;
    bool isWorking;
    bool isTimerStarted;
    
    int delayCount;
    static const int MAX_DELAY_COUNT = 3;
    
    LockScreenDialog *lockDialog;
    
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    
    FloatWindow *floatWindow;
    QWidget *remindWindow;
    
    bool m_isUpdatingSettings;
    
    QMediaPlayer *voicePlayer;
    
    // 颜色变量
    QColor floatBgColor;
    QColor floatFontColor;
};

#endif // MAINWINDOW_H

