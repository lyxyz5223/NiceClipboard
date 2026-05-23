#pragma once
#include <string>
#include <Windows.h>
#include <QMetaType>

struct Win32HotKey
{
    UINT virtualKeyCode{ 0 };
    UINT modifiers{ 0 };

    Win32HotKey(UINT vkCode, UINT mods) : virtualKeyCode(vkCode), modifiers(mods) {}

    enum Modifier {
        None = 0,
        Control = MOD_CONTROL,
        Alt = MOD_ALT,
        Shift = MOD_SHIFT,
        NoRepeat = MOD_NOREPEAT
    };

    std::string toString() const {
        std::string s;
        if (modifiers & Control)
            s += "Ctrl+";
        if (modifiers & Alt)
            s += "Alt+";
        if (modifiers & Shift)
            s += "Shift+";
        s += (char)virtualKeyCode;
        return s;
    }

    static UINT keyCodeFromQtKey(Qt::Key key) {
        if (key >= Qt::Key_A && key <= Qt::Key_Z) // A-Z
            return 'A' + (key - Qt::Key_A);
        else if (key >= Qt::Key_0 && key <= Qt::Key_9) // 0-9
            return '0' + (key - Qt::Key_0);
        else if (key >= Qt::Key_F1 && key <= Qt::Key_F24) // F1-F24
            return VK_F1 + (key - Qt::Key_F1);
        else
        {
            switch (key)
            {
            case Qt::Key_Space: // space
                return VK_SPACE;
            //case Qt::Key_Exclam: // !
            //case Qt::Key_QuoteDbl: // "
            //case Qt::Key_NumberSign: // #
            //case Qt::Key_Dollar: // $
            //case Qt::Key_Percent: // %
            //case Qt::Key_Ampersand: // &
            //case Qt::Key_Apostrophe: // '
            //case Qt::Key_ParenLeft: // (
            //case Qt::Key_ParenRight: // )
            //case Qt::Key_Asterisk: // *
            //case Qt::Key_Plus: // +
            //case Qt::Key_Comma: // ,
            //case Qt::Key_Minus: // -
            //case Qt::Key_Period: // .
            //case Qt::Key_Slash: // /
            //case Qt::Key_Colon: // :
            //case Qt::Key_Semicolon: // ;
            //case Qt::Key_Less: // <
            //case Qt::Key_Equal: // =
            //case Qt::Key_Greater: // >
            //case Qt::Key_Question: // ?
            //case Qt::Key_At: // @

            case Qt::Key_Escape: // Esc
                return VK_ESCAPE;
            case Qt::Key_Tab: // Tab
                return VK_TAB;
            //case Qt::Key_Backtab: // Shift+Tab 
            case Qt::Key_Backspace: // Backspace
                return VK_BACK;
            case Qt::Key_Return: // Main Keypad Enter
                return VK_RETURN;
            case Qt::Key_Enter: // Numeric Keypad Enter
                return VK_RETURN;
            case Qt::Key_Insert: // Insert
                return VK_INSERT;
            case Qt::Key_Delete: // Delete
                return VK_DELETE;
            case Qt::Key_Pause: // Pause
                return VK_PAUSE;
            case Qt::Key_Print: // Print Screen
                return VK_SNAPSHOT;
            //case Qt::Key_SysReq: // SysReq
            //case Qt::Key_Clear: // Clear
            case Qt::Key_Home: // Home
                return VK_HOME;
            case Qt::Key_End: // End
                return VK_END;
            case Qt::Key_Left: // Left Arrow
                return VK_LEFT;
            case Qt::Key_Up: // Up Arrow
                return VK_UP;
            case Qt::Key_Right: // Right Arrow
                return VK_RIGHT;
            case Qt::Key_Down: // Down Arrow
                return VK_DOWN;
            case Qt::Key_PageUp: // Page Up
                return VK_PRIOR;
            case Qt::Key_PageDown: // Page Down
                return VK_NEXT;
            //case Qt::Key_Shift: // Shift
            //case Qt::Key_Control: // Control
            //case Qt::Key_Meta: // Meta
            //case Qt::Key_Alt: // Alt
            case Qt::Key_CapsLock: // Caps Lock
                return VK_CAPITAL;
            case Qt::Key_NumLock: // Num Lock
                return VK_NUMLOCK;
            case Qt::Key_ScrollLock: // Scroll Lock
                return VK_SCROLL;
            default:
                return 0;
            }
        }
    }

    static Win32HotKey fromQKeyCombination(const QKeyCombination& comb) {
        UINT vkCode = keyCodeFromQtKey(comb.key());
        auto qtModifiers = comb.keyboardModifiers();
        UINT mods = 0;
        if (qtModifiers & Qt::CTRL)
            mods |= Control;
        if (qtModifiers & Qt::ALT)
            mods |= Alt;
        if (qtModifiers & Qt::SHIFT)
            mods |= Shift;
        return Win32HotKey(vkCode, mods);
    }

    static Win32HotKey fromLParam(LPARAM lParam) {
        Win32HotKey hk;
        hk.virtualKeyCode = (lParam >> 16) & 0xFFFF;
        hk.modifiers = lParam & 0xFFFF;
        return hk;
    }

    static Win32HotKey removeNorepeatModifier(const Win32HotKey& hk) {
        Win32HotKey newHk = hk;
        newHk.modifiers &= ~NoRepeat;
        return newHk;
    }
    static UINT removeNorepeatModifier(UINT modifiers) {
        modifiers &= ~NoRepeat;
        return modifiers;
    }
    void removeNorepeatModifier() {
        modifiers &= ~NoRepeat;
    }
    //Win32HotKey(const Win32HotKey&) = default;
    //Win32HotKey(Win32HotKey&&) = default;
    //Win32HotKey& operator=(const Win32HotKey&) = default;
    Win32HotKey() = default;
private:
};

Q_DECLARE_METATYPE(Win32HotKey)
