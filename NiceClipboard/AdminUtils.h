#pragma once
#include <vector>
#include <string>

bool isRunningAsAdmin();

/// <summary>
/// 重新启动当前程序，并以管理员权限运行。
/// 该函数会发起一个提权请求，用户同意后会启动一个新的进程
/// 若不指定新的参数列表和工作目录，新的进程会继承当前进程的命令行参数和工作目录。
/// </summary>
/// <param name="args">新的参数列表</param>
/// <param name="workingDirectory">新的工作目录</param>
/// <returns></returns>
bool relaunchAsAdmin(const std::vector<std::string>& args = {}, const std::string& workingDirectory = "");

