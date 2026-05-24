#include "NiceClipboard.h"

#include <QGuiApplication>
#include <QMenu>
#include <QMimeData>
#include <QClipboard>
#include <QDebug>
#include <QMessageBox>
#include <sstream>
#include "StringProcess.h"
#include <iostream>
#include <iomanip>
#include <QWindow>
#include <QShortCut>
#include <mutex>
#include <windowsx.h> // GET_X_LPARAM, GET_Y_LPARAM
#include <QThread>
#include "SettingsWidget.h"
#include <QScrollBar>
#include <QScroller>
#include "CheckBoxWithUserData.h"
#include "Win32HotKey.h"
#include <QKeySequence>
#include "GlobalConfig.h"
#include <QDir>
#include <QFile>
#include "AdminUtils.h"
#include <QLocalSocket>
#include <QLocalServer>
#include "ClipboardListViewItem.h"

NiceClipboard* g_MainWindow{ nullptr };
HWND g_hwndLastForeground{ nullptr };
HWINEVENTHOOK g_hookWinEvent{ 0 };
std::mutex g_mtxHookWinEvent;
HHOOK g_hookMouseEvent{ 0 };
std::mutex g_mtxHookMouseEvent;

// 全局热键ID列表
QList<int> g_globalHotKeyIds;
QHash<QKeyCombination, int> g_globalHotKeys;
int NiceClipboard::s_nextGlobalHotKeyId{ 1 }; // 从1开始，0通常保留给无效ID

NiceClipboard::NiceClipboard(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    g_MainWindow = this;
    replyToRestartedProcess();
    logger.info("Initializing NiceClipboard...");
    //this->setAttribute(Qt::WA_ShowWithoutActivating);
    this->setAttribute(Qt::WA_AlwaysStackOnTop);
    this->setAttribute(Qt::WA_QuitOnClose);
    this->setWindowFlags(/*Qt::WindowDoesNotAcceptFocus | *///Qt::Tool | Qt::CustomizeWindowHint |
        Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint
        | Qt::WindowOverridesSystemGestures
    );
    this->setWindowModality(Qt::NonModal); // 非模态窗口
    //this->setFocusPolicy(Qt::NoFocus);
    hwndThisWindow = reinterpret_cast<HWND>(this->winId());
    logger.info("Setting window styles...");

    //SetWindowLong(hwndThisWindow, GWL_EXSTYLE, GetWindowLong(hwndThisWindow, GWL_EXSTYLE) | WS_EX_NOACTIVATE); // 取消激活窗口
    SetWindowLong(hwndThisWindow, GWL_EXSTYLE, GetWindowLong(hwndThisWindow, GWL_EXSTYLE) & ~(WS_EX_WINDOWEDGE));

    clipboard = QGuiApplication::clipboard();
    connect(clipboard, &QClipboard::dataChanged, this, &NiceClipboard::onClipboardDataChanged);
    connect(ui.listWidgetClipboard, &ClipboardListView::leftClicked, this, &NiceClipboard::onClipboardHistoryItemLeftClicked);
    //connect(ui.listWidgetClipboard, &ClipboardListView::itemClicked, this, &NiceClipboard::onClipboardHistoryItemClicked);
    //connect(ui.listWidgetClipboard, &ClipboardListView::itemDoubleClicked, this, &NiceClipboard::onClipboardHistoryItemDoubleClicked);
    //ui.listWidgetClipboard->setContextMenuPolicy(Qt::CustomContextMenu);
    //connect(ui.listWidgetClipboard, &ClipboardListView::customContextMenuRequested, this, &NiceClipboard::onClipboardHistoryItemRightClicked);
    connect(ui.listWidgetClipboard, &ClipboardListView::rightClicked, this, &NiceClipboard::onClipboardHistoryItemRightClicked);

    if (ui.listWidgetClipboard->verticalScrollMode() == QAbstractItemView::ScrollPerPixel)
        ui.listWidgetClipboard->verticalScrollBar()->setSingleStep(30);

    systemTray = new QSystemTrayIcon(this);
    // 创建托盘菜单
    AnimatedMenu* trayMenu = new AnimatedMenu(this);
    QAction* showAction = trayMenu->addAction(tr("显示主窗口"), [this] { this->show(); });
    QAction* hideAction = trayMenu->addAction(tr("隐藏主窗口"), [this] { hideWindow(); });
    trayMenu->addSeparator();
    QAction* settingsAction = trayMenu->addAction(tr("设置"), [this] {  });
    trayMenu->addSeparator();
    QAction* quitAction = trayMenu->addAction(tr("退出"), [this] { closeWindow(); });
    systemTray->setContextMenu(trayMenu);
    systemTray->setIcon(QIcon(":/NiceClipboard/svgs/NiceClipboard.svg"));
    systemTray->setToolTip(tr("Nice Clipboard"));
    systemTray->connect(systemTray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) // 单击托盘图标
            this->show();
        });

    ui.btnShowMinimized->setIcon(minimizeIcon);
    ui.btnShowMaximized->setIcon(maximizeIcon);
    ui.btnClose->setIcon(closeIcon);
    //ui.btnShowMinimized->setBackgroundBrush(QBrush(titleBarBtnsBkgColor));
    //ui.btnShowMaximized->setBackgroundBrush(QBrush(titleBarBtnsBkgColor));
    //ui.btnClose->setBackgroundBrush(QBrush(titleBarBtnsBkgColor));
    //ui.btnMenu->setBackgroundBrush(QBrush(titleBarBtnsBkgColor));

    connect(ui.lineEditSearch, &QLineEdit::editingFinished, this, &NiceClipboard::searchClipboardHistoryItem);
    connect(ui.btnSearch, &QPushButton::clicked, this, &NiceClipboard::searchClipboardHistoryItem);

    clipboardHistoryItemClickedTimer = new QTimer(this);
    clipboardHistoryItemDoubleRightClickController = std::make_unique<DoubleClickController>(this);

    screenBackgroundMask = new TranslucentScreenMask();
    screenBackgroundMask->connect(screenBackgroundMask, &TranslucentScreenMask::clicked, this, &NiceClipboard::hideWindow);

    logger.info("Loading config...");
    globalConfig->read();
    updateCurrentSettings();
    for (auto& page : g_settingPages)
        for (auto& setting : page.settings)
            if (setting.key.size())
                logger.trace("Setting: {} = {}", setting.key.toStdString(), setting.currentValue.toString().toStdString());

    switch (globalConfig->query(GlobalConfigManager::ClipboardHistoryListDragType).toInt())
    {
    case ClipboardHistoryListDragType::Touch:
        QScroller::grabGesture(ui.listWidgetClipboard->viewport(), QScroller::TouchGesture);
        break;
    case ClipboardHistoryListDragType::MouseLeftButton:
        QScroller::grabGesture(ui.listWidgetClipboard->viewport(), QScroller::LeftMouseButtonGesture);
        break;
    case ClipboardHistoryListDragType::MouseMiddleButton:
        QScroller::grabGesture(ui.listWidgetClipboard->viewport(), QScroller::MiddleMouseButtonGesture);
        break;
    case ClipboardHistoryListDragType::MouseRightButton:
        QScroller::grabGesture(ui.listWidgetClipboard->viewport(), QScroller::RightMouseButtonGesture);
        break;
    case ClipboardHistoryListDragType::None:
    default:
        break;
    }


    auto&& showInSystemTray = globalConfig->query(GlobalConfigManager::ShowInSystemTray).toBool();
    if (showInSystemTray)
        systemTray->show();
    else
        systemTray->hide();

    // 更新数据持久化文件读取位置
    auto&& dataFilePath = globalConfig->query(GlobalConfigManager::SavedDataFilePath).toString();
    dataSettings = std::make_unique<QSettings>(dataFilePath, QSettings::Format::IniFormat, this);
    savedDataImagePath = globalConfig->query(GlobalConfigManager::SavedDataImagePath).toString();
    ensureDirectoryExists(savedDataImagePath);


    registerGlobalHotKeys();

    logger.info("NiceClipboard initialized, reading clipboard history...");
    
    if (globalConfig->query(GlobalConfigManager::ClearHistoryOnLoad).toBool())
    {
        logger.info("Clearing clipboard history on load is enabled, skipping loading clipboard history");
        saveClipboardHistory();
    }
    else
        readClipboardHistory();
    saveClipboardHistoryTimer = new QTimer(this);
    connect(saveClipboardHistoryTimer, &QTimer::timeout, this, &NiceClipboard::updateAndSaveClipboardHistoryAsync);
    auto savedDataTimerInterval = globalConfig->query(GlobalConfigManager::SavedDataTimerInterval).toUInt();
    saveClipboardHistoryTimer->setInterval(savedDataTimerInterval); // 每n秒保存一次剪贴板历史
    saveClipboardHistoryTimer->start();

    // update main window style
    logger.info("Updating main window style...");
    updateMainWindowStyle();

    logger.info("Setting up global event hooks...");
    std::lock_guard lockMtxHookWinEvent(g_mtxHookWinEvent);
    if (!g_hookWinEvent)
    {
        g_hookWinEvent = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr, winEventHookProc, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
        if (g_hookWinEvent)
            logger.info("Global event hooks set up successfully");
        else
            logger.error("Failed to set up global event hooks");
    }
    //logger.info("Setting up global mouse hook...");
    //std::unique_lock lockMtxHookMouseEvent(g_mtxHookMouseEvent);
    //if (!g_hookMouseEvent)
    //{
    //    g_hookMouseEvent = SetWindowsHookEx(WH_MOUSE_LL, lowLevelMouseHookProc, nullptr, 0);
    //    if (g_hookMouseEvent)
    //        logger.info("Global mouse hook set up successfully");
    //    else
    //        logger.error("Failed to set up global mouse hook");
    //}
    logger.info("NiceClipboard started");
}

