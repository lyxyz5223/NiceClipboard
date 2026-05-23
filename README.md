# Nice Clipboard

<a href="https://github.com/lyxyz5223/NiceClipboard">
  <img src="https://img.shields.io/badge/platform-Windows%20x64-blue" alt="Platform">
  <img src="https://img.shields.io/badge/Qt-6.7.2-green" alt="Qt">
  <img src="https://img.shields.io/badge/VS-2022%20(v143)-purple" alt="Visual Studio">
</a>

**Nice Clipboard** 是一款 Windows 平台上的剪贴板增强工具，提供剪贴板历史记录管理、全局快捷键、数据持久化等功能，帮助用户高效管理剪贴板内容。

## ✨ 功能特性

- **剪贴板历史** — 自动记录复制的内容（文本、图片等），支持浏览、搜索和管理历史记录。
- **系统托盘** — 最小化至系统托盘，后台静默运行，随时唤醒。
- **全局快捷键** — 注册全局热键，快速打开主窗口或执行特定操作。
- **数据持久化** — 剪贴板历史自动保存到磁盘，重启后恢复。
- **排序与搜索** — 支持对历史记录排序和关键词搜索。
- **管理员权限** — 支持以管理员权限重启程序以执行计划任务等特权操作。
- **自定义设置** — 丰富的配置项，提供图形化设置界面。
- **SVG 图标** — 使用 SVG 矢量图标，清晰适配不同分辨率。
- **动画菜单** — 流畅的动画效果，提升交互体验。

## 🖥️ 系统要求

| 要求       | 说明                    |
| ---------- | ----------------------- |
| 操作系统   | Windows 10 / 11 (x64)   |
| 运行时     | Visual C++ Redistributable |
| 开发编译   | Visual Studio 2022      |
| Qt 版本    | 6.7.2                   |

## 🔧 构建方法

### 前置条件

1. 安装 [Visual Studio 2022](https://visualstudio.microsoft.com/)（选择"使用 C++ 的桌面开发"工作负载）。
2. 安装 [Qt 6.7.2](https://www.qt.io/download) 或更高版本，确保包含以下模块：
   - `core`、`gui`、`network`、`widgets`、`svg`
3. 配置 Qt MSBuild 集成（Qt VS Tools 或手动配置 `QtMsBuild`）。

### 构建步骤

```bash
# 克隆仓库
git clone https://github.com/lyxyz5223/NiceClipboard.git
cd NiceClipboard

# 使用 Visual Studio 打开解决方案
# 或命令行构建：
msbuild NiceClipboard.sln /p:Configuration=Release /p:Platform=x64
```

构建产物位于 `NiceClipboard\x64\Release\NiceClipboard.exe`。

## 🚀 使用说明

1. 运行 `NiceClipboard.exe`，程序将出现在系统托盘中。
2. 使用全局热键（默认配置）或双击托盘图标打开主窗口。
3. 主窗口显示剪贴板历史列表，支持左键/右键点击、双击等操作。
4. 可通过设置界面自定义快捷键、外观和行为。

## 📁 项目结构

```
NiceClipboard/
├── NiceClipboard.sln              # 解决方案文件
├── NiceClipboard/                 # 项目源码
│   ├── main.cpp                   # 入口，异常处理
│   ├── NiceClipboard.h/cpp        # 主窗口类
│   ├── ClipboardListView.h/cpp    # 剪贴板历史列表视图
│   ├── GlobalConfig.h/cpp         # 全局配置定义
│   ├── GlobalConfigManager.h/cpp  # 配置管理器
│   ├── SettingsWidget.h/cpp       # 设置界面
│   ├── SettingsUIFactory.h/cpp    # 设置 UI 工厂
│   ├── AnimatedMenu.h/cpp         # 动画菜单
│   ├── AnimatedMenuAction.h/cpp   # 动画菜单动作
│   ├── HotKeyEdit.h/cpp           # 热键编辑控件
│   ├── Win32HotKey.h/cpp          # Win32 全局热键
│   ├── ScheduledTask.h/cpp        # 计划任务（开机自启）
│   ├── GlobalScheduledTaskManager.h/cpp
│   ├── StringProcess.h/cpp        # 字符串编码转换
│   ├── AdminUtils.h/cpp           # 管理员权限工具
│   ├── DoubleClickController.h/cpp# 双击事件控制器
│   ├── RoundedIconButton.h/cpp    # 圆角图标按钮
│   ├── ColoredSvgIcon.h/cpp       # 彩色 SVG 图标
│   ├── TranslucentScreenMask.h/cpp# 半透明屏幕遮罩
│   ├── CheckBoxWithUserData.h/cpp # 带用户数据的复选框
│   └── svgs/                      # SVG 图标资源
```

## 📄 许可证

本项目基于 MIT 许可证开源 — 参见 [LICENSE](LICENSE) 文件。
