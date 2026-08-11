# kitsune Driver Installer

Windows Vista 及以上版本使用的 MFC 驱动安装程序。
用于本人封装 Windows 系统时在部署过程中使用。
程序主要由 Codex 设计。

## 功能

- 使用 SetupAPI 枚举当前存在的即插即用设备。
- 按硬件 ID 精确查询 Audio、Chipset、Network、Video 索引。
- 分列显示驱动提供商、版本和日期，并默认勾选缺少驱动的设备。
- 仅解压匹配 INF 所在子目录，再调用 `UpdateDriverForPlugAndPlayDevicesW` 安装。
- 自动请求管理员权限，报告单项安装结果和重启要求。

## 只读自检（仅Codex使用）

```powershell
.\kitsuneDrvInstaller.exe --self-test 'D:\Project\DrvInst\Win7\x64\Data' '.\self-test.json'
```

自检仅加载索引、扫描设备并定位 7-Zip，不执行驱动安装。
