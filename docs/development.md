# GNSS Cyberpunk Host 开发文档

## 1. 项目目标

GNSS Cyberpunk Host 是一个 C++ Qt 桌面上位机，用于实时串口采集 0813-NMEA 数据、离线 NMEA 回放、离线地图轨迹显示、COM 命令下发、标准轨迹精度分析和工程报告导出。

项目优先使用 Qt 6，兼容 Qt 5.15 的常见 API。编译器使用 GCC，构建时建议开启 `-Wall -Wextra`。

## 2. 功能范围

- 串口连接：默认 `115200 8N1`，支持端口刷新、连接、断开、接收原始 NMEA；界面选择的数据位、校验位和停止位必须真实传入串口驱动层。
- NMEA 解析：支持 `GGA/RMC/GSA/GSV/VTG`，解析定位、速度、海拔、卫星数、DOP、CN0、卫星方位角和仰角。
- 仪表盘：定位信息、搜星数、速度、海拔、CN0 TOP7、卫星分布图分区显示。
- CN0 柱状图：必须按星座/模式独立分组显示，每一模各取本模 CN0 最高的 TOP7；不同星座不可混排成一个总 TOP7。
- 卫星筛选：天空图提供 GPS/BDS/GLO/GAL/QZSS/GNSS 星座过滤开关，关闭的星座不参与天空图、CN0 TOP7 和 CN0 统计分析。
- NMEA 文件：支持导出接收到的原始 NMEA，支持导入离线 NMEA 并按回放速度播放；回放进度条支持拖动定位到指定语句并刷新显示。
- 离线地图：使用 `tiles/{z}/{x}/{y}.png` 本地 Web Mercator 瓦片，显示当前位置和轨迹。
- COM 命令：支持 ASCII 文本模式和 HEX 模式。文本模式默认追加 CRLF；HEX 模式不自动追加结束符。
- 自动化指令测试：支持加载 CSV 用例，按顺序通过已连接串口下发 ASCII/HEX 指令，等待响应并匹配期望文本，输出 PASS/FAIL 结果。
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

自动化指令测试：

- 测试用例 CSV 字段固定为 `name,mode,ending,command,expect,timeout_ms`。
- `mode` 支持 `text` 和 `hex`；`ending` 支持 `crlf/cr/lf/none`，仅文本模式生效。
- `command` 为待下发内容；HEX 命令继续复用手动下发的 HEX 校验规则，非法字符或奇数长度输入不得写串口。
- `expect` 为期望响应子串；为空时只验证命令成功写入串口。
- `timeout_ms` 默认 1000，允许范围 50 到 60000。
- 测试运行期间按 CSV 顺序串行执行，每条用例完成后进入下一条；结果区记录 PASS/FAIL、用例名称和失败原因。
- 自动化测试仅使用本地串口，不应自动联网、上传测试结果或读取 CSV 以外的外部资源。

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
- 模拟 GSV 覆盖 GPS、BDS、GLO、GAL，用于验证每一模 TOP7 CN0 柱状图和星座图分组显示。
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

## 9. 自动化测试

工程提供 C++ 单元测试可执行程序，覆盖 NMEA 解析和 COM 命令编码：

- NMEA 校验成功与失败。
- GGA/RMC/GSA/GSV/VTG 基础字段解析。
- 多星座 GSV 解析与卫星字段聚合。
- 文本命令 CR/LF/CRLF/无结束符编码。
- HEX 命令空格格式、连续格式、非法字符和奇数长度校验。

测试源码位于 `tests/unit_tests/`，运行方式：

```powershell
.\tools\run_unit_tests.ps1
```

## 10. 实现注意点

- 地图坐标转换使用 Web Mercator 公式，常量使用代码内显式定义，避免依赖非标准宏。
- HEX 命令校验按 Unicode 字符逐个判断，只接受 `0-9/a-f/A-F` 和空白字符。
- Qt 自绘控件使用 `paintEvent`，避免依赖额外图表库。
- Qt 6.11 下时间戳使用 `QTimeZone::UTC` 构造 `QDateTime`，避免使用已废弃的 `Qt::UTC` 构造函数。
- MSYS2 UCRT64 的 GCC 16 会在 Qt 6.11 头文件中触发 `-Wsfinae-incomplete` 第三方警告，工程文件中显式关闭该警告，保留 `-Wall -Wextra` 用于项目代码检查。
- 模拟器中的航向角归一化使用标准库 `std::fmod`，避免依赖 Qt 版本差异较大的数学封装函数。
- 开源安全基线：程序不应内置 API key、token、私钥、账号口令、个人路径或自动联网/上传逻辑。
- 串口、回放文件、真值 CSV 和原始 NMEA 缓冲必须设置大小上限，避免异常输入导致内存无限增长。
- 打包脚本只允许安全包名，不接受包含路径分隔符或上级目录跳转的包名。
- 用户主动导出的原始 NMEA 可能包含定位轨迹，导出动作必须由用户通过界面触发，不自动上传。

## 11. 效果预览

在 Qt 环境尚未完成安装时，可打开 `preview/gnss_preview.html` 查看静态界面效果。该文件仅用于视觉确认，不参与最终 Qt 程序构建。

