#ifndef LOCKSCREENDIALOG_H
#define LOCKSCREENDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QShowEvent>
#include <QCloseEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QEvent>
#include <QPixmap>
#include <QList>

class LockScreenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LockScreenDialog(int restSeconds, QWidget *parent = nullptr);
    ~LockScreenDialog();

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateCountdown();
    void switchBackgroundImage();

private:
    void setupUI();
    void updateTimeDisplay();
    void loadBackgroundImages();
    void loadImageList();
    void drawTexts(QPainter &painter);  // 添加这个声明
    
    QTimer *timer;           // 倒计时计时器
    QTimer *slideshowTimer;  // 幻灯片计时器
    int restSeconds;
    QPixmap backgroundImage;
    QPixmap scaledBackground;
    QList<QPixmap> backgroundImages;  // 存储所有背景图片
    int currentImageIndex;            // 当前图片索引
    
    // 文字内容
    QString messageText;      // "请休息"
    QString timeText;         // 倒计时
    QString tipText;          // "🔒 系统已锁定 - 请放松眼睛和身体 🔒"
    
    // 文字颜色状态
    bool isFlashing;          // 最后3秒闪烁效果
};

#endif // LOCKSCREENDIALOG_H