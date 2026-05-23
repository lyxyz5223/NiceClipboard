#pragma once
#include <string>
#include <memory>
#include <vector>
#include <Logger.h>
#include <variant>

/// <summary>
/// 封装的计划任务类，支持创建和管理 Windows 任务计划中的任务
/// 但是每个ScheduledTask仅能管理一个任务计划中的一个任务，暂不支持一个ScheduledTask管理多个任务的情况
/// 可通过getRegisteredTask获取已注册任务再通过getRegisteredTaskDefinition获取定义后修改
/// </summary>
class ScheduledTask
{
    class Impl;
    Logger logger{ "ScheduledTask" };
    std::unique_ptr<Impl> pImpl{ nullptr };
    std::string taskName;
    std::string creator;
    std::string taskDescription;
    std::string userId; // 用什么用户运行任务，默认为当前用户
    bool runOnlyAfterUserLogon{ true };
    bool runWithHighestLevel{ false };
    bool hidden{ false };

public:
    enum TaskResult {
        TRSuccess = 0, // TRSuccess表示函数执行成功
        TRAlreadyExists, // TRAlreadyExists表示已被ScheduledTask管理
        TRNotExists, // TRNotExists表示不受ScheduledTask管理
        TRServiceAlreadyConnected,
        TRServiceCreateFailed,
        TRServiceConnectFailed,
        TRDefinitionCreateFailed,
        TRDefinitionGetRegistrationInfoFailed,
        TRDefinitionSetCreatorFailed,
        TRDefinitionSetDescriptionFailed,
        TRDefinitionGetFailed,
        TRTriggerGetCollectionFailed,
        TRTriggerClearFailed,
        TRTriggerCreateFailed,
        TRTriggerGetRepetitionPatternFailed,
        TRTriggerRemoveFailed,
        TRActionGetCollectionFailed,
        TRActionClearFailed,
        TRActionCreateFailed,
        TRActionRemoveFailed,
        TRFolderGetRootFailed,
        TRDefinitionRegisterFailed,
        TRTaskGetFailed,
        TRTaskRunFailed,
        TRTaskDeleteFailed,

        TRNumberOfResults = TRTaskDeleteFailed - TRSuccess + 1
    };

    enum class TriggerType {
        Event, // 当事件触发时
        Time, // 单次
        Daily, // 每天
        Weekly, // 每周
        MonthlyDayOfMonth, // 每月的某些天
        MonthlyDayOfWeek, // 每月的某些周几
        Idle, // 空闲时
        CreateOrModifyTask, // 创建或修改任务时
        Boot, // 启动时
        Logon, // 登录时
        SessionStateChange, // 会话状态变化时，连接、断开、锁定、解锁等
        //Custom, // 用于支持在内核/驱动层定义的自定义触发器，我们的软件不需要支持该功能
    };
    enum class SessionStateChangeType {
        ConsoleConnect = 1, // 会话被连接（登录一个用户）
        ConsoleDisconnect = 2, // 会话被断开（切换用户时，被切出的用户收到改状态改变）
        RemoteConnect = 3, // 会话被远程连接
        RemoteDisconnect = 4, // 会话被远程断开
        SessionLock = 7, // 计算机锁定
        SessionUnlock = 8, // 计算机解锁
    };

