#include "NiceClipboard.h"
#include <QtWidgets/QApplication>
#include <QSystemTrayIcon>

#include <windows.h>
#include <dbghelp.h>
#include <tchar.h>
#include <iostream>
#include <string>
#include <crtdbg.h>

#pragma comment(lib, "dbghelp.lib")


class MemoryErrorHandler
{
public:
    
    static void Init()
    {
        // 设置未处理异常过滤器
        SetUnhandledExceptionFilter(UnhandledExceptionHandler);
        // 启用CRT内存泄露检测（Debug模式下）
#ifdef _DEBUG
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
        // 设置无效参数处理
        _set_invalid_parameter_handler(InvalidParameterHandler);
        // 设置纯虚函数调用处理
        _set_purecall_handler(PureVirtualCallHandler);
    }

private:
    static LONG WINAPI UnhandledExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo)
    {
        std::wstring errorMsg = FormatExceptionInfo(pExceptionInfo);

        MessageBoxW(NULL, errorMsg.c_str(), L"程序崩溃",
            MB_ICONERROR | MB_OK | MB_SYSTEMMODAL);

        return EXCEPTION_EXECUTE_HANDLER;
    }

    static void InvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t reserved)
    {
        std::wstring msg = L"无效参数错误！\n函数: " + std::wstring(function) +
            L"\n文件: " + std::wstring(file) +
            L"\n行号: " + std::to_wstring(line);

        MessageBoxW(NULL, msg.c_str(), L"运行时错误",
            MB_ICONERROR | MB_OK);
    }

    static void PureVirtualCallHandler()
    {
        MessageBox(NULL,
            _T("检测到纯虚函数调用！\n程序将终止。"),
            _T("严重错误"),
            MB_ICONERROR | MB_OK);
    }

    static std::wstring FormatExceptionInfo(PEXCEPTION_POINTERS pExceptionInfo)
    {
        DWORD exceptionCode = pExceptionInfo->ExceptionRecord->ExceptionCode;
        std::wstring errorType;

        switch (exceptionCode)
        {
        case EXCEPTION_ACCESS_VIOLATION:
            errorType = L"内存访问违规 (Access Violation)";
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            errorType = L"数组越界";
            break;
        case EXCEPTION_STACK_OVERFLOW:
            errorType = L"栈溢出";
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            errorType = L"整数除零";
            break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            errorType = L"浮点数除零";
            break;
        default:
            errorType = L"未知异常";
        }

        wchar_t buffer[512];
        swprintf_s(buffer,
            L"程序发生严重错误！\n\n"
            L"错误类型：%s\n"
            L"错误代码：0x%08X\n\n"
            L"建议：\n"
            L"1. 保存您的工作\n"
            L"2. 重启应用程序\n"
            L"3. 如果问题持续，请联系技术支持",
            errorType.c_str(), exceptionCode);

        return buffer;
    }
};

struct MemoryErrorHandlerInstaller {
    MemoryErrorHandlerInstaller() {
        MemoryErrorHandler::Init();
    }
};

void myMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    QString fullMsg = qFormatLogMessage(type, context, msg);
    QByteArray localMsg = msg.toUtf8();  // 转成utf8再输出
    fprintf(stderr, "%s\n", localMsg.constData());
}

int main(int argc, char *argv[])
{
    SetConsoleOutputCP(65001); // 设置控制台输出为UTF-8编码
    auto sink = std::make_shared<ConsoleLoggerSink>();
    Logger logger{ "NiceClipboard", { sink } };
    logger.info("Application starting...");
    //HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    //if (FAILED(hr))
    //    logger.error("Failed to initialize COM library: 0x{:08X}", hr);
    qInstallMessageHandler(myMessageHandler);
    QCoreApplication::setOrganizationDomain("lyxyz5223.com");
    QCoreApplication::setOrganizationName("lyxyz5223");
    QCoreApplication::setApplicationName("Nice Clipboard");
    MemoryErrorHandler::Init();
    QApplication app(argc, argv);
    NiceClipboard window;
    window.show();
    auto retCode = app.exec();
    return retCode;
}