NiceClipboard::~NiceClipboard()
{
    std::lock_guard lock1(g_mtxHookWinEvent);
    if (g_hookWinEvent)
    {
        UnhookWinEvent(g_hookWinEvent);
        g_hookWinEvent = 0;
    }
    std::lock_guard lock2(g_mtxHookMouseEvent);
    if (g_hookMouseEvent)
    {
        UnhookWindowsHookEx(g_hookMouseEvent);
        g_hookMouseEvent = 0;
    }
    unregisterGlobalHotKeys();
    saveClipboardHistoryTimer->stop();

    if (globalConfig->query(GlobalConfigManager::ClearHistoryOnQuit).toBool())
    {
        logger.info("Clearing clipboard history on quit is enabled, skipping saving clipboard history");
        std::unique_lock lock(savingClipboardHistoryMutex);
        clearClipboardHistory();
        saveClipboardHistory();
    }
    else
    {
        // 退出时主动保存一次剪贴板历史，避免丢失
        updateAndSaveClipboardHistorySync();
    }
    globalConfig->destroy();
}

void NiceClipboard::winEventHookProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    if (event == EVENT_SYSTEM_FOREGROUND)
    {
        //qDebug() << "前台窗口变化，当前前台窗口: " << hwnd;
        if (hwnd == reinterpret_cast<HWND>(g_MainWindow->winId()))
            return;
        //g_hwndLastForeground = hwnd;
        //g_MainWindow->hideWindow();
    }
}

LRESULT NiceClipboard::lowLevelMouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0)
        return CallNextHookEx(g_hookMouseEvent, nCode, wParam, lParam);
    switch (wParam)
    {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONDBLCLK:
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCMBUTTONDOWN:
    case WM_NCMBUTTONDBLCLK:
    case WM_NCRBUTTONDOWN:
    case WM_NCRBUTTONDBLCLK:
    {
        auto run = [] {
            QPoint curPos = QCursor::pos();
            if (!QApplication::widgetAt(curPos))
                g_MainWindow->hideWindow();
            };
        //MSLLHOOKSTRUCT* pMouseStruct = (MSLLHOOKSTRUCT*)lParam;
        QMetaObject::invokeMethod(g_MainWindow, run, Qt::QueuedConnection);
    }
        break;
    default:
        break;
    }
    return CallNextHookEx(g_hookMouseEvent, nCode, wParam, lParam);;
}

void NiceClipboard::pushInsertClipboardHistoryQueue(int index, const ClipboardItemData& item)
{
    ClipboardHistoryOperation op{ ClipboardHistoryOperation::Insert };
    op.insert = { index, new ClipboardItemData(item) };
    std::unique_lock lock(clipboardHistoryOperationQueueMutex);
    clipboardHistoryOperationQueue.push_back(op);
}

void NiceClipboard::pushRemoveClipboardHistoryQueue(int index)
{
    ClipboardHistoryOperation op{ ClipboardHistoryOperation::Remove };
    op.remove.index = index;
    std::unique_lock lock(clipboardHistoryOperationQueueMutex);
    clipboardHistoryOperationQueue.push_back(op);
}

void NiceClipboard::insertClipboardHistoryItem(qsizetype index, const ClipboardItemData& itemData)
{
    ClipboardListItem* item = new ClipboardListItem();
    item->setClipboardItemData(itemData);
    auto showMenu = [this, itemData, item](QPoint menuPos) {
        QMenu* menu = new AnimatedMenu(this);
        menu->addAction(tr("复制"), [this, itemData]() { itemData.copyToClipboard(); });
        menu->addAction(tr("删除"), [this, item]() {
            auto idx = ui.listWidgetClipboard->row(item);
            ui.listWidgetClipboard->removeItemWidget(ui.listWidgetClipboard->item(idx));
            pushRemoveClipboardHistoryQueue(idx);
            delete item;
        });
        menu->popup(menuPos);
    };
    item->setShowMenuFunction(showMenu);
    if (index >= 0)
        ui.listWidgetClipboard->insertItem(index, item);
    else
        ui.listWidgetClipboard->addItem(item);
    ui.listWidgetClipboard->setItemWidget(item, item->itemWidget());
}

