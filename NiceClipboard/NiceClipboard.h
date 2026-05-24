#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_NiceClipboard.h"
#include <QSystemTrayIcon>
#include "ClipboardListView.h"
#include <Logger.h>
#include <Windows.h>
#include <QTimer>
#include <QSettings>
#include "TranslucentScreenMask.h"
#include <QList>
#include "DoubleClickController.h"
#include "GlobalConfigManager.h"
#include <QCryptoGraphicHash>
#include <atomic>
#include <mutex>
#include <QQueue>
#include "ScheduledTask.h"

class NiceClipboard : public QMainWindow
{
    Q_OBJECT

private:
    QIcon appIcon{ ":/NiceClipboard/svgs/NiceClipboard.svg" };
    QIcon minimizeIcon{ ":/iconoir/svgs/iconoir/minus.svg" };
    QIcon maximizeIcon{ ":/iconoir/svgs/iconoir/enlarge.svg" };
    QIcon restoreIcon{ ":/iconoir/svgs/iconoir/reduce.svg" };
    QIcon closeIcon{ ":/iconoir/svgs/iconoir/xmark.svg" };
    //QColor titleBarBtnsBkgColor{ 250, 240, 230, 255 };

public:
    NiceClipboard(QWidget *parent = nullptr);
    ~NiceClipboard();

protected:
    virtual void showEvent(QShowEvent* event) override;
    virtual void hideEvent(QHideEvent* event) override;
    virtual void closeEvent(QCloseEvent* event) override;
    virtual bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result);
    virtual void changeEvent(QEvent* event) override;

private slots:
    void onClipboardDataChanged();
    void onClipboardHistoryItemLeftClicked(QListWidgetItem* item, const QPoint& pos);
    void onClipboardHistoryItemLeftDoubleClicked(QListWidgetItem* item, const QPoint& pos);
    void onClipboardHistoryItemRightClicked(QListWidgetItem* item, const QPoint& pos);
    void onClipboardHistoryItemRightDoubleClicked(QListWidgetItem* item, const QPoint& pos);

    void onBtnCloseClicked();
    void onBtnShowMinimizedClicked();
    void onBtnShowMaximizedClicked();

    void onBtnMenuClicked();
    void onBtnSortClicked();
    void onBtnSettingsClicked();
    void onBtnClearClicked();

    void openAboutDialog();
