#include "AdminUtils.h"
#include <QGuiApplication>
#include <QDir>
//#include <QProcess>
#include <windows.h>
#include <shellapi.h>

bool isRunningAsAdmin()
{
    BOOL bIsAdmin = FALSE;
    PSID pAdminSid = nullptr;

    // 构造一个表示Administrators组的SID
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, // 内置域
        DOMAIN_ALIAS_RID_ADMINS, // 管理员组的RID
        0, 0, 0, 0, 0, 0, &pAdminSid))
    {
        // 检查当前进程的令牌是否属于该组，第一个参数传入NULL表示使用当前线程的令牌
        if (!CheckTokenMembership(NULL, pAdminSid, &bIsAdmin))
        {
            // 调用失败，安全起见，视为非管理员
            bIsAdmin = FALSE;
        }
        // 释放由AllocateAndInitializeSid分配的内存
        FreeSid(pAdminSid);
    }
    return (bIsAdmin != FALSE);
}


bool relaunchAsAdmin(const std::vector<std::string>& args, const std::string& workingDirectory)
{
    // 获取当前程序的完整路径
    QString appPath = QGuiApplication::applicationFilePath();
    std::string appPathStr = appPath.toStdString();
    std::vector<std::string> arguments = args;
    if (arguments.empty())
    {
        QStringList qargs = QGuiApplication::arguments();
        for (const auto& arg : qargs)
            arguments.push_back(arg.toStdString());
    }
    QString workingDir = QDir::currentPath();
    std::string workingDirStr = workingDirectory;
    if (workingDirStr.empty())
        workingDirStr = workingDir.toStdString();
    // 配置ShellExecuteEx的参数
    SHELLEXECUTEINFOA sei = { sizeof(sei) };
    sei.fMask = 0; // 不使用特殊标志
    sei.lpVerb = "runas"; // 使用runas请求UAC提权
    sei.lpFile = appPathStr.c_str(); // 指定要运行的程序路径
    sei.lpDirectory = workingDirStr.c_str(); // 指定工作目录
    std::string argsText;
    for (const auto& arg : arguments)
        argsText += arg + " ";
    if (argsText.size())
        argsText.pop_back(); // 去掉最后一个空格
    sei.lpParameters = argsText.c_str(); // 指定命令行参数
    sei.nShow = SW_NORMAL;
    return ShellExecuteExA(&sei); // 执行提权后的新进程
}