void NiceClipboard::searchClipboardHistoryItem()
{
    QString text = ui.lineEditSearch->text();
    QRegularExpression exp{ text, QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption };
    auto model = ui.listWidgetClipboard->model();
    for (int i = 0; i < model->rowCount(); ++i)
    {
        ClipboardItemData itemData = model->data(model->index(i, 0), Qt::DisplayRole).value<ClipboardItemData>();
        auto match = exp.match(itemData.text, 0, QRegularExpression::PartialPreferCompleteMatch, QRegularExpression::DontCheckSubjectStringMatchOption);
        auto matched = match.hasMatch();
        ui.listWidgetClipboard->setRowHidden(i, !matched);
    }
}



void NiceClipboard::readClipboardHistory()
{
    logger.info("Reading clipboard history from settings...");
    auto timeStart = std::chrono::high_resolution_clock::now();
    qsizetype arraySize = dataSettings->beginReadArray(clipboardDataKey);
    for (qsizetype i = 0; i < arraySize; ++i)
    {
        dataSettings->setArrayIndex(i);
        ClipboardItemData itemData{ false };
        itemData.dateTime = dataSettings->value("dateTime", QDateTime()).value<QDateTime>();
        itemData.formats = dataSettings->value("formats", QStringList()).value<QStringList>();
        itemData.text = dataSettings->value("text", QString()).value<QString>();
        itemData.html = dataSettings->value("html", QString()).value<QString>();
        QString imageFileName = dataSettings->value("imageFileName", QString()).value<QString>();
        QString imageFileExtension = dataSettings->value("imageFileSuffix", QString()).value<QString>();
        if (!imageFileName.isEmpty())
        {
            itemData.loadImageFile(QDir(savedDataImagePath).absoluteFilePath(imageFileName + imageFileExtension), savedDataImageHashAlgorithm, savedDataImageHashFormat);
            increaseClipboardDataImageLinkCounter(imageFileName);
        }
        QVariant urlsVariant = dataSettings->value("urls", QByteArray());
        QVariant rawDataVariant = dataSettings->value("rawData", QByteArray());
        QByteArray urlsData = urlsVariant.value<QByteArray>();
        QByteArray rawDataData = rawDataVariant.value<QByteArray>();
        QDataStream urlsStream(&urlsData, QIODevice::ReadOnly);
        QDataStream rawDataStream(&rawDataData, QIODevice::ReadOnly);
        while (!urlsStream.atEnd())
        {
            QUrl url;
            urlsStream >> url;
            itemData.urls.emplaceBack(url);
        }
        while (!rawDataStream.atEnd())
        {
            QString key;
            QByteArray value;
            rawDataStream >> key >> value;
            if (key == "application/x-qt-image")
                itemData.rawData[key] = QByteArrayView(itemData.image.constBits(), itemData.image.sizeInBytes()).toByteArray();
            else
                itemData.rawData[key] = value;
        }
        insertClipboardHistoryItem(-1, itemData);
        pushInsertClipboardHistoryQueue(-1, itemData);
    }
    dataSettings->endArray();
    auto timeEnd = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeStart).count();
    logger.info("Finished reading clipboard history, total {} items, time taken: {} ms", arraySize, duration);
}

void NiceClipboard::insertClipboardHistory(int index, const ClipboardItemData& item)
{
    if (index < 0 || index > clipboardHistory.size())
        index = clipboardHistory.size(); // 插入末尾
    clipboardHistory.insert(index, item);
}

void NiceClipboard::removeClipboardHistory(int index)
{
    clipboardHistory.remove(index, 1);
}

void NiceClipboard::clearClipboardHistory()
{
    clipboardHistory.clear();
}

void NiceClipboard::updateClipboardHistory()
{
    std::unique_lock lock(clipboardHistoryOperationQueueMutex);
    while (!clipboardHistoryOperationQueue.empty())
    {
        auto op = clipboardHistoryOperationQueue.front();
        clipboardHistoryOperationQueue.pop_front();
        lock.unlock();
        if (op.op == ClipboardHistoryOperation::Insert)
        {
            insertClipboardHistory(op.insert.index, *op.insert.itemData);
            delete op.insert.itemData;
            clipboardHistoryChanged.store(true);
        }
        else if (op.op == ClipboardHistoryOperation::Remove)
        {
            removeClipboardHistory(op.remove.index);
            clipboardHistoryChanged.store(true);
        }
        lock.lock();
    }
}

void NiceClipboard::saveClipboardHistory()
{
    logger.info("Writing clipboard history to settings...");
    auto timeStart = std::chrono::high_resolution_clock::now();
    dataSettings->beginWriteArray(clipboardDataKey);
    ensureDirectoryExists(savedDataImagePath);
    auto clipboardSize = clipboardHistory.size();
    for (qsizetype i = 0; i < clipboardSize; ++i)
    {
        auto& itemData = clipboardHistory[i];
        auto timeStart = std::chrono::high_resolution_clock::now();
        dataSettings->setArrayIndex(i);
        dataSettings->setValue("dateTime", itemData.getDateTime());
        dataSettings->setValue("formats", itemData.getFormats());
        dataSettings->setValue("text", itemData.getText());
        dataSettings->setValue("html", itemData.getHtml());
        QString imageFilePath;
        if (itemData.hasImage())
        {
            QByteArray imageHash = itemData.getCachedImageHash(savedDataImageHashAlgorithm, savedDataImageHashFormat); // 使用Sha-2的Sha256加密算法计算哈希
            if (imageHash.isEmpty())
                QMetaObject::invokeMethod(this, [this] { QMessageBox::critical(this, tr("错误"), tr("图片哈希为空，可能计算失败，请检查保存格式")); });
            QString imageFileName = imageHash.toHex();
            logger.trace("Image hash for clipboard item {}: {}", i, imageFileName.toStdString());
            dataSettings->setValue("imageFileName", imageFileName); // 保存图片的文件名到剪贴板数据文件中
            dataSettings->setValue("imageFileSuffix", savedDataImageExtension); // 保存图片的文件名到剪贴板数据文件中
            imageFilePath = QDir(savedDataImagePath).absoluteFilePath(imageFileName + savedDataImageExtension);
            if (!QFile::exists(imageFilePath)) // 如果图片不存在
                itemData.getImage().save(imageFilePath);
        }
        else // 没有图片则保存空字符串，否则会导致该项会保存上一项的图片文件名和后缀，导致读取时错误地认为有图片并尝试加载另一项的图片文件
        {
            dataSettings->setValue("imageFileName", QString());
            dataSettings->setValue("imageFileSuffix", QString());
        }
        QByteArray urlsData;
        QDataStream urlsStream(&urlsData, QIODevice::WriteOnly);
        for (const auto& url : itemData.getUrls())
            urlsStream << url;
        dataSettings->setValue("urls", urlsData);
        QByteArray rawDataData;
        QDataStream rawDataStream(&rawDataData, QIODevice::WriteOnly);
        for (auto it = itemData.getRawData().begin(); it != itemData.getRawData().end(); ++it)
        {
            if (it.key() == "application/x-qt-image")
                rawDataStream << it.key() << imageFilePath;
            else
                rawDataStream << it.key() << it.value();
        }
        dataSettings->setValue("rawData", rawDataData);
        auto timeEnd = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeStart).count();
        logger.trace("Finished writing clipboard history: {}, total {} items, time taken: {} ms", i + 1, clipboardSize, duration);
    }
    dataSettings->endArray();
    dataSettings->sync();
    auto timeEnd = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeStart).count();
    logger.info("Finished writing clipboard history, total {} items, time taken: {} ms", clipboardSize, duration);
}

