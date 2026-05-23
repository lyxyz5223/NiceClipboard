#include "ScheduledTask.h"
#include <atlbase.h>
#include <atlcom.h>
#include <comutil.h>
#include <taskschd.h>
#pragma comment(lib, "taskschd.lib")
// Unicode支持
#pragma comment(lib, "comsuppw.lib")
// 多字节支持
#pragma comment(lib, "comsupp.lib")

class ScheduledTask::Impl {
public:
    Impl(Logger& logger) : logger(logger) {}
    ~Impl() {
        if (pRegisteredTask)
            pRegisteredTask->Release();
        if (pRootTaskFolder)
            pRootTaskFolder->Release();
        if (pTriggerCollection)
            pTriggerCollection->Release();
        if (pActionCollection)
            pActionCollection->Release();
        if (m_taskDefinition)
            m_taskDefinition->Release();
        if (m_taskService)
            m_taskService->Release();
    }
private:
    friend class ScheduledTask;
    ITaskService* m_taskService{ nullptr };
    ITaskDefinition* m_taskDefinition{ nullptr };
    Logger& logger;

    ITriggerCollection* pTriggerCollection{ nullptr };
    IActionCollection* pActionCollection{ nullptr };

    ITaskFolder* pRootTaskFolder{ nullptr };
    IRegisteredTask* pRegisteredTask{ nullptr };

    ITriggerCollection* getTriggerCollection() {
        if (!pTriggerCollection)
        {
            HRESULT hr = m_taskDefinition->get_Triggers(&pTriggerCollection);
            if (FAILED(hr))
            {
                logger.error("Failed to get trigger collection: 0x{:X}", static_cast<unsigned long>(hr));
                return nullptr;
            }
        }
        return pTriggerCollection;
    }

    IActionCollection* getActionCollection() {
        if (!pActionCollection)
        {
            HRESULT hr = m_taskDefinition->get_Actions(&pActionCollection);
            if (FAILED(hr))
            {
                logger.error("Failed to get action collection: 0x{:X}", static_cast<unsigned long>(hr));
                return nullptr;
            }
        }
        return pActionCollection;
    }

    ITaskFolder* getRootTaskFolder() {
        if (!pRootTaskFolder)
        {
            HRESULT hr = m_taskService->GetFolder(_bstr_t(L"\\"), &pRootTaskFolder);
            if (FAILED(hr))
            {
                logger.error("Failed to get root task folder: 0x{:X}", static_cast<unsigned long>(hr));
                return nullptr;
            }
        }
        return pRootTaskFolder;
    }

    constexpr TASK_TRIGGER_TYPE2 win32TaskTriggerTypeFromTriggerType(TriggerType type) const {
        switch (type)
        {
        case TriggerType::Event:
            return TASK_TRIGGER_EVENT;
        case TriggerType::Time:
            return TASK_TRIGGER_TIME;
        case TriggerType::Daily:
            return TASK_TRIGGER_DAILY;
        case TriggerType::Weekly:
            return TASK_TRIGGER_WEEKLY;
        case TriggerType::MonthlyDayOfMonth:
            return TASK_TRIGGER_MONTHLY;
        case TriggerType::MonthlyDayOfWeek:
            return TASK_TRIGGER_MONTHLYDOW;
        case TriggerType::Idle:
            return TASK_TRIGGER_IDLE;
        case TriggerType::CreateOrModifyTask:
            return TASK_TRIGGER_REGISTRATION;
        case TriggerType::Boot:
            return TASK_TRIGGER_BOOT;
        case TriggerType::Logon:
            return TASK_TRIGGER_LOGON;
        case TriggerType::SessionStateChange:
            return TASK_TRIGGER_SESSION_STATE_CHANGE;
        default:
            logger.error("Unknown trigger type: {}", static_cast<int>(type));
            return TASK_TRIGGER_CUSTOM_TRIGGER_01; // 默认返回一个值
        }
    }

    constexpr TASK_ACTION_TYPE win32TaskActionTypeFromActionType(ActionType type) const {
        switch (type)
        {
        case ActionType::Execute:
            return TASK_ACTION_EXEC;
        case ActionType::ComHandler:
            return TASK_ACTION_COM_HANDLER;
        case ActionType::SendEmail:
            return TASK_ACTION_SEND_EMAIL;
        case ActionType::ShowMessage:
            return TASK_ACTION_SHOW_MESSAGE;
        default:
            return TASK_ACTION_EXEC;
        }
    }

