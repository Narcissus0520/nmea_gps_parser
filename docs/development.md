# GNSS Cyberpunk Host 开发文档

## 1. 项目目标

GNSS Cyberpunk Host 是一个 C++ Qt 桌面上位机，用于实时串口采集 0813-NMEA 数据、离线 NMEA 回放、离线地图轨迹显示、COM 命令下发、标准轨迹精度分析和工程报告导出。

项目优先使用 Qt 6，兼容 Qt 5.15 的常见 API。编译器使用 GCC，构建时建议开启 `-Wall -Wextra`。

## 2. 功能范围

- 串口连接：默认 `115200 8N1`，支持端口刷新、连接、断开、接收原始 NMEA。
- NMEA 解析：支持 `GGA/RMC/GSA/GSV/VTG`，解析定位、速度、海拔、卫星数、DOP、CN0、卫星方位角和仰角。
- 仪表盘：定位信息、搜星数、速度、海拔、CN0 TOP7、卫星分布图分区显示。
- NMEA 文件：支持导出接收到的原始 NMEA，支持导入离线 NMEA 并按回放速度播放。
- 离线地图：使用 `tiles/{z}/{x}/{y}.png` 本地 Web Mercator 瓦片，显示当前位置和轨迹。
- COM 命令：支持 ASCII 文本模式和 HEX 模式。文本模式默认追加 CRLF；HEX 模式不自动追加结束符。
- 精度分析：导入标准轨迹 CSV，格式为 `timestamp,lat,lon,alt`，按时间匹配 GNSS 历元并计算误差。
- 工程报告：导出 HTML 摘要报告和 CSV 明细。

## 3. 数据流

```text
SerialPort / ReplayFile
        |
        v
Raw NMEA line buffer -----> Export NMEA
        |
        v
NmeaParser
        |
        v
GnssState
   |       |        |        |
   v       v        v        v
Dashboards Map   Analysis  Report
```

实时串口和离线回放共用 `NmeaParser` 与 `GnssState`，确保两种模式的显示效果一致。

## 4. 模块划分

- `src/app/`：应用入口、主窗口、界面组织和信号连接。
- `src/serial/`：串口端口扫描、连接控制、接收数据、文本/HEX 命令编码。
- `src/nmea/`：NMEA 校验、字段拆分、GGA/RMC/GSA/GSV/VTG 解析和状态聚合。
- `src/replay/`：离线文件载入、暂停、继续、停止、进度、倍率控制。
- `src/analysis/`：标准轨迹导入、误差统计、定位情况分析。
- `src/map/`：离线瓦片地图、当前位置、移动轨迹绘制。
- `src/report/`：HTML 与 CSV 报告导出。
- `src/widgets/`：赛博朋克风仪表盘、自绘柱状图、卫星分布图。
- `resources/`：QSS 主题。
- `qml/`：预留 Qt Quick 仪表资源目录，后续可替换自绘 Widgets。

## 5. COM 命令下发规则

文本模式：

- 输入按 UTF-8/ASCII 字节发送。
- 默认追加 CRLF。
- 可切换 CR、LF、CRLF、无结束符。

HEX 模式：

- 支持空格分隔格式，例如 `AA 55 01 0D 0A`。
- 支持连续格式，例如 `AA55010D0A`。
- 忽略空白字符。
- 必须为偶数个十六进制字符。
- 遇到非法字符或奇数长度时提示错误，不写串口。
- 不自动追加 CR/LF/CRLF。

## 6. 精度分析

标准轨迹 CSV 字段：

```csv
timestamp,lat,lon,alt
2026-06-03T10:00:00Z,31.230416,121.473701,12.3
```

分析输出：

- 定位率
- 2D/3D 状态统计
- 平均卫星数
- 平均 HDOP/PDOP
- 平均 CN0
- 水平误差 RMS/MAX/CEP50/CEP95
- 高程误差 RMS/MAX

## 7. 模拟数据模式

为了在未连接 GNSS 设备时演示界面动态效果，工程提供内置模拟 NMEA 数据源：

- 模拟器使用 `QTimer` 每秒输出一组 NMEA 语句。
- 输出语句包含 `GGA/RMC/GSA/GSV/VTG`，并带标准 NMEA 校验和。
- 模拟位置沿固定小范围轨迹移动，速度、航向、海拔、卫星 CN0 会随时间变化。
- 模拟数据通过和串口、离线回放相同的 `process_nmea_line` 数据入口进入解析器，确保仪表盘、天空图、CN0 柱状图、轨迹和分析模块的刷新路径一致。
- UI 在底部工具区提供 `Start Sim` / `Stop Sim` 按钮用于启动或停止模拟移动。

## 8. 构建方式

若本机已安装 Qt：

```sh
qmake GnssCyberpunkHost.pro
mingw32-make
```

