#include "InputBlocker.h"
#include <QDebug>
#include <QApplication>

InputBlocker* InputBlocker::m_instance = nullptr;

#ifdef Q_OS_WIN
HHOOK InputBlocker::m_keyboardHook = nullptr;
HHOOK InputBlocker::m_mouseHook = nullptr;
#endif

InputBlocker::InputBlocker()
    : m_blocked(false)
{
    qApp->installNativeEventFilter(this);
}

InputBlocker::~InputBlocker()
{
    qApp->removeNativeEventFilter(this);
#ifdef Q_OS_WIN
    if (m_keyboardHook) {
        UnhookWindowsHookEx(m_keyboardHook);
        m_keyboardHook = nullptr;
    }
    if (m_mouseHook) {
        UnhookWindowsHookEx(m_mouseHook);
        m_mouseHook = nullptr;
    }
#endif
}

InputBlocker* InputBlocker::instance()
{
    if (!m_instance) {
        m_instance = new InputBlocker();
    }
    return m_instance;
}

void InputBlocker::blockInput(bool block)
{
    m_blocked = block;
    
#ifdef Q_OS_WIN
    if (block) {
        // 安装键盘钩子
        if (!m_keyboardHook) {
            m_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardProc, 
                                              GetModuleHandle(nullptr), 0);
        }
        // 安装鼠标钩子
        if (!m_mouseHook) {
            m_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, mouseProc, 
                                          GetModuleHandle(nullptr), 0);
        }
    } else {
        // 卸载钩子
        if (m_keyboardHook) {
            UnhookWindowsHookEx(m_keyboardHook);
            m_keyboardHook = nullptr;
        }
        if (m_mouseHook) {
            UnhookWindowsHookEx(m_mouseHook);
            m_mouseHook = nullptr;
        }
    }
#endif
}

bool InputBlocker::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);
    
#ifdef Q_OS_WIN
    if (m_blocked) {
        MSG* msg = static_cast<MSG*>(message);
        
        // 阻止键盘和鼠标消息
        switch (msg->message) {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_MOUSEWHEEL:
            return true;
        }
    }
#endif
    
    return false;
}

#ifdef Q_OS_WIN
LRESULT CALLBACK InputBlocker::keyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && m_instance && m_instance->isBlocked()) {
        return 1;
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT CALLBACK InputBlocker::mouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && m_instance && m_instance->isBlocked()) {
        return 1;
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
#endif