    struct DateTime {
        size_t year{ 0 };
        size_t month{ 0 };
        size_t day{ 0 };
        size_t hour{ 0 };
        size_t minute{ 0 };
        size_t second{ 0 };
        bool useUTC{ false }; // 是否使用UTC时间
        bool isNull() const {
            return year == 0 && month == 0 && day == 0 && hour == 0 && minute == 0 && second == 0;
        }
    };
    struct TriggerDataBase {
        virtual ~TriggerDataBase() = default;
        size_t repetitionInterval{ 0 }; // 触发器重复的间隔时间，单位为秒，0表示不重复
        size_t repetitionDuration{ 0 }; // 触发器持续的时间，单位为秒
        bool stopAtDurationEnd{ false }; // 是否在持续时间结束时停止触发器
        size_t executeTimeLimit{ 0 }; // 执行的时间限制，单位为秒，0表示没有限制
        DateTime startBoundary{ 0 }; // 触发器的开始时间
        DateTime endBoundary{ 0 }; // 触发器的结束时间
    };
    struct EventTriggerData : TriggerDataBase {
        size_t delay{ 0 }; // 触发器触发后的延迟时间，单位为秒
        std::string subscription;
        EventTriggerData() {
            throw std::runtime_error("EventTriggerData is not implemented yet");
        }
    };
    struct PlanTriggerData : TriggerDataBase {
        size_t maxRandomDelay{ 0 }; // 触发器触发后的最长随机延迟时间，单位为秒
    };
    struct TimeTriggerData : PlanTriggerData {};
    struct DailyTriggerData : PlanTriggerData {
        size_t daysInterval{ 0 }; // 触发器关联的每天的间隔时间，单位为天，0表示每天，仅使用2字节short类型
    };
    struct WeeklyTriggerData : PlanTriggerData {
        size_t weeksInterval{ 0 }; // 触发器关联的每周的间隔时间，单位为周，0表示每周，仅使用2字节short类型
        std::vector<unsigned char> daysOfWeek; // 在目标周的哪些天触发，1=周一，2=周二，7=周日，依此类推
    };
    struct MonthlyTriggerData : PlanTriggerData {
        std::vector<unsigned char> monthsOfYear; // 在哪些月触发，1=1月，2=2月，依此类推
    };
    struct MonthlyDayOfMonthTriggerData : MonthlyTriggerData {
        std::vector<unsigned char> daysOfMonth; // 在目标月的哪些天触发，1=1日，2=2日，依此类推
        bool runOnLastDayOfMonth{ false }; // 是否在每月的最后一天触发
    };
    struct MonthlyDayOfWeekTriggerData : MonthlyTriggerData {
        std::vector<unsigned char> weeksOfMonth; // 在目标月的哪些周触发，1=第一周，2=第二周，依此类推，5=最后一周
        std::vector<unsigned char> daysOfWeek; // 在目标周的哪些天触发，1=周一，2=周二，7=周日，依此类推
    };

    struct IdleTriggerData : TriggerDataBase {};
    struct CreateOrModifyTaskTriggerData : TriggerDataBase {
        size_t delay{ 0 }; // 触发器触发后的延迟时间，单位为秒
    };
    struct BootTriggerData : TriggerDataBase {
        size_t delay{ 0 }; // 触发器触发后的延迟时间，单位为秒
    };
    struct LogonTriggerData : TriggerDataBase {
        size_t delay{ 0 }; // 触发器触发后的延迟时间，单位为秒
        std::string userId; // 可选，触发器关联的用户（如用户名或者LAPTOP-XXXXXX\用户名），默认为当前用户
    };
    struct SessionStateChangeTriggerData : TriggerDataBase {
        size_t delay{ 0 }; // 触发器触发后的延迟时间，单位为秒
        SessionStateChangeType stateChangeType{ 0 }; // 触发器关联的会话状态变化类型
        std::string userId; // 可选，触发器关联的用户（如用户名或者LAPTOP-XXXXXX\用户名），默认为当前用户
    };


    struct Trigger {
        TriggerType type;
        std::string id; // 可选，触发器的唯一标识符
        bool enabled{ true };
        std::variant<TriggerDataBase, EventTriggerData, TimeTriggerData, DailyTriggerData, WeeklyTriggerData, MonthlyDayOfMonthTriggerData, MonthlyDayOfWeekTriggerData, IdleTriggerData, CreateOrModifyTaskTriggerData, BootTriggerData, LogonTriggerData, SessionStateChangeTriggerData> data;
    };

    enum class ActionType {
        Execute, // 运行一个程序
        ComHandler, // 运行一个COM处理程序，COM处理程序是一个实现了IClonable接口的COM对象，任务计划服务会创建该对象的一个实例并调用其IClonable::Clone方法来执行任务
        SendEmail,// [[deprecated("Use Execute instead")]], // 发送邮件
        ShowMessage,// [[deprecated("Use Execute instead")]], // 显示消息框
    };

    struct ActionDataBase {
        virtual ~ActionDataBase() = default;
    };

    struct ExecuteActionData : ActionDataBase {
        std::string executable; // 要运行的程序/脚本的文件路径/命令（可以是可执行文件、脚本文件或者一个命令（如：echo hello world））
        std::string arguments; // 运行程序的参数
        std::string workingDirectory; // 运行程序的工作目录
    };

    struct Action {
        ActionType type{ ActionType::Execute };
        std::string id; // 可选，动作的唯一标识符
        std::variant<ExecuteActionData> data;
    };

public:
    /// <summary>
    /// 默认构造，不进行任务计划服务的创建与连接
    /// </summary>
    ScheduledTask();