或使用 Qt Creator 打开 `GnssCyberpunkHost.pro`。

当前机器 PATH 下未发现 `qmake/cmake` 时，工程文件仍可作为 Qt Creator 工程骨架使用。

## 9. 实现注意点

- 地图坐标转换使用 Web Mercator 公式，常量使用代码内显式定义，避免依赖非标准宏。
- HEX 命令校验按 Unicode 字符逐个判断，只接受 `0-9/a-f/A-F` 和空白字符。
- Qt 自绘控件使用 `paintEvent`，避免依赖额外图表库。
- Qt 6.11 下时间戳使用 `QTimeZone::UTC` 构造 `QDateTime`，避免使用已废弃的 `Qt::UTC` 构造函数。
- MSYS2 UCRT64 的 GCC 16 会在 Qt 6.11 头文件中触发 `-Wsfinae-incomplete` 第三方警告，工程文件中显式关闭该警告，保留 `-Wall -Wextra` 用于项目代码检查。
- 模拟器中的航向角归一化使用标准库 `std::fmod`，避免依赖 Qt 版本差异较大的数学封装函数。

## 10. 效果预览

在 Qt 环境尚未完成安装时，可打开 `preview/gnss_preview.html` 查看静态界面效果。该文件仅用于视觉确认，不参与最终 Qt 程序构建。

当前预览风格参考专业 NMEA/GNSS 调试仪器界面：深色扫描线背景、荧光绿主色、青色边框、紧凑面板、状态栏、串口配置栏、仪表盘、增强卫星天空图、原始 NMEA 报文区和底部记录/回放/导出工具条。

正式 Qt 界面按同一方向重构：

- 顶部状态栏显示产品名、协议说明、版本号和连接状态。
- 串口配置栏横向排列端口、波特率、数据位、停止位、校验、连接和刷新。
- 主工作区采用三列布局：左侧仪表盘与位置，中间卫星天空图和 TOP7 CN0，右侧原始 NMEA 报文。
- 底部工具条集中放置 COM 命令、回放、导出、真值导入、报告和清空功能。
- 自绘星座图使用多层仰角圆环、方位辅助线、N/E/S/W 标记、按星座/CN0 着色的卫星点和可见星统计。

离线地图正式实现要求：

- UI 提供 `Tiles...` 按钮选择本地瓦片根目录。
- 瓦片目录格式固定为 `{root}/{z}/{x}/{y}.png`。
- 地图控件支持 `Zoom +` / `Zoom -` 和鼠标滚轮缩放。
- 无瓦片时显示深色坐标网格、当前缩放级别、瓦片目录和提示文本，避免用户误以为地图未工作。
- 有定位点后始终绘制当前位置和历史轨迹，即使瓦片缺失也保留轨迹显示。
- 地图状态栏显示当前缩放级别、轨迹点数和瓦片目录。
- 当前 release 包预置一小份深圳中心演示瓦片，默认路径为 `release/tiles/{z}/{x}/{y}.png`，用于离线地图功能验证。
- 预置演示瓦片来源为 OpenStreetMap 标准瓦片，地图控件显示 `Tiles (C) OpenStreetMap contributors` attribution。该预置包仅用于小范围功能演示，不作为批量离线地图下载方案。

## 11. 仓库提交说明

推送到 Git 仓库时只提交源码、开发文档、静态预览、QSS 资源和工程文件。

不提交以下构建产物：

- `debug/`
- `release/`
- `Makefile`
- `Makefile.Debug`
- `Makefile.Release`
- `.qmake.stash`
- 本地离线地图瓦片目录 `tiles/`

如果需要发布可直接运行的程序，应单独打包 release 目录，不放入源码仓库。

## 12. Windows 便携发布包

MSYS2/UCRT64 环境下，`windeployqt` 会部署 Qt DLL 和插件，但不会完整复制 GCC/UCRT64 运行库以及 Qt 依赖的第三方 DLL。因此直接把 `release/GnssCyberpunkHost.exe` 拷到其他 Windows 机器可能无法运行。

发布时应使用 `tools/package_release.ps1` 生成便携目录：

```powershell
.\tools\package_release.ps1
```

脚本会执行：

- 重新运行 `qmake` 和 `make release`
- 创建 `dist/GnssCyberpunkHost`
- 拷贝 release EXE
- 运行 `windeployqt`
- 递归扫描 EXE/DLL 的 `DLL Name` 依赖
- 从 Qt/MSYS2 UCRT64 `bin` 目录复制缺失的运行库和第三方 DLL
- 拷贝预置深圳演示瓦片 `release/tiles`
- 生成 zip 包 `dist/GnssCyberpunkHost-portable.zip`

用户在其他 Windows 机器上应解压整个 `GnssCyberpunkHost` 目录后运行其中的 EXE，不要只复制单个 EXE 文件。
