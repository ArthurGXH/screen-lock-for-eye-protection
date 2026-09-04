#ifndef INPUTBLOCKER_H
#define INPUTBLOCKER_H

#include <QObject>
#include <QAbstractNativeEventFilter>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class InputBlocker : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    static InputBlocker* instance();
    void blockInput(bool block);
    bool isBlocked() const { return m_blocked; }

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;

private:
    InputBlocker();
    ~InputBlocker();
    
    static InputBlocker* m_instance;
    bool m_blocked;
    
#ifdef Q_OS_WIN
    static HHOOK m_keyboardHook;
    static HHOOK m_mouseHook;
    
    static LRESULT CALLBACK keyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK mouseProc(int nCode, WPARAM wParam, LPARAM lParam);
#endif
};

#endif // INPUTBLOCKER_H