bool NiceClipboard::ensureDirectoryExists(const QString& path)
{
    QDir dir(path);
    if (!dir.exists())
    {
        if (!dir.mkpath("."))
        {
            logger.warning("Cannot create directory: {}", path.toStdString());
            return false;
        }
        logger.info("Created directory: {}", path.toStdString());
    }
    return true;
}

void NiceClipboard::increaseClipboardDataImageLinkCounter(const QString& imageFileName)
{
    // 添加列表项后添加计数器哈希表计数
    if (!clipboardDataImageLinkCounters.contains(imageFileName))
        clipboardDataImageLinkCounters[imageFileName] = 1;
    else
        ++clipboardDataImageLinkCounters[imageFileName];
}

void NiceClipboard::decreaseClipboardDataImageLinkCounter(const QString& imageFileName)
{
    if (clipboardDataImageLinkCounters.contains(imageFileName))
        --clipboardDataImageLinkCounters[imageFileName];
}

int NiceClipboard::getNextHotKeyId()
{
    do
    {
        if (s_nextGlobalHotKeyId <= 0) // Windows全局热键ID上限
        {
            Logger("NiceClipboard").error("全局热键ID已达上限，无法注册更多热键");
            return -1; // 返回-1表示无法分配更多ID
        }
        if (!g_globalHotKeyIds.contains(s_nextGlobalHotKeyId))
        {
            g_globalHotKeyIds.append(s_nextGlobalHotKeyId);
            break;
        }
        else
            ++s_nextGlobalHotKeyId;
    } while (true);
    return s_nextGlobalHotKeyId++;
}

bool NiceClipboard::registerHotKey(QKeyCombination hotKey)
{
    auto win32HotKeyShowClipboard = Win32HotKey::fromQKeyCombination(hotKey);
    int id = -1;
    bool contained = false;
    if (g_globalHotKeys.contains(hotKey))
    {
        id = g_globalHotKeys[hotKey];
        contained = true;
    }
    else
        id = getNextHotKeyId();
    if (id <= 0)
    {
        QMessageBox::critical(this, tr("错误"), tr("无法注册全局热键，已达全局热键ID上限"));
        return false;
    }
    if (!RegisterHotKey(reinterpret_cast<HWND>(this->winId()), id, win32HotKeyShowClipboard.modifiers, win32HotKeyShowClipboard.virtualKeyCode))
    {
        QMessageBox::critical(this, tr("错误"), tr("注册全局热键失败，请检查全局快捷键是否冲突"));
        return false;
    }
    if (!contained)
        g_globalHotKeys[hotKey] = id;
    return true;
}

bool NiceClipboard::unregisterHotKey(QKeyCombination hotKey)
{
    if (!g_globalHotKeys.contains(hotKey))
        return false;
    int id = g_globalHotKeys[hotKey];
    BOOL result = UnregisterHotKey(reinterpret_cast<HWND>(this->winId()), id);
    if (!result)
        return false;
    g_globalHotKeys.remove(hotKey);
    g_globalHotKeyIds.removeOne(id);
    return true;
}

void NiceClipboard::updateAndSaveClipboardHistoryAsync()
{
    //QThread* updateThread = QThread::create([this] {
    //    std::unique_lock lock(clipboardHistoryWritingMutex);
    //    updateClipboardHistory();
    //});
    //updateThread->start();
    //if (clipboardHistoryChanged.load())
    //{
    //    clipboardHistoryChanged.store(false);
    //    QThread* writeThread = QThread::create([this] {
    //        std::unique_lock lock(clipboardHistoryWritingMutex);
    //        writeClipboardHistory();
    //    });
    //    writeThread->start();
    //}
    QThread* updateThread = QThread::create([this] {
        updateAndSaveClipboardHistorySync();
    });
    updateThread->start();
}

void NiceClipboard::updateAndSaveClipboardHistorySync()
{
    std::unique_lock lock(savingClipboardHistoryMutex);
    updateClipboardHistory();
    if (clipboardHistoryChanged.load())
    {
        clipboardHistoryChanged.store(false);
        saveClipboardHistory();
    }
}

bool NiceClipboard::checkAndRegisterAutoRunScheduledTask()
{
    ScheduledTask::TaskResult result = autoRunScheduledTask.getRegisteredTask("\\lyxyz5223\\NiceClipboard");
    if (result != ScheduledTask::TRSuccess && result != ScheduledTask::TRAlreadyExists)
    {
        if (!isRunningAsAdmin())
        {
            logger.warning("Current process is not running with administrator privileges, cannot register autorun scheduled task");
            return false;
        }
        return registerAutoRunScheduledTask();
    }
    return true;
}

bool NiceClipboard::checkAndUnregisterAutoRunScheduledTask()
{
    ScheduledTask::TaskResult result = autoRunScheduledTask.getRegisteredTask("\\lyxyz5223\\NiceClipboard");
    if (result == ScheduledTask::TRSuccess || result == ScheduledTask::TRAlreadyExists)
    {
        if (!isRunningAsAdmin())
        {
            logger.warning("Current process is not running with administrator privileges, cannot unregister autorun scheduled task");
            return false;
        }
        return unregisterAutoRunScheduledTask();
    }
    return true;
}

void NiceClipboard::updateMainWindowStyle()
{
    auto mainWindowStyle = globalConfig->query(GlobalConfigManager::QSSMainWindowStyle).toString();
    this->setStyleSheet(mainWindowStyle);
}

