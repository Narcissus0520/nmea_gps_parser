# NMEA GPS Parser

GNSS Cyberpunk Host is a C++ Qt desktop upper-computer tool for parsing NMEA-0183 GNSS data.

## Version

Current version: `v1.1.2`

## Features

- Serial NMEA acquisition with default `115200 8N1`
- ASCII and HEX command sending
- GGA/RMC/GSA/GSV/VTG parsing
- Offline NMEA replay
- Simulated NMEA data mode
- GNSS dashboard, sky plot, per-constellation TOP7 CN0 chart, raw NMEA log
- Satellite constellation filtering for sky plot, CN0 chart, and analysis
- Offline map track view using `tiles/{z}/{x}/{y}.png`
- In-app map tile loading help
- Truth CSV import and precision analysis
- HTML and CSV report export
- Portable Windows release package script

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

## Preview

Before building the Qt app, open:

```text
preview/gnss_preview.html
```

## Notes

Build outputs and local map tiles are intentionally excluded from Git.

## Version History

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
