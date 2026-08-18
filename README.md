# kiri Driver Installer

- Windows XP 及以上版本使用的 MFC 驱动安装程序。
- 自用于封装 Windows 系统时在部署过程中使用。
- 程序主要由 Codex 设计。

## 功能

- 使用 SetupAPI 枚举当前存在的即插即用设备。
- 按硬件 ID 精确查询 Audio、Chipset、Network、Video 索引。
- 分列显示驱动提供商、版本和日期，并默认勾选缺少驱动的设备。
- 仅解压匹配 INF 所在子目录，再调用 `UpdateDriverForPlugAndPlayDevicesW` 安装。

## 只读自检（仅 Codex 自动测试使用）

```powershell
.\kitsuneDrvInstaller.exe --self-test 'D:\Project\DrvInst\Win7\x64\Data' '.\self-test.json'
```