void NiceClipboard::restartAsAdminAndWaitForResponse(const QList<QPair<QString, QVariant>>& pendingOperations)
{
    QLocalServer* localServer = new QLocalServer(this);
    if (!localServer->listen(localServerName))
    {
        logger.error("Failed to create local server for admin process communication: {}", localServer->errorString().toStdString());
        QMessageBox::critical(this, tr("错误"), tr("无法创建本地服务器与管理员进程通信，请检查是否有同名的其他程序正在运行"));
        return;
    }
    connect(localServer, &QLocalServer::newConnection, [localServer, pendingOperations, this]() {
        QLocalSocket* socket = localServer->nextPendingConnection();
        connect(socket, &QLocalSocket::readyRead, [socket, localServer, pendingOperations, this]() {
            // 处理数据
            QByteArray data = socket->readAll();
            QString message(data);
            if (message == "RelaunchAsAdmin")
            {
                // 收到管理员进程的启动消息
                socket->write("OK");
                socket->flush();
            }
            else if (message == "WaitingForPendingOperations")
            {
                // 管理员进程正在等待主进程发送待执行的操作
                QByteArray msgToSend;
                QDataStream stream(&msgToSend, QIODevice::WriteOnly);
                stream << QString("PendingOperations");
                for (const auto& op : pendingOperations)
                {
                    QString key = op.first;
                    QVariant value = op.second;
                    stream << key << value;
                }
                socket->write(msgToSend);
                socket->flush();
            }
            else if (message == "Finished")
            {
                // 管理员进程执行完毕的确认消息
                socket->write("Finished"); // 发送消息通知管理员进程可以继续执行了
                socket->flush();
                socket->disconnectFromServer();
                localServer->deleteLater();
                show();
                close();
            }
            else
            {
                logger.warning("Received unexpected message from admin process: {}", message.toStdString());
                QMessageBox::warning(nullptr, tr("警告"), tr("收到管理员进程的意外消息，可能无法正常运行"));
                localServer->deleteLater();
                show();
                close();
            }
        });
    });

    unregisterGlobalHotKeys();
    bool relaunchSucceed = relaunchAsAdmin();
    if (!relaunchSucceed)
    {
        logger.error("Failed to relaunch as admin");
        QMessageBox::critical(this, tr("错误"), tr("以管理员权限重新启动失败"));
        localServer->deleteLater();
        registerGlobalHotKeys();
    }
}

void NiceClipboard::replyToRestartedProcess()
{
    QLocalSocket socket;
    socket.connectToServer(localServerName);
    if (socket.waitForConnected(3000))
    {
        // 连接到之前的实例，发送消息
        socket.write("RelaunchAsAdmin");
        socket.flush();
        while (true) // 等待管理员进程的响应，最长等待5秒
        {
            if (socket.waitForReadyRead(5000))
            {
                QByteArray data = socket.readAll();
                QString message(data);
                if (message == "OK")
                {
                    // 收到管理员进程的确认消息
                    logger.info("Received confirmation from admin process, waiting for pending operations");
                    // 发送等待执行操作的消息给管理员进程
                    socket.write("WaitingForPendingOperations");
                    socket.flush();
                }
                else if (message == "Finished")
                {
                    // 收到管理员进程的继续执行消息
                    logger.info("Received continue message from admin process, continuing execution");
                    break;
                }
                else
                {
                    QDataStream stream(data);
                    QString head;
                    stream >> head;
                    if (head == "PendingOperations")
                    {
                        // 收到未完成的操作，反序列化
                        while (!stream.atEnd())
                        {
                            QString key;
                            QVariant value;
                            stream >> key >> value;
                            // 处理反序列化的操作
                            handleConfigChangedOperation(key, value);
                        }
                        socket.write("Finished"); // 发送消息通知管理员进程可以继续执行了
                        socket.flush();
                    }
                    else
                    {
                        logger.warning("Received unexpected message from admin process: {}", message.toStdString());
                        QMessageBox::warning(this, tr("警告"), tr("收到管理员进程的意外消息，可能无法正常运行"));
                        break;
                    }
                }
            }
            else
            {
                logger.error("No response from admin process after relaunching as admin");
                QMessageBox::critical(this, tr("错误"), tr("没有收到管理员进程的响应，可能无法正常运行"));
                break;
            }
        }
    }
    // 如果连接失败，说明没有之前的实例在监听，正常继续执行
    // 如果处理完成，则继续执行
}

void NiceClipboard::handleConfigChangedOperation(const QString& key, const QVariant& value)
{
    if (key == GlobalConfigManager::configKeyToString(GlobalConfigManager::StartWithWindows))
    {
        bool startWithWindows = value.toBool();
        if (startWithWindows)
            checkAndRegisterAutoRunScheduledTask();
        else
            checkAndUnregisterAutoRunScheduledTask();
    }
}


bool NiceClipboard::registerAutoRunScheduledTask()
{
    ScheduledTask::TaskResult result = autoRunScheduledTask.createTaskDefinition();
    if (result != ScheduledTask::TRSuccess && result != ScheduledTask::TRAlreadyExists)
    {
        logger.error("Failed to register autorun task: createTaskDefinition failed");
        return false;
    }
    ScheduledTask::Trigger trigger;
    trigger.type = ScheduledTask::TriggerType::Logon;
    trigger.enabled = true;
    trigger.id = "NiceClipboard autorun";
    ScheduledTask::LogonTriggerData triggerData;
    trigger.data = triggerData;
    result = autoRunScheduledTask.setTrigger(trigger);
    if (result != ScheduledTask::TRSuccess && result != ScheduledTask::TRAlreadyExists)
    {
        logger.error("Failed to register autorun task: addTrigger failed");
        return false;
    }
    ScheduledTask::Action action;
    action.type = ScheduledTask::ActionType::Execute;
    ScheduledTask::ExecuteActionData actionData;
    actionData.executable = QGuiApplication::applicationFilePath().toStdString();
    actionData.workingDirectory = QGuiApplication::applicationDirPath().toStdString();
    actionData.arguments = "--autorun";
    action.data = actionData;
    result = autoRunScheduledTask.setAction(action);
    if (result != ScheduledTask::TRSuccess && result != ScheduledTask::TRAlreadyExists)
    {
        logger.error("Failed to register autorun task: addAction failed");
        return false;
    }
    result = autoRunScheduledTask.registerTask("\\lyxyz5223\\NiceClipboard");
    if (result != ScheduledTask::TRSuccess && result != ScheduledTask::TRAlreadyExists)
    {
        logger.error("Failed to register autorun task: registerTask failed");
        return false;
    }
    logger.info("Autorun scheduled task registered successfully");
    return true;
}

bool NiceClipboard::unregisterAutoRunScheduledTask()
{
    auto result = autoRunScheduledTask.deleteTask("\\lyxyz5223\\NiceClipboard");
    if (result != ScheduledTask::TRSuccess && result != ScheduledTask::TRNotExists)
    {
        logger.error("Failed to unregister autorun task: unregisterTask failed");
        return false;
    }
    logger.info("Autorun scheduled task unregistered successfully");
    return true;
}

