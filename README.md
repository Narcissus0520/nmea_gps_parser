# NMEA GPS Parser

GNSS Cyberpunk Host is a C++ Qt desktop upper-computer tool for parsing NMEA-0183 GNSS data.

## Version

Current version: `v1.3.0`

## Features

- Serial NMEA acquisition with default `115200 8N1`
- ASCII and HEX command sending
- Automated serial command testing from CSV cases
- Real serial data bits, parity, and stop bits configuration
- GGA/RMC/GSA/GSV/VTG parsing
- Offline NMEA replay
- Draggable offline replay progress seeking
- Stable replay seeking with state rebuild
- Simulated NMEA data mode
- GNSS dashboard, sky plot, per-constellation TOP7 CN0 chart, raw NMEA log
- Satellite constellation, CN0, azimuth, and elevation filtering for sky plot, CN0 chart, and analysis
- Offline map track view using `tiles/{z}/{x}/{y}.png`
- In-app map tile loading help
- Truth CSV import and precision analysis
- Batch NMEA analysis report import with per-second trend charts
- NMEA to KML and KML to NMEA conversion
- In-app quick start and feature guide
- HTML and CSV report export
- Portable Windows release package script
- C++ unit test coverage for NMEA parsing and command encoding

## Build

The project uses Qt qmake.

```sh
qmake GnssCyberpunkHost.pro
make release
```

On the current MSYS2 UCRT64 environment:

```sh
qmake GnssCyberpunkHost.pro
C:\msys64\usr\bin\make.exe release
windeployqt release\GnssCyberpunkHost.exe
```

## Tests

```powershell
.\tools\run_unit_tests.ps1
```

## Preview

Before building the Qt app, open:

```text
preview/gnss_preview.html
```

## Notes

Build outputs and local map tiles are intentionally excluded from Git.

## Version History

### v1.3.0 - 2026-06-06

上库修改点：

- Fixed replay progress dragging by seeking on release and rebuilding parser/UI state to the target epoch.
- Fixed replay CN0 refresh by preserving parsed GSV satellite state during replay seek/rebuild.
- Added sky plot filters for CN0, azimuth, and elevation ranges, and hid CN0=0 satellites from real-time sky display.
- Expanded fix quality display to include 2D/3D/VDR/RTK style categories.
- Added batch NMEA analysis import and richer HTML report sections for per-second satellite, DOP, CN0, speed, and course trends.
- Added NMEA/KML conversion actions.
- Added an in-app quick start guide so users can understand the main workflows after opening the tool.

### v1.2.1 - 2026-06-06

上库修改点：

- Fixed offline map tile discovery when the app is launched from `debug`, `release`, or the portable `dist` directory.
- Added clearer map fallback text with the active tile root and expected center tile path.
- Hardened the release packaging script so map tiles can be copied from multiple local candidate directories.
- Added automated command test CSV loading, sequential execution, response matching, and PASS/FAIL logging.

### v1.2.0 - 2026-06-06

上库修改点：

- Made serial data bits, parity, and stop bits UI settings take effect in `QSerialPort`.
- Replaced replay progress display with a draggable slider that can seek to a replay epoch.
- Added C++ unit tests for NMEA parsing and ASCII/HEX command encoding.
- Added test build instructions to the development document.

### v1.1.2 - 2026-06-05

上库修改点：

- Completed an open-source security scan for secrets, privacy leakage, network access, and command execution risks.
- Added size and line-length limits for replay NMEA files and truth CSV files.
- Added serial receive buffer and raw NMEA buffer caps to reduce memory exhaustion risk.
- Added epoch retention limits for long-running sessions.
- Hardened package script arguments to reject path-like package names.
- Documented the security baseline for offline-only operation and release packaging.

### v1.1.1 - 2026-06-05

上库修改点：

- Enlarged the sky plot area relative to the TOP7 CN0 chart.
- Reduced satellite marker size in the sky plot for dense satellite views.
- Added click interaction on sky plot satellites to show elevation, azimuth, and CN0 details.

### v1.1.0 - 2026-06-05

上库修改点：

- Added per-constellation TOP7 CN0 charts instead of one mixed global TOP7 list.
- Added GPS/BDS/GLO/GAL simulated GSV data for multi-constellation display validation.
- Added sky plot constellation filters for GPS/BDS/GLO/GAL/QZSS/GNSS.
- Synced satellite filters to sky plot, TOP7 CN0 display, and analysis summary.
- Fixed TOP7 CN0 bars disappearing in compact layouts.
- Reduced dashboard footprint, enlarged the map area, and stabilized layout size while simulation data updates.
- Added offline map tile help inside the tool.
- Added portable Windows packaging script and DLL dependency collection notes.

提交要求：

- Every code commit must update `Current version`.
- Every code commit must add or update the matching `Version History` entry.
- The in-app version label must stay consistent with this README.