    /// <summary>
    /// 创建并连接到计划任务服务的构造函数
    /// </summary>
    /// <param name="taskName">任务名</param>
    /// <param name="creator">创建者</param>
    /// <param name="taskDescription">任务描述</param>
    /// <param name="hidden">隐藏任务</param>
    /// <param name="runWithHighestLevel">以最高权限运行任务，最高指的是运行用户所具有的最高权限</param>
    /// <param name="userId">运行任务的用户名</param>
    /// <param name="runOnlyAfterUserLogon">仅在登录后运行</param>
    ScheduledTask(const std::string& taskName, const std::string& creator = "", const std::string& taskDescription = "", bool hidden = false, bool runWithHighestLevel = false, const std::string& userId = "", bool runOnlyAfterUserLogon = true);

    ~ScheduledTask();

    void setTaskName(const std::string& name) { taskName = name; }
    void setTaskDescription(const std::string& description) { taskDescription = description; }
    void setUserId(const std::string& userId) { this->userId = userId; }
    //void setRunOnlyAfterUserLogon(bool runOnlyAfterUserLogon) { this->runOnlyAfterUserLogon = runOnlyAfterUserLogon; }
    void setRunWithHighestLevel(bool runWithHighestLevel) { this->runWithHighestLevel = runWithHighestLevel; }
    void setHidden(bool hidden) { this->hidden = hidden; }

    /// <summary>
    /// 创建计划任务服务对象
    /// </summary>
    TaskResult createService();

    /// <summary>
    /// 连接到计划任务服务
    /// </summary>
    TaskResult connectService();

    /// <summary>
    /// 创建任务定义
    /// </summary>
    TaskResult createTaskDefinition();

    /// <summary>
    /// 设置触发器（如：登录时触发）
    /// </summary>
    TaskResult setTrigger(const Trigger& trigger);

    /// <summary>
    /// 设置触发器（如：登录时触发）
    /// </summary>
    TaskResult setTriggers(const std::vector<Trigger>& triggers);

    /// <summary>
    /// 添加触发器（如：登录时触发）
    /// </summary>
    TaskResult addTriggers(const std::vector<Trigger>& triggers);

    /// <summary>
    /// 添加触发器（如：登录时触发）
    /// </summary>
    TaskResult addTrigger(const Trigger& trigger);

    /// <summary>
    /// 删除触发器
    /// </summary>
    TaskResult removeTrigger(size_t index);

    /// <summary>
    /// 添加动作（如：运行一个程序）
    /// </summary>
    TaskResult addAction(const Action& action);

    /// <summary>
    /// 添加动作（如：运行一个程序）
    /// </summary>
    TaskResult addActions(const std::vector<Action>& actions);
    
    /// <summary>
    /// 设置动作（如：运行一个程序）
    /// </summary>
    TaskResult setAction(const Action& action);

    /// <summary>
    /// 设置动作（如：运行一个程序）
    /// </summary>
    TaskResult setActions(const std::vector<Action>& actions);

    TaskResult removeAction(size_t index);

    /// <summary>
    /// 设置xml格式的任务定义，xml格式参考：https://docs.microsoft.com/en-us/windows/win32/taskschd/task-scheduler-schema
    /// </summary>
    /// <param name="xml">xml格式的任务定义</param>
    /// <returns>true if success, else false.</returns>
    TaskResult addXmlTask(const std::string& xml) {
        throw std::runtime_error("addXmlTask is not implemented yet");
    }

    TaskResult registerTask(const std::string& folderPath = "\\", bool overwriteExisting = false, bool createFolderIfNotExists = true);

    TaskResult getRegisteredTask(const std::string folderPath = "\\");

    TaskResult getRegisteredTaskDefinition();

    TaskResult runTask();

    [[deprecated("Use deleteTask instead")]]
    TaskResult unregisterTask(const std::string& folderPath = "\\");

    TaskResult deleteTask(const std::string& folderPath = "\\");

    /// <summary>
    /// 当前管理的任务是否已经注册到计划任务服务中
    /// 注：请先调用getTask方法获取任务，之后才可调用该函数检查是否已经注册，
    /// 如果任务不存在则调用registerTask方法注册任务，注册成功后再调用该方法检查任务是否注册成功
    /// </summary>
    /// <returns></returns>
    bool isTaskRegistered();

private:
    TaskResult addTriggerPrivate(const Trigger& trigger, void* pTriggerCollection);
    TaskResult addTriggersPrivate(const std::vector<Trigger>& triggers, void* pTriggerCollection);
    TaskResult addActionPrivate(const Action& action, void* pActionCollection);
    TaskResult addActionsPrivate(const std::vector<Action>& actions, void* pActionCollection);

    static std::string secondsToISO8601String(size_t seconds);
    static std::string dateTimeToISO8601String(DateTime duration);
    static std::string ensurePath(const std::string& path);
};