void NiceClipboard::showEvent(QShowEvent* event)
{
    g_hwndLastForeground = GetForegroundWindow();
    screenBackgroundMask->show();
    screenBackgroundMask->lower(); // 确保遮罩在窗口下方
    LONG winLong = GetWindowLong(hwndThisWindow, GWL_EXSTYLE);
    if (!(winLong & WS_EX_TOPMOST))
    {
        // 确保窗口置顶
        //SetWindowLong(hwndThisWindow, GWL_EXSTYLE, winLong | WS_EX_TOPMOST);
        SetWindowPos(hwndThisWindow, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    this->raise(); // 确保窗口在最前
    QMainWindow::showEvent(event);
}

void NiceClipboard::hideEvent(QHideEvent* event)
{
    screenBackgroundMask->hide();
}

void NiceClipboard::closeEvent(QCloseEvent* event)
{
    QMainWindow::closeEvent(event);
}


bool NiceClipboard::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
    if (eventType == "windows_generic_MSG")
    {
        MSG* msg = static_cast<MSG*>(message);
        switch (msg->message)
        {
        case WM_NCCALCSIZE:
            // 移除所有非客户区
            if (msg->wParam)
            {
                const int defaultCaptionHeight = GetSystemMetrics(SM_CYCAPTION);
                const int defaultXBorderWidth = GetSystemMetrics(SM_CXSIZEFRAME);
                const int defaultYBorderWidth = GetSystemMetrics(SM_CYSIZEFRAME);
                int captionHeight = defaultCaptionHeight;
                int xBorderWidth = defaultXBorderWidth;
                int yBorderWidth = defaultYBorderWidth;

                // 获取建议的新矩形
                NCCALCSIZE_PARAMS* pParams = (NCCALCSIZE_PARAMS*)msg->lParam;
                RECT suggestedMovedRect = pParams->rgrc[0];
                RECT origRect = pParams->rgrc[1];
                RECT origClientRect = pParams->rgrc[2];

                RECT& rstMovedClientRect = pParams->rgrc[0];
                RECT& rstMovedRect = pParams->rgrc[1];
                RECT& rstOriginalRect = pParams->rgrc[2];
                auto rst = DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
                rstMovedClientRect.top = suggestedMovedRect.top;
                //qDebug() << "pParams->rgrc[0]: lt rb" << pParams->rgrc[0].left - suggestedMovedRect.left << pParams->rgrc[0].top - suggestedMovedRect.top << pParams->rgrc[0].right - suggestedMovedRect.right << pParams->rgrc[0].bottom - suggestedMovedRect.bottom;
                //qDebug() << "pParams->rgrc[1]: lt rb" << pParams->rgrc[1].left - origRect.left << pParams->rgrc[1].top - origRect.top << pParams->rgrc[1].right - origRect.right << pParams->rgrc[1].bottom - origRect.bottom;
                //qDebug() << "pParams->rgrc[2]: lt rb" << pParams->rgrc[2].left - origClientRect.left << pParams->rgrc[2].top - origClientRect.top << pParams->rgrc[2].right - origClientRect.right << pParams->rgrc[2].bottom - origClientRect.bottom;
                *result = rst;
                return true;
            }
            break;
        case WM_NCHITTEST:
        {
            // 允许窗口通过客户区拖动
            int xPos = GET_X_LPARAM(msg->lParam);
            int yPos = GET_Y_LPARAM(msg->lParam);
            int frameSize = GetSystemMetrics(SM_CYSIZEFRAME);
            QPoint curPos = QCursor::pos();
            QPoint curWindowPos = this->mapFromGlobal(curPos);
            RECT windowRect;
            GetWindowRect(msg->hwnd, &windowRect);
            if (yPos - windowRect.top <= frameSize)
                *result = HTTOP;
            else if (ui.labelTitle->geometry().bottom() >= curWindowPos.y()
                && !ui.btnMenu->geometry().contains(curWindowPos)
                && !ui.btnShowMinimized->geometry().contains(curWindowPos)
                && !ui.btnShowMaximized->geometry().contains(curWindowPos)
                && !ui.btnClose->geometry().contains(curWindowPos))
                *result = HTCAPTION; // 允许窗口顶部的区域通过客户区拖动
            else
                *result = DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
            return true;
        }
        case WM_HOTKEY:
        {
            Win32HotKey hk = Win32HotKey::fromLParam(msg->lParam);
            logger.trace() << "Hot key triggered: " << hk.toString();
            auto hotKeyShowClipboard = globalConfig->query(GlobalConfigManager::ShowWindowShortcut).value<QKeySequence>();
            auto win32HotKeyShowClipboard = Win32HotKey::fromQKeyCombination(hotKeyShowClipboard[0]);
            if (hk.modifiers == Win32HotKey::removeNorepeatModifier(win32HotKeyShowClipboard.modifiers) && hk.virtualKeyCode == win32HotKeyShowClipboard.virtualKeyCode)
                this->show();
            return true;
        }
        default:
            break;
        }
    }
    return false;
}



void NiceClipboard::registerGlobalHotKeys()
{
    auto hotKeyShowClipboard = globalConfig->query(GlobalConfigManager::ShowWindowShortcut).value<QKeySequence>();
    logger.info("Registering global hotkey {}...", hotKeyShowClipboard.toString().toStdString());
    registerHotKey(hotKeyShowClipboard[0]);
}

void NiceClipboard::unregisterGlobalHotKeys()
{
    auto hotKeyShowClipboard = globalConfig->query(GlobalConfigManager::ShowWindowShortcut).value<QKeySequence>();
    logger.info("Unregistering global hotkey {}...", hotKeyShowClipboard.toString().toStdString());
    unregisterHotKey(hotKeyShowClipboard[0]);
}

void NiceClipboard::printClipboardData(const QMimeData * mimeData)
{
    ClipboardItemData item{ false };
    item.setMimeData(mimeData, savedDataImageHashAlgorithm, savedDataImageHashFormat);
    logger.info() << getClipboardDataSummary(item).toStdString();
}

QString NiceClipboard::getClipboardDataSummary(const ClipboardItemData& item)
{
    const QStringList& formats = item.getFormats();
    QString str = tr("所有可用格式：\n");
    QString tabText = "    ";
    for (const auto& format : formats)
        str += tabText + format + "\n";
    QTextStream strStream(&str);
    for (const QString& format : formats)
    {
        strStream << tr("格式：") << format << "\n    ";
        if (format == "text/plain")
        {
            QString text = item.getText();
            strStream << tr("文本内容：") << text << "\n";
        }
        else if (format == "text/html")
        {
            QString html = item.getHtml();
            strStream << tr("HTML内容：") << html << "\n";
        }
        else if (format == "text/uri-list")
        {
            QList<QUrl> urls = item.getUrls();
            strStream << tr("URI列表：") << "\n";
            for (const QUrl& url : urls)
            {
                strStream << tabText << tabText << url.toString() << "\n";
            }
        }
        else if (format == "application/x-qt-image")
        {
            QImage image = qvariant_cast<QImage>(item.getImage());
            uchar* imageBits = reinterpret_cast<uchar*>(image.bits());
            size_t byteSize = image.sizeInBytes();
            if (byteSize > 50)
                byteSize = 50;
            strStream << tr("图像内容: ") << "\n";
            for (size_t i = 0; i < byteSize; ++i)
                strStream << "\\x" << QString::number(imageBits[i], 16);
            if (image.sizeInBytes() > 50)
                strStream << "...";
            strStream << "\n";
        }
        else
        {
            QByteArray data = item.getRawData().value(format);
            strStream << tr("原始数据: ") << data << "\n";
        }
    }
    return str + strStream.readAll();
}

void NiceClipboard::pasteClipboardToForegroundWindow()
{
    HWND hwndTarget = g_hwndLastForeground;
    //char title[256];
    //GetWindowTextA(hwndTarget, title, 256);
    //qDebug() << "当前前台窗口标题: " << QString::fromLocal8Bit(title);

    // 优先尝试WM_PASTE（目标窗口直接从剪贴板读取）
    //DWORD_PTR result = 0;
    //if (PostMessage(m_targetHwnd, WM_PASTE, 0, 0))
    //    return;

    // 否则把目标窗口设为前台并模拟Ctrl+V
    DWORD curTid = GetCurrentThreadId();
    DWORD targetTid = GetWindowThreadProcessId(hwndTarget, nullptr);

    AttachThreadInput(curTid, targetTid, TRUE);
    SetForegroundWindow(hwndTarget);
    AttachThreadInput(curTid, targetTid, FALSE);

    INPUT inputs[4] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    inputs[0].ki.dwFlags = 0;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'V';
    inputs[1].ki.dwFlags = 0;
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'V';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(4, inputs, sizeof(INPUT));
}

void NiceClipboard::addPasteAsActionsIntoMenu(QMenu* menu, const ClipboardItemData& itemData)
{
    auto sectionAction = new QAction(tr("提取指定内容到剪贴板并粘贴"), menu);
    sectionAction->setEnabled(false);
    menu->addAction(sectionAction);
    sectionAction = new QAction(tr("（请选择一种数据类型）"), menu);
    sectionAction->setEnabled(false);
    menu->addAction(sectionAction);
    menu->addSeparator();
    for (const auto& fmt : itemData.getFormats())
    {
        menu->addAction(fmt, [this, &fmt, &itemData] {
            logger.info("Paste as: {}", fmt.toLocal8Bit().constData());
            itemData.copyToClipboard(QStringList() << fmt, this);
            pasteClipboardToForegroundWindow();
            hideWindow();
        });
    }
}

void NiceClipboard::addPasteAsActionsIntoDialog(QDialog* dialog, const ClipboardItemData& itemData)
{
    auto* layout = dialog->layout();
    QLabel* title = new QLabel(tr("提取指定内容到剪贴板并粘贴"), dialog);
    layout->addWidget(title);
    title = new QLabel(tr("（请选择一种或多种数据类型）"), dialog);
    layout->addWidget(title);
    for (const auto& fmt : itemData.getFormats())
    {
        CheckBoxWithUserData* checkBox = new CheckBoxWithUserData(fmt, dialog);
        checkBox->setUserData(fmt);
        layout->addWidget(checkBox);
    }
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* btnOk = new QPushButton(tr("确定"), dialog);
    btnLayout->addWidget(btnOk);
    QPushButton* btnCancel = new QPushButton(tr("取消"), dialog);
    btnLayout->addWidget(btnCancel);
    layout->addItem(btnLayout);
    connect(btnOk, &QPushButton::clicked, this, [this, dialog, itemData] {
        QStringList fmtList;
        for (auto* child : dialog->children()) // 遍历所有子控件
            if (auto* checkBox = qobject_cast<CheckBoxWithUserData*>(child)) // 如果是CheckBoxWithUserData
                if (checkBox->isChecked()) // 如果被选中
                    fmtList << checkBox->getUserData().toString(); // 将用户数据添加到列表
        itemData.copyToClipboard(fmtList, this);
        if (!fmtList.isEmpty())
            pasteClipboardToForegroundWindow();
        dialog->close();
    });
    connect(btnCancel, &QPushButton::clicked, this, [this, dialog, itemData] {
        dialog->close();
    });
}

void NiceClipboard::minimizeWindow()
{
    this->showMinimized();
    //SendMessage(hwndThisWindow, WM_SYSCOMMAND, SC_MINIMIZE, 0);
}

void NiceClipboard::maximizeWindow()
{
    if (isMaximized())
        showNormal();
    else
        showMaximized();
    //if (isMaximized())
    //    SendMessage(hwndThisWindow, WM_SYSCOMMAND, SC_RESTORE, 0);
    //else
    //    SendMessage(hwndThisWindow, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
}

void NiceClipboard::closeWindow()
{
    this->show();
    this->close();
    //SendMessage(hwndThisWindow, WM_SYSCOMMAND, SC_CLOSE, 0);
}

void NiceClipboard::hideWindow()
{
    //QScroller::scroller(ui.listWidgetClipboard->viewport())->stop();
    hide();
}


void NiceClipboard::onClipboardDataChanged()
{
    const QMimeData* mimeData = clipboard->mimeData();
    ClipboardItemData itemData{ true };
    itemData.setMimeData(mimeData, savedDataImageHashAlgorithm, savedDataImageHashFormat);
    if (!itemData.shouldHandleClipboardEvent())
        return;
    insertClipboardHistoryItem(0, itemData);
    pushInsertClipboardHistoryQueue(0, itemData);
    //printClipboardData(mimeData);
}

void NiceClipboard::onClipboardHistoryItemLeftClicked(QListWidgetItem* item, const QPoint& pos)
{
    if (clipboardHistoryItemClickedTimer->isActive())
    {
        clipboardHistoryItemClickedTimer->stop();
        onClipboardHistoryItemLeftDoubleClicked(item, pos);
        return;
    }
    clipboardHistoryItemClickedTimer->disconnect(clipboardHistoryItemClickedTimer, SIGNAL(timeout()), nullptr, nullptr);
    clipboardHistoryItemClickedTimer->connect(clipboardHistoryItemClickedTimer, &QTimer::timeout, this, [this, itemData = item->data(Qt::DisplayRole).value<ClipboardItemData>()] {
        logger.info("List item clicked: {}", itemData.getText().toLocal8Bit().constData());
        itemData.copyToClipboard(this);
        pasteClipboardToForegroundWindow();
        hideWindow();
        });
    clipboardHistoryItemClickedTimer->setSingleShot(true);
    auto interval = globalConfig->query(GlobalConfigManager::MouseDoubleLeftClickInterval).toUInt();
    clipboardHistoryItemClickedTimer->setInterval(interval);
    clipboardHistoryItemClickedTimer->start();
}

void NiceClipboard::onClipboardHistoryItemLeftDoubleClicked(QListWidgetItem* item, const QPoint& pos)
{
    if (clipboardHistoryItemClickedTimer->isActive())
        clipboardHistoryItemClickedTimer->stop();
    auto itemData = item->data(Qt::DisplayRole).value<ClipboardItemData>();
    logger.info("List item double clicked: {}", itemData.getText().toLocal8Bit().constData());
    QString itemDesc = getClipboardDataSummary(itemData);
    QMessageBox::information(this, tr("列表项内容"), itemDesc);
}

// 剪贴板历史记录列表项的右键点击处理函数
void NiceClipboard::onClipboardHistoryItemRightClicked(QListWidgetItem* item, const QPoint& pos)
{
    if (clipboardHistoryItemDoubleRightClickController->detectDoubleClick([this, item, pos]() {
        onClipboardHistoryItemRightDoubleClicked(item, pos);
    })) return;
    ClipboardItemData itemData = item->data(Qt::DisplayRole).value<ClipboardItemData>();
    clipboardHistoryItemDoubleRightClickController->connect([this, itemData, pos]() {
        AnimatedMenu* menu = new AnimatedMenu{ this };
        menu->setAttribute(Qt::WA_DeleteOnClose);
        menu->setTitle(tr("粘贴为："));
        addPasteAsActionsIntoMenu(menu, itemData);
        menu->popup(ui.listWidgetClipboard->mapToGlobal(pos));
    });
    auto interval = globalConfig->query(GlobalConfigManager::MouseDoubleRightClickInterval).toUInt();
    clipboardHistoryItemDoubleRightClickController->setInterval(interval);
    clipboardHistoryItemDoubleRightClickController->start();
}

void NiceClipboard::onClipboardHistoryItemRightDoubleClicked(QListWidgetItem* item, const QPoint& pos)
{
    ClipboardItemData itemData = item->data(Qt::DisplayRole).value<ClipboardItemData>();
    QDialog* dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    // 为对话框添加格式多选功能
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    //dialog.setLayout(layout);
    addPasteAsActionsIntoDialog(dialog, itemData);
    dialog->exec();
}

void NiceClipboard::onBtnCloseClicked()
{
    closeWindow();
}

void NiceClipboard::onBtnShowMinimizedClicked()
{
    hideWindow();
    //minimizeWindow();
}

void NiceClipboard::onBtnShowMaximizedClicked()
{
    maximizeWindow();
}

void NiceClipboard::onBtnMenuClicked()
{
    AnimatedMenu* menu = new AnimatedMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->addAction(tr("设置"), [this] { onBtnSettingsClicked(); });
    menu->addSeparator();
    menu->addAction(tr("关于"), [this] {
        QMessageBox::about(this, tr("关于 Nice Clipboard"), tr("Nice Clipboard 是一个剪贴板增强工具，支持文本、图片等多种格式的剪贴板历史记录和管理。\n\n项目地址: https://github.com/lyxyz5223/NiceClipboard"));
    });
    menu->popup(ui.btnMenu->mapToGlobal(QPoint(ui.btnMenu->width() / 2, ui.btnMenu->height())));
}

void NiceClipboard::onBtnSortClicked()
{

}

void NiceClipboard::onBtnSettingsClicked()
{
    //QDialog* dialog = new QDialog(this);
    //dialog->setAttribute(Qt::WA_DeleteOnClose);
    //auto layout = new QGridLayout();
    //layout->setSpacing(0);
    //layout->setContentsMargins(0, 0, 0, 0);
    //dialog->setLayout(layout);
    struct SettingsWidget1 : public SettingsWidget
    {
        std::mutex stateMtx;
        bool needRestart = false;
        bool needAdmin = false;
        bool mainWindowStyleUpdated = false;
        QList<QPair<QString/*key*/, QVariant/*value*/>> pendingConfigChanges;
        using SettingsWidget::SettingsWidget;
    };
    SettingsWidget1* settingsWidget = new SettingsWidget1(this);
    settingsWidget->setAttribute(Qt::WA_DeleteOnClose);
    //dialog->resize(settingsWidget->size());
    //layout->addWidget(settingsWidget);
    connect(settingsWidget, &SettingsWidget::configChanged, this, [this, settingsWidget](const QString& key, const QVariant& value, const QVariant& oldValue) {
        logger.info("Config changed, write config: {} = {}", key.toStdString(), value.toString().toStdString());
        GlobalConfigManager::instance()->write(key, value);
        GlobalConfigManager::instance()->sync();
        std::lock_guard lock(settingsWidget->stateMtx);
        if (key == GlobalConfigManager::configKeyToString(GlobalConfigManager::StartWithWindows))
        {
            if (value.toBool())
            {
                if (!checkAndRegisterAutoRunScheduledTask())
                {
                    settingsWidget->needAdmin = true;
                    settingsWidget->pendingConfigChanges.append({ key, value });
                }
            }
            else
            {
                if (!checkAndUnregisterAutoRunScheduledTask())
                {
                    settingsWidget->needAdmin = true;
                    settingsWidget->pendingConfigChanges.append({ key, value });
                }
            }
        }
        else if (key == GlobalConfigManager::configKeyToString(GlobalConfigManager::ShowInSystemTray))
        {
            if (value.toBool())
                systemTray->show();
            else
                systemTray->hide();
        }
        else if (key == GlobalConfigManager::configKeyToString(GlobalConfigManager::SavedDataFilePath))
            settingsWidget->needRestart = true;
        else if (key == GlobalConfigManager::configKeyToString(GlobalConfigManager::SavedDataImagePath))
            settingsWidget->needRestart = true;
        else if (key == GlobalConfigManager::configKeyToString(GlobalConfigManager::QSSMainWindowStyle))
            settingsWidget->mainWindowStyleUpdated = true;
    });
    connect(settingsWidget, &SettingsWidget::hotKeyChanged, this, [this](const QString& key, const QKeySequence& keySeq, const QKeySequence& oldKeySeq) {
        logger.info("Hotkey changed, update hotkey: {} = {}, old hotkey: {}", key.toStdString(), keySeq.toString().toStdString(), oldKeySeq.toString().toStdString());
        unregisterHotKey(oldKeySeq[0]);
        registerHotKey(keySeq[0]);
    });
    connect(settingsWidget, &SettingsWidget::configSaved, this, [this, settingsWidget] {
        std::lock_guard lock(settingsWidget->stateMtx);
        if (settingsWidget->mainWindowStyleUpdated)
        {
            settingsWidget->mainWindowStyleUpdated = false;
            // 更新主窗口样式
            updateMainWindowStyle();
        }
        // 显示需要重启的提示框
        if (settingsWidget->needRestart)
        {
            settingsWidget->needRestart = false;
            QMessageBox::information(settingsWidget, QObject::tr("配置已更新"), QObject::tr("剪贴板数据保存路径已更新，请重启程序以应用更改"));
        }
        if (settingsWidget->needAdmin)
        {
            settingsWidget->needAdmin = false;
            logger.info("Attempting to relaunch as administrator...");
            restartAsAdminAndWaitForResponse(settingsWidget->pendingConfigChanges);
        }
    });
    settingsWidget->show();
    //dialog->show();
}