private:
    Ui::NiceClipboardClass ui;
    
    Logger logger{ "NiceClipboard" };

    GlobalConfigManager* globalConfig{ GlobalConfigManager::instance() };
    
    constexpr static const char* localServerName = "NiceClipboardLocalServer";

    ScheduledTask autoRunScheduledTask{ tr("Nice Clipboard").toLocal8Bit().toStdString(), tr("Nice Clipboard").toLocal8Bit().toStdString(), tr("Nice Clipboard开机自启动。").toLocal8Bit().toStdString() };

    // 数据持久化
    QString savedDataImagePath;
    QCryptographicHash::Algorithm savedDataImageHashAlgorithm = QCryptographicHash::Md5;
    QString savedDataImageHashFormat = "png";
    QString savedDataImageExtension = ".png";
    std::unique_ptr<QSettings> dataSettings{ nullptr };
    const std::string clipboardDataKey = "Clipboard data list";
    QHash<QString, int> clipboardDataImageLinkCounters; // 保存图片的引用计数，计数为0时删除文件

    // 列表项操作的时候不立即操作，而是添加到操作列表
    struct ClipboardHistoryOperation {
        enum Operation {
            Insert,
            Remove,
            Clear,
        };
        struct InsertData {
            int index = -1;
            ClipboardItemData* itemData{ nullptr };
        };
        struct RemoveData {
            int index = -1;
        };
        Operation op;
        union {
            InsertData insert;
            RemoveData remove;
        };
        ClipboardHistoryOperation(Operation op = (Operation)0) : op(op) {}
        ~ClipboardHistoryOperation() {}
        ClipboardHistoryOperation(const ClipboardHistoryOperation& other) : op(other.op) {
            copyPrivate(other);
        }
        ClipboardHistoryOperation& operator=(const ClipboardHistoryOperation& other) {
            if (this != &other)
                copy(other);
            return *this;
        }
        void copy(const ClipboardHistoryOperation& other) {
            op = other.op;
            copyPrivate(other);
        }
    private:
        void copyPrivate(const ClipboardHistoryOperation& other) {
            switch (op)
            {
            case Insert:
                insert = other.insert;
                break;
            case Remove:
                remove = other.remove;
                break;
            default:
                break;
            }
        }
    };
    // 添加内容到临时缓冲区，等待定时器写入磁盘
    std::mutex clipboardHistoryOperationQueueMutex;
    QQueue<ClipboardHistoryOperation> clipboardHistoryOperationQueue;
    QList<ClipboardItemData> clipboardHistory;
    std::atomic<bool> clipboardHistoryChanged{ false };
    std::mutex savingClipboardHistoryMutex;
    // 使用定时器定时保存剪贴板历史，以避免频繁读写磁盘
    QTimer* saveClipboardHistoryTimer{ nullptr };

    QSystemTrayIcon* systemTray{ nullptr };
    QClipboard* clipboard{ nullptr };
    HWND hwndThisWindow{ 0 };

    TranslucentScreenMask* screenBackgroundMask{ nullptr };

    QTimer* clipboardHistoryItemClickedTimer{ nullptr };

    std::unique_ptr<DoubleClickController> clipboardHistoryItemDoubleRightClickController{ nullptr };
    
    void registerGlobalHotKeys();
    void unregisterGlobalHotKeys();

    void printClipboardData(const QMimeData* mimeData);
    QString getClipboardDataSummary(const ClipboardItemData& item);

    void pasteClipboardToForegroundWindow();
    void addPasteAsActionsIntoMenu(QMenu* menu, const ClipboardItemData& itemData);
    void addPasteAsActionsIntoDialog(QDialog* dialog, const ClipboardItemData& itemData);

    void minimizeWindow();
    void maximizeWindow();
    void closeWindow();
    void hideWindow();

    static void winEventHookProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
    static LRESULT lowLevelMouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    //void pushClipboardHistoryQueue(ClipboardHistoryOperation::Operation operation, int index = 0, const ClipboardItemData& item = {});
    void pushInsertClipboardHistoryQueue(int index, const ClipboardItemData& item);
    void pushRemoveClipboardHistoryQueue(int index);
    void pushClearClipboardHistoryQueue();

    // index = -1 表示插入末尾
    void insertClipboardHistoryItem(qsizetype index, const ClipboardItemData& itemData);
    void searchClipboardHistoryItem();
    void removeClipboardHistoryItem(qsizetype index);
    void removeClipboardHistoryItem(QListWidgetItem* item);

    void readClipboardHistory();
    // index = -1 表示插入末尾
    void insertClipboardHistory(int index, const ClipboardItemData& item);
    void removeClipboardHistory(int index);
    void clearClipboardHistory();
    void updateClipboardHistory();
    void saveClipboardHistory();
    bool ensureDirectoryExists(const QString& path);
    void increaseClipboardDataImageLinkCounter(const QString& imageFileName);
    void decreaseClipboardDataImageLinkCounter(const QString& imageFileName);

    static int s_nextGlobalHotKeyId;
    static int getNextHotKeyId();
    bool registerHotKey(QKeyCombination hotKey);
    bool unregisterHotKey(QKeyCombination hotKey);

    void updateAndSaveClipboardHistoryAsync();
    void updateAndSaveClipboardHistorySync();

    bool registerAutoRunScheduledTask();
    bool unregisterAutoRunScheduledTask();
    bool checkAndRegisterAutoRunScheduledTask();
    bool checkAndUnregisterAutoRunScheduledTask();


    void updateMainWindowStyle();

    // 以管理员权限重启程序，并在程序重启后等待重启的程序发送的消息，以确定何时重启的程序已经启动完成
    // 然后向管理员程序发送一个消息，通知管理员程序继续完成剩下的操作（比如修改计划任务）
    void restartAsAdminAndWaitForResponse(const QList<QPair<QString, QVariant>>& pendingOperations = {});
    void replyToRestartedProcess();

    void handleConfigChangedOperation(const QString& key, const QVariant& value);
};