    constexpr TASK_SESSION_STATE_CHANGE_TYPE win32SessionStateChangeTypeFromSessionStateChangeType(SessionStateChangeType type) const {
        switch (type)
        {
        case SessionStateChangeType::ConsoleConnect:
            return TASK_CONSOLE_CONNECT;
        case SessionStateChangeType::ConsoleDisconnect:
            return TASK_CONSOLE_DISCONNECT;
        case SessionStateChangeType::RemoteConnect:
            return TASK_REMOTE_CONNECT;
        case SessionStateChangeType::RemoteDisconnect:
            return TASK_REMOTE_DISCONNECT;
        case SessionStateChangeType::SessionLock:
            return TASK_SESSION_LOCK;
        case SessionStateChangeType::SessionUnlock:
            return TASK_SESSION_UNLOCK;
        default:
            logger.error("Unknown session state change type: {}", static_cast<int>(type));
            return TASK_SESSION_STATE_CHANGE_TYPE(0);
        }
    }

    constexpr short win32DaysOfWeekFromDayOfWeek(unsigned char dayOfWeek) const {
        switch (dayOfWeek)
        {
        case 7:
            return 0x01; // 周日
        case 1:
            return 0x02; // 周一
        case 2:
            return 0x04; // 周二
        case 3:
            return 0x08; // 周三
        case 4:
            return 0x10; // 周四
        case 5:
            return 0x20; // 周五
        case 6:
            return 0x40; // 周六
        default:
            return 0;
        }
    }

    short win32DaysOfWeekFromDaysOfWeek(const std::vector<unsigned char>& daysOfWeek) const {
        short result = 0;
        for (const auto& dayOfWeek : daysOfWeek)
            result |= win32DaysOfWeekFromDayOfWeek(dayOfWeek);
        return result;
    }
    
    constexpr short win32WeeksOfMonthFromWeekOfMonth(unsigned char weekOfMonth) const {
        switch (weekOfMonth)
        {
        case 1:
            return 0x01; // 第一周
        case 2:
            return 0x02; // 第二周
        case 3:
            return 0x04; // 第三周
        case 4:
            return 0x08; // 第四周
        default:
            return 0;
        }
    }

    short win32WeeksOfMonthFromWeeksOfMonth(const std::vector<unsigned char>& weeksOfMonth) const {
        short result = 0;
        for (const auto& weekOfMonth : weeksOfMonth)
            result |= win32WeeksOfMonthFromWeekOfMonth(weekOfMonth);
        return result;
    }

    constexpr short win32MonthsOfYearFromMonthOfYear(unsigned char monthOfYear) const {
        switch (monthOfYear)
        {
        case 1:
            return 0x01; // 1月
        case 2:
            return 0x02; // 2月
        case 3:
            return 0x04; // 3月
        case 4:
            return 0x08; // 4月
        case 5:
            return 0x10; // 5月
        case 6:
            return 0x20; // 6月
        case 7:
            return 0x40; // 7月
        case 8:
            return 0x80; // 8月
        case 9:
            return 0x100; // 9月
        case 10:
            return 0x200; // 10月
        case 11:
            return 0x400; // 11月
        case 12:
            return 0x800; // 12月
        default:
            return 0;
        }
    }

    short win32MonthsOfYearFromMonthsOfYear(const std::vector<unsigned char>& monthsOfYear) const {
        short result = 0;
        for (const auto& monthOfYear : monthsOfYear)
            result |= win32MonthsOfYearFromMonthOfYear(monthOfYear);
        return result;
    }

    constexpr long win32DaysOfMonthFromDayOfMonth(unsigned char dayOfMonth) const {
        switch (dayOfMonth)
        {
        case 1:
            return 0x1;
        case 2:
            return 0x2;
        case 3:
            return 0x4;
        case 4:
            return 0x8;
        case 5:
            return 0x10;
        case 6:
            return 0x20;
        case 7:
            return 0x40;
        case 8:
            return 0x80;
        case 9:
            return 0x100;
        case 10:
            return 0x200;
        case 11:
            return 0x400;
        case 12:
            return 0x800;
        case 13:
            return 0x1000;
        case 14:
            return 0x2000;
        case 15:
            return 0x4000;
        case 16:
            return 0x8000;
        case 17:
            return 0x10000;
        case 18:
            return 0x20000;
        case 19:
            return 0x40000;
        case 20:
            return 0x80000;
        case 21:
            return 0x100000;
        case 22:
            return 0x200000;
        case 23:
            return 0x400000;
        case 24:
            return 0x800000;
        case 25:
            return 0x1000000;
        case 26:
            return 0x2000000;
        case 27:
            return 0x4000000;
        case 28:
            return 0x8000000;
        case 29:
            return 0x10000000;
        case 30:
            return 0x20000000;
        case 31:
            return 0x40000000;
        default:
            return 0;
        }
    }