当前预览风格参考专业 NMEA/GNSS 调试仪器界面：深色扫描线背景、荧光绿主色、青色边框、紧凑面板、状态栏、串口配置栏、仪表盘、增强卫星天空图、原始 NMEA 报文区和底部记录/回放/导出工具条。

正式 Qt 界面按同一方向重构：

- 顶部状态栏显示产品名、协议说明、版本号和连接状态。
- 串口配置栏横向排列端口、波特率、数据位、停止位、校验、连接和刷新。
- 主工作区采用三列布局：左侧仪表盘与位置，中间卫星天空图和分星座 TOP7 CN0，右侧原始 NMEA 报文。
- 左侧仪表盘采用紧凑尺寸，避免占用地图空间；位置信息区中的离线地图应获得更高纵向权重。
- 启动模拟、接收串口数据或回放数据时，窗口尺寸不应因文本、统计值或动态图表刷新发生跳变。
- 底部工具条集中放置 COM 命令、回放、导出、真值导入、报告和清空功能。
- 自绘星座图使用多层仰角圆环、方位辅助线、N/E/S/W 标记、按星座/CN0 着色的卫星点和可见星统计。
- 天空图顶部提供星座过滤开关，用户可取消不想分析的星座；过滤结果同步刷新天空图、分星座 TOP7 CN0 和分析摘要。
- 天空图在中间面板中应比 CN0 柱状图占用更多高度；卫星点保持小尺寸，避免密集星况下互相遮挡。
- 用户点击天空图中的卫星点时，应显示该星的星座、PRN、俯仰角、方位角和 CN0。
- 分星座 TOP7 CN0 小图在紧凑布局下仍必须保留可见柱状图，不能因控件高度较小只显示标题或空面板。

离线地图正式实现要求：

- UI 提供 `Tiles...` 按钮选择本地瓦片根目录。
- UI 提供 `Help` 按钮弹出地图瓦片说明，指导用户准备、下载和加载新的离线瓦片。
- 瓦片目录格式固定为 `{root}/{z}/{x}/{y}.png`。
- 加载新瓦片时，用户点击 `Tiles...` 后应选择包含缩放级别目录的根目录，例如 `D:/maps/shenzhen_tiles`，其下存在 `16/52364/34012.png` 这类文件。
- 工具不内置批量抓取公网瓦片功能，避免违反地图服务条款；用户应从授权地图源或自建瓦片服务导出 PNG XYZ 瓦片后再加载。
- 地图控件支持 `Zoom +` / `Zoom -` 和鼠标滚轮缩放。
- 无瓦片时显示深色坐标网格、当前缩放级别、瓦片目录和提示文本，避免用户误以为地图未工作。
- 尚未收到定位点但默认瓦片可用时，地图以深圳中心演示坐标作为预览中心绘制瓦片，并叠加等待定位提示；收到真实定位点后切换到实际位置和轨迹。
- 有定位点后始终绘制当前位置和历史轨迹，即使瓦片缺失也保留轨迹显示。
- 地图状态栏显示当前缩放级别、轨迹点数和瓦片目录。
- 当前 release 包预置一小份深圳中心演示瓦片，默认路径为 `release/tiles/{z}/{x}/{y}.png`，用于离线地图功能验证。
- 预置演示瓦片来源为 OpenStreetMap 标准瓦片，地图控件显示 `Tiles (C) OpenStreetMap contributors` attribution。该预置包仅用于小范围功能演示，不作为批量离线地图下载方案。
- 地图控件启动时需要自动探测常见瓦片目录，按优先级尝试 EXE 同级 `tiles/`、当前工作目录 `tiles/`、当前工作目录 `release/tiles/`、项目发布目录 `release/tiles/` 和便携包目录 `dist/GnssCyberpunkHost/tiles/`，以兼容 Qt Creator Debug、Release EXE 和打包目录三种运行方式。
- 如果当前 zoom/位置没有匹配瓦片，地图 fallback 提示必须显示当前瓦片根目录和中心瓦片期望路径，便于用户判断是目录选错、zoom 不匹配还是瓦片缺失。

## 12. 仓库提交说明

推送到 Git 仓库时只提交源码、开发文档、静态预览、QSS 资源和工程文件。

每次提交代码前必须同步更新：

- README.md 中的当前版本号。
- README.md 中对应版本的上库修改点说明。
- 程序界面显示的版本号，保持与 README.md 当前版本一致。

不提交以下构建产物：

- `debug/`
- `release/`
- `Makefile`
- `Makefile.Debug`
- `Makefile.Release`
- `.qmake.stash`
- 本地离线地图瓦片目录 `tiles/`

如果需要发布可直接运行的程序，应单独打包 release 目录，不放入源码仓库。

## 13. Windows 便携发布包

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
- 从多个候选目录拷贝预置深圳演示瓦片，优先使用 `release/tiles`，其次使用项目根目录 `tiles`
- 生成 zip 包 `dist/GnssCyberpunkHost-portable.zip`

用户在其他 Windows 机器上应解压整个 `GnssCyberpunkHost` 目录后运行其中的 EXE，不要只复制单个 EXE 文件。