    long win32DaysOfMonthFromDaysOfMonth(const std::vector<unsigned char>& daysOfMonth) const {
        long result = 0;
        for (const auto& dayOfMonth : daysOfMonth)
            result |= win32DaysOfMonthFromDayOfMonth(dayOfMonth);
        return result;
    }

};

#define m_taskService pImpl->m_taskService
#define m_taskDefinition pImpl->m_taskDefinition


ScheduledTask::ScheduledTask()
    : pImpl(std::make_unique<Impl>(logger))
{
}

ScheduledTask::ScheduledTask(const std::string& taskName, const std::string& creator, const std::string& taskDescription, bool hidden, bool runWithHighestLevel, const std::string& userId, bool runOnlyAfterUserLogon)
    : pImpl(std::make_unique<Impl>(logger)), taskName(taskName), creator(creator), taskDescription(taskDescription), userId(userId), runOnlyAfterUserLogon(runOnlyAfterUserLogon), runWithHighestLevel(runWithHighestLevel), hidden(hidden)
{
    if (!runOnlyAfterUserLogon)
        throw std::runtime_error("Only tasks that run after user logon are supported for now");
    createService();
    connectService();
}

ScheduledTask::~ScheduledTask()
{

}

ScheduledTask::TaskResult ScheduledTask::createService()
{
    if (m_taskService)
        return TRAlreadyExists;
    // 创建任务计划服务对象
    HRESULT hr = CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&m_taskService);
    if (FAILED(hr))
    {
        logger.error("Failed to create task service instance: 0x{:X}", static_cast<unsigned long>(hr));
        return TRServiceCreateFailed;
    }
    return TRSuccess;
}

ScheduledTask::TaskResult ScheduledTask::connectService()
{
    VARIANT_BOOL connected = VARIANT_FALSE;
    HRESULT hr = m_taskService->get_Connected(&connected);
    if (FAILED(hr))
        logger.error("Failed to get task service connection status: 0x{:X}", static_cast<unsigned long>(hr));
    else if (connected == VARIANT_TRUE)
        return TRServiceAlreadyConnected;
    // 连接到任务计划服务
    hr = m_taskService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr))
    {
        logger.error("Failed to connect to task service: 0x{:X}", static_cast<unsigned long>(hr));
        return TRServiceConnectFailed;
    }
    return TRSuccess;
}

ScheduledTask::TaskResult ScheduledTask::createTaskDefinition()
{
    if (m_taskDefinition)
        return TRAlreadyExists;
    // 创建一个新的任务定义
    HRESULT hr = m_taskService->NewTask(0, &m_taskDefinition);
    if (FAILED(hr))
    {
        logger.error("Failed to create task definition: 0x{:X}", static_cast<unsigned long>(hr));
        return TRDefinitionCreateFailed;
    }
    // 设置任务的注册信息
    IRegistrationInfo* pRegInfo = nullptr;
    hr = m_taskDefinition->get_RegistrationInfo(&pRegInfo);
    if (FAILED(hr))
    {
        logger.error("Failed to get registration info: 0x{:X}", static_cast<unsigned long>(hr));
        return TRDefinitionGetRegistrationInfoFailed;
    }
    hr = pRegInfo->put_Author(_bstr_t(creator.c_str()));
    if (FAILED(hr))
    {
        logger.error("Failed to set author: 0x{:X}", static_cast<unsigned long>(hr));
        return TRDefinitionSetCreatorFailed;
    }
    hr = pRegInfo->put_Description(_bstr_t(taskDescription.c_str()));
    if (FAILED(hr))
    {
        logger.error("Failed to set description: 0x{:X}", static_cast<unsigned long>(hr));
        return TRDefinitionSetDescriptionFailed;
    }
    return TRSuccess;
}

ScheduledTask::TaskResult ScheduledTask::setTrigger(const Trigger& trigger)
{
    return setTriggers({ trigger });
}

ScheduledTask::TaskResult ScheduledTask::setTriggers(const std::vector<Trigger>& triggers)
{
    auto* pTriggerCollection = pImpl->getTriggerCollection();
    if (!pTriggerCollection)
        return TRTriggerGetCollectionFailed;
    // 清除现有触发器
    auto hr = pTriggerCollection->Clear();
    if (FAILED(hr))
    {
        logger.error("Failed to clear existing triggers: 0x{:X}", static_cast<unsigned long>(hr));
        return TRTriggerClearFailed;
    }
    return addTriggersPrivate(triggers, pTriggerCollection);
}

ScheduledTask::TaskResult ScheduledTask::addTriggers(const std::vector<Trigger>& triggers)
{
    auto* pTriggerCollection = pImpl->getTriggerCollection();
    if (!pTriggerCollection)
        return TRTriggerGetCollectionFailed;
    return addTriggersPrivate(triggers, pTriggerCollection);
}

ScheduledTask::TaskResult ScheduledTask::addTrigger(const Trigger& trigger)
{
    auto* pTriggerCollection = pImpl->getTriggerCollection();
    if (!pTriggerCollection)
        return TRTriggerGetCollectionFailed;
    return addTriggerPrivate(trigger, pTriggerCollection);
}


ScheduledTask::TaskResult ScheduledTask::addTriggerPrivate(const Trigger& trigger, void* pTriggerCollection)
{
    auto triggerCollection = static_cast<ITriggerCollection*>(pTriggerCollection);
    ITrigger* pTrigger = nullptr;
    HRESULT hr = triggerCollection->Create(pImpl->win32TaskTriggerTypeFromTriggerType(trigger.type), &pTrigger);
    if (FAILED(hr))
    {
        logger.error("Failed to create trigger: 0x{:X}", static_cast<unsigned long>(hr));
        return TRTriggerCreateFailed;
    }
    const TriggerDataBase& triggerData = std::visit([](auto& obj) -> const TriggerDataBase& { return obj; }, trigger.data);
    if (triggerData.repetitionInterval)
    {
        IRepetitionPattern* pRepetitionPattern = nullptr;
        hr = pTrigger->get_Repetition(&pRepetitionPattern);
        if (FAILED(hr))
        {
            logger.error("Failed to get repetition pattern: 0x{:X}", static_cast<unsigned long>(hr));
            pTrigger->Release();
            return TRTriggerGetRepetitionPatternFailed;
        }
        pRepetitionPattern->put_Interval(_bstr_t(secondsToISO8601String(triggerData.repetitionInterval).c_str()));
        if (triggerData.repetitionDuration)
            pRepetitionPattern->put_Duration(_bstr_t(secondsToISO8601String(triggerData.repetitionDuration).c_str()));
        pRepetitionPattern->put_StopAtDurationEnd(triggerData.stopAtDurationEnd ? VARIANT_TRUE : VARIANT_FALSE);
        pTrigger->put_Repetition(pRepetitionPattern);
        pRepetitionPattern->Release();
    }
    if (!triggerData.startBoundary.isNull())
        pTrigger->put_StartBoundary(_bstr_t(dateTimeToISO8601String(triggerData.startBoundary).c_str()));
    if (!triggerData.endBoundary.isNull())
        pTrigger->put_EndBoundary(_bstr_t(dateTimeToISO8601String(triggerData.endBoundary).c_str()));
    if (triggerData.executeTimeLimit)
        pTrigger->put_ExecutionTimeLimit(_bstr_t(secondsToISO8601String(triggerData.executeTimeLimit).c_str()));
    if (trigger.id.size())
        pTrigger->put_Id(_bstr_t(trigger.id.c_str()));
    pTrigger->put_Enabled(trigger.enabled ? VARIANT_TRUE : VARIANT_FALSE);
    switch (trigger.type)
    {
    case TriggerType::Event:
    {
        IEventTrigger* pTargetTrigger = nullptr;
        HRESULT hr = pTrigger->QueryInterface(IID_IEventTrigger, (void**)&pTargetTrigger);
        if (SUCCEEDED(hr))
        {
            auto& eventData = std::get<EventTriggerData>(trigger.data);
            if (eventData.delay)
                pTargetTrigger->put_Delay(_bstr_t(secondsToISO8601String(eventData.delay).c_str()));
            pTargetTrigger->put_Subscription(_bstr_t(eventData.subscription.c_str()));
            pTargetTrigger->Release();
        }
        break;
    }
    case TriggerType::Time:
    {
        ITimeTrigger* pTargetTrigger = nullptr;
        HRESULT hr = pTrigger->QueryInterface(IID_ITimeTrigger, (void**)&pTargetTrigger);
        if (SUCCEEDED(hr))
        {
            auto& eventData = std::get<TimeTriggerData>(trigger.data);
            if (eventData.maxRandomDelay)
                pTargetTrigger->put_RandomDelay(_bstr_t(secondsToISO8601String(eventData.maxRandomDelay).c_str()));
            pTargetTrigger->Release();
        }
        break;
    }
    case TriggerType::Daily:
    {
        IDailyTrigger* pTargetTrigger = nullptr;
        HRESULT hr = pTrigger->QueryInterface(IID_IDailyTrigger, (void**)&pTargetTrigger);
        if (SUCCEEDED(hr))
        {
            auto& eventData = std::get<DailyTriggerData>(trigger.data);
            if (eventData.maxRandomDelay)
                pTargetTrigger->put_RandomDelay(_bstr_t(secondsToISO8601String(eventData.maxRandomDelay).c_str()));
            pTargetTrigger->put_DaysInterval(eventData.daysInterval);
            pTargetTrigger->Release();
        }
        break;
    }
    case TriggerType::Weekly:
    {
        IWeeklyTrigger* pTargetTrigger = nullptr;
        HRESULT hr = pTrigger->QueryInterface(IID_IWeeklyTrigger, (void**)&pTargetTrigger);
        if (SUCCEEDED(hr))
        {
            auto& eventData = std::get<WeeklyTriggerData>(trigger.data);
            if (eventData.maxRandomDelay)
                pTargetTrigger->put_RandomDelay(_bstr_t(secondsToISO8601String(eventData.maxRandomDelay).c_str()));
            pTargetTrigger->put_WeeksInterval(eventData.weeksInterval);
            pTargetTrigger->put_DaysOfWeek(pImpl->win32DaysOfWeekFromDaysOfWeek(eventData.daysOfWeek));
            pTargetTrigger->Release();
        }
        break;
    }
    case TriggerType::MonthlyDayOfMonth:
    {
        IMonthlyTrigger* pTargetTrigger = nullptr;
        HRESULT hr = pTrigger->QueryInterface(IID_IMonthlyTrigger, (void**)&pTargetTrigger);
        if (SUCCEEDED(hr))
        {
            auto& eventData = std::get<MonthlyDayOfMonthTriggerData>(trigger.data);
            if (eventData.maxRandomDelay)
                pTargetTrigger->put_RandomDelay(_bstr_t(secondsToISO8601String(eventData.maxRandomDelay).c_str()));
            pTargetTrigger->put_MonthsOfYear(pImpl->win32MonthsOfYearFromMonthsOfYear(eventData.monthsOfYear));
            pTargetTrigger->put_DaysOfMonth(pImpl->win32DaysOfMonthFromDaysOfMonth(eventData.daysOfMonth));
            pTargetTrigger->put_RunOnLastDayOfMonth(eventData.runOnLastDayOfMonth ? VARIANT_TRUE : VARIANT_FALSE);
            pTargetTrigger->Release();
        }
        break;
    }
    case TriggerType::MonthlyDayOfWeek:
    {
        IMonthlyDOWTrigger* pTargetTrigger = nullptr;
        HRESULT hr = pTrigger->QueryInterface(IID_IMonthlyDOWTrigger, (void**)&pTargetTrigger);
        if (SUCCEEDED(hr))
        {
            auto& eventData = std::get<MonthlyDayOfWeekTriggerData>(trigger.data);
            if (eventData.maxRandomDelay)
                pTargetTrigger->put_RandomDelay(_bstr_t(secondsToISO8601String(eventData.maxRandomDelay).c_str()));
            pTargetTrigger->put_MonthsOfYear(pImpl->win32MonthsOfYearFromMonthsOfYear(eventData.monthsOfYear));
            pTargetTrigger->put_WeeksOfMonth(pImpl->win32WeeksOfMonthFromWeeksOfMonth(eventData.weeksOfMonth));
            pTargetTrigger->put_DaysOfWeek(pImpl->win32DaysOfWeekFromDaysOfWeek(eventData.daysOfWeek));
            pTargetTrigger->Release();
        }
        break;
    }
    case TriggerType::Idle:
    {// 无特殊数据，无需特殊处理
        //IIdleTrigger* pTargetTrigger = nullptr;
        //HRESULT hr = pTrigger->QueryInterface(IID_IIdleTrigger, (void**)&pTargetTrigger);
        //if (SUCCEEDED(hr))
        //{
        //    auto& eventData = std::get<IdleTriggerData>(trigger.data);
        //    pTargetTrigger->Release();
        //}
        break;
    }
    case TriggerType::CreateOrModifyTask:
    {
        IRegistrationTrigger* pTargetTrigger = nullptr;
        HRESULT hr = pTrigger->QueryInterface(IID_IRegistrationTrigger, (void**)&pTargetTrigger);
        if (SUCCEEDED(hr))
        {
            auto& eventData = std::get<CreateOrModifyTaskTriggerData>(trigger.data);
            if (eventData.delay)
                pTargetTrigger->put_Delay(_bstr_t(secondsToISO8601String(eventData.delay).c_str()));
            pTargetTrigger->Release();
        }
        break;
    }
    case TriggerType::Boot:
    {
        IBootTrigger* pTargetTrigger = nullptr;
        HRESULT hr = pTrigger->QueryInterface(IID_IBootTrigger, (void**)&pTargetTrigger);
        if (SUCCEEDED(hr))
        {
            auto& eventData = std::get<BootTriggerData>(trigger.data);
            if (eventData.delay)
                pTargetTrigger->put_Delay(_bstr_t(secondsToISO8601String(eventData.delay).c_str()));
            pTargetTrigger->Release();
        }
        break;
    }
    case TriggerType::Logon:
    {
        ILogonTrigger* pTargetTrigger = nullptr;
        HRESULT hr = pTrigger->QueryInterface(IID_ILogonTrigger, (void**)&pTargetTrigger);
        if (SUCCEEDED(hr))
        {
            auto& eventData = std::get<LogonTriggerData>(trigger.data);
            if (eventData.delay)
                pTargetTrigger->put_Delay(_bstr_t(secondsToISO8601String(eventData.delay).c_str()));
            pTargetTrigger->put_UserId(_bstr_t(eventData.userId.c_str()));
            pTargetTrigger->Release();
        }
        break;
    }
    case TriggerType::SessionStateChange:
    {
        ISessionStateChangeTrigger* pTargetTrigger = nullptr;
        HRESULT hr = pTrigger->QueryInterface(IID_ISessionStateChangeTrigger, (void**)&pTargetTrigger);
        if (SUCCEEDED(hr))
        {
            auto& eventData = std::get<SessionStateChangeTriggerData>(trigger.data);
            if (eventData.delay)
                pTargetTrigger->put_Delay(_bstr_t(secondsToISO8601String(eventData.delay).c_str()));
            pTargetTrigger->put_StateChange(pImpl->win32SessionStateChangeTypeFromSessionStateChangeType(eventData.stateChangeType));
            pTargetTrigger->put_UserId(_bstr_t(eventData.userId.c_str()));
            pTargetTrigger->Release();
        }
        break;
    }
    default:
        break;
    }
    pTrigger->Release();
    return TRSuccess;
}

ScheduledTask::TaskResult ScheduledTask::addTriggersPrivate(const std::vector<Trigger>& triggers, void* pTriggerCollection)
{
    TaskResult rst = TRSuccess;
    for (auto& trigger : triggers)
    {
        TaskResult r = TRSuccess;
        if ((r = addTriggerPrivate(trigger, pTriggerCollection)) != TRSuccess)
        {
            rst = r;
            logger.error("Failed to add trigger of type {}", static_cast<int>(trigger.type));
        }
    }
    return rst;
}


ScheduledTask::TaskResult ScheduledTask::removeTrigger(size_t index)
{
    auto* pTriggerCollection = pImpl->getTriggerCollection();
    if (!pTriggerCollection)
        return TRTriggerGetCollectionFailed;
    _variant_t vIndex(index);
    HRESULT hr = pTriggerCollection->Remove(vIndex);
    if (FAILED(hr))
    {
        logger.error("Failed to remove trigger at index {}: 0x{:X}", index, static_cast<unsigned long>(hr));
        return TRTriggerRemoveFailed;
    }
    return TRSuccess;
}

ScheduledTask::TaskResult ScheduledTask::addAction(const Action& action)
{
    auto pActionCollection = pImpl->getActionCollection();
    if (!pActionCollection)
        return TRActionGetCollectionFailed;
    return addActionPrivate(action, pActionCollection);
}

ScheduledTask::TaskResult ScheduledTask::addActions(const std::vector<Action>& actions)
{
    auto pActionCollection = pImpl->getActionCollection();
    if (!pActionCollection)
        return TRActionGetCollectionFailed;
    return addActionsPrivate(actions, pActionCollection);
}

ScheduledTask::TaskResult ScheduledTask::setAction(const Action& action)
{
    return setActions({ action });
}

ScheduledTask::TaskResult ScheduledTask::setActions(const std::vector<Action>& actions)
{
    auto pActionCollection = pImpl->getActionCollection();
    if (!pActionCollection)
        return TRActionGetCollectionFailed;
    auto hr = pActionCollection->Clear();
    if (FAILED(hr))
    {
        logger.error("Failed to clear existing actions: 0x{:X}", static_cast<unsigned long>(hr));
        return TRActionClearFailed;
    }
    return addActionsPrivate(actions, pActionCollection);
}


ScheduledTask::TaskResult ScheduledTask::addActionPrivate(const Action& action, void* pActionCollection)
{
    IAction* pAction = nullptr;
    HRESULT hr = static_cast<IActionCollection*>(pActionCollection)->Create(pImpl->win32TaskActionTypeFromActionType(action.type), &pAction);
    if (FAILED(hr))
    {
        logger.error("Failed to create action: 0x{:X}", static_cast<unsigned long>(hr));
        return TRActionCreateFailed;
    }
    pAction->put_Id(_bstr_t(action.id.c_str()));
    switch (action.type)
    {
    case ActionType::Execute:
    {
        IExecAction* pTargetAction = nullptr;
        HRESULT hr = pAction->QueryInterface(IID_IExecAction, (void**)&pTargetAction);
        if (SUCCEEDED(hr))
        {
            auto& actionData = std::get<ExecuteActionData>(action.data);
            pTargetAction->put_Path(_bstr_t(actionData.executable.c_str()));
            pTargetAction->put_Arguments(_bstr_t(actionData.arguments.c_str()));
            pTargetAction->put_WorkingDirectory(_bstr_t(actionData.workingDirectory.c_str()));
            pTargetAction->Release();
        }
        break;
    }
    case ActionType::ComHandler:
    {
        throw std::runtime_error("ComHandler action type is not supported for now, and this is deprecated, use Execute instead.");
        break;
    }
    case ActionType::SendEmail:
    {
        throw std::runtime_error("SendEmail action type is not supported for now, and this is deprecated, use Execute instead.");
        break;
    }
    case ActionType::ShowMessage:
    {
        throw std::runtime_error("ShowMessage action type is not supported for now, and this is deprecated, use Execute instead.");
        break;
    }
    default:
        break;
    }
    pAction->Release();
    return TRSuccess;
}

ScheduledTask::TaskResult ScheduledTask::addActionsPrivate(const std::vector<Action>& actions, void* pActionCollection)
{
    TaskResult rst = TRSuccess;
    for (auto& action : actions)
    {
        TaskResult r = TRSuccess;
        if ((r = addActionPrivate(action, pActionCollection)) != TRSuccess)
        {
            rst = r;
            logger.error("Failed to add action of type {}", static_cast<int>(action.type));
        }
    }
    return rst;
}


ScheduledTask::TaskResult ScheduledTask::removeAction(size_t index)
{
    auto* pActionCollection = pImpl->getActionCollection();
    if (!pActionCollection)
        return TRActionGetCollectionFailed;
    _variant_t vIndex(index);
    HRESULT hr = pActionCollection->Remove(vIndex);
    if (FAILED(hr))
    {
        logger.error("Failed to remove action at index {}: 0x{:X}", index, static_cast<unsigned long>(hr));
        return TRActionRemoveFailed;
    }
    return TRSuccess;
}


std::string ScheduledTask::secondsToISO8601String(size_t seconds)
{
    DateTime dateTime;
    dateTime.year = 0;
    dateTime.month = 0;
    dateTime.day = seconds / 60 / 60 / 24; // /60=minute, minute/60=hour, hour/24=day
    dateTime.hour = (seconds / 60 / 60) % 24;
    dateTime.minute = (seconds / 60) % 60;
    dateTime.second = seconds % 60;
    return dateTimeToISO8601String(dateTime);
}

std::string ScheduledTask::dateTimeToISO8601String(DateTime duration)
{
    //return std::string("P{}Y{}M{}DT{}H{}M{}S");
    std::string r = "P";
    if (duration.year)
        r += std::to_string(duration.year) + "Y";
    if (duration.month)
        r += std::to_string(duration.month) + "M";
    if (duration.day)
        r += std::to_string(duration.day) + "D";
    r += "T";
    if (duration.hour)
        r += std::to_string(duration.hour) + "H";
    if (duration.minute)
        r += std::to_string(duration.minute) + "M";
    r += std::to_string(duration.second) + "S";
    return r;
}

std::string ScheduledTask::ensurePath(const std::string& path)
{
    if (path.size() && path.back() != '\\' && path.back() != '/')
        return path + '\\';
    return path;
}


ScheduledTask::TaskResult ScheduledTask::registerTask(const std::string& folderPath, bool overwriteExisting, bool createFolderIfNotExists)
{
    if (isTaskRegistered())
        return TRAlreadyExists;
    auto* pRootFolder = pImpl->getRootTaskFolder();
    if (!pRootFolder)
        return TRFolderGetRootFailed;
    HRESULT hr = pRootFolder->RegisterTaskDefinition(_bstr_t((ensurePath(folderPath) + taskName).c_str()), m_taskDefinition, overwriteExisting ? TASK_CREATE_OR_UPDATE : 0, _variant_t(userId.c_str()), _variant_t(), TASK_LOGON_INTERACTIVE_TOKEN, _variant_t(), &pImpl->pRegisteredTask);
    if (FAILED(hr))
    {
        logger.error("Failed to register task: 0x{:X}", static_cast<unsigned long>(hr));
#ifdef _DEBUG
        BSTR xml = nullptr;
        HRESULT hrXml = m_taskDefinition->get_XmlText(&xml);
        if (SUCCEEDED(hrXml) && xml)
        {
            std::wstring wXml(xml);
            std::string xmlStr(wXml.begin(), wXml.end());
            logger.error("Task XML to register:\n{}", xmlStr);
            SysFreeString(xml);
        }
#endif
        return TRDefinitionRegisterFailed;
    }
    return TRSuccess;
}

ScheduledTask::TaskResult ScheduledTask::getRegisteredTask(const std::string folderPath)
{
    if (isTaskRegistered())
        return TRAlreadyExists;
    auto* pRootFolder = pImpl->getRootTaskFolder();
    if (!pRootFolder)
        return TRFolderGetRootFailed;
    HRESULT hr = pRootFolder->GetTask(_bstr_t((ensurePath(folderPath) + taskName).c_str()), &pImpl->pRegisteredTask);
    if (FAILED(hr))
    {
        logger.error("Failed to get registered task: 0x{:X}", static_cast<unsigned long>(hr));
        return TRTaskGetFailed;
    }
    return TRSuccess;
}

ScheduledTask::TaskResult ScheduledTask::getRegisteredTaskDefinition()
{
    if (!isTaskRegistered())
        return TRNotExists;
    if (m_taskDefinition)
        return TRSuccess;
    HRESULT hr = pImpl->pRegisteredTask->get_Definition(&m_taskDefinition);
    if (FAILED(hr))
    {
        logger.error("Failed to get registered task definition: 0x{:X}", static_cast<unsigned long>(hr));
        return TRDefinitionGetFailed;
    }
    return TRSuccess;
}

ScheduledTask::TaskResult ScheduledTask::runTask()
{
    if (!isTaskRegistered())
        return TRNotExists;
    IRunningTask* runningTask = nullptr;
    HRESULT hr = pImpl->pRegisteredTask->Run(_variant_t(), &runningTask);
    if (FAILED(hr))
    {
        logger.error("Failed to run task: 0x{:X}", static_cast<unsigned long>(hr));
        return TRTaskRunFailed;
    }
    runningTask->Release();
    return TRSuccess;
}

ScheduledTask::TaskResult ScheduledTask::unregisterTask(const std::string& folderPath)
{
    return deleteTask(folderPath);
}

ScheduledTask::TaskResult ScheduledTask::deleteTask(const std::string& folderPath)
{
    if (!isTaskRegistered())
        return TRNotExists;
    auto* pRootFolder = pImpl->getRootTaskFolder();
    if (!pRootFolder)
        return TRFolderGetRootFailed;
    HRESULT hr = pRootFolder->DeleteTask(_bstr_t((ensurePath(folderPath) + taskName).c_str()), 0);
    if (FAILED(hr))
    {
        logger.error("Failed to delete task: 0x{:X}", static_cast<unsigned long>(hr));
        return TRTaskDeleteFailed;
    }
    pImpl->pRegisteredTask->Release();
    pImpl->pRegisteredTask = nullptr;
    return TRSuccess;
}

bool ScheduledTask::isTaskRegistered()
{
    return pImpl->pRegisteredTask != nullptr;
}



