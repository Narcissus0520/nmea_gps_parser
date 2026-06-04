# NMEA GPS Parser

GNSS Cyberpunk Host is a C++ Qt desktop upper-computer tool for parsing NMEA-0183 GNSS data.

## Features

- Serial NMEA acquisition with default `115200 8N1`
- ASCII and HEX command sending
- GGA/RMC/GSA/GSV/VTG parsing
- Offline NMEA replay
- Simulated NMEA data mode
- GNSS dashboard, sky plot, TOP7 CN0 chart, raw NMEA log
- Offline map track view using `tiles/{z}/{x}/{y}.png`
- Truth CSV import and precision analysis
- HTML and CSV report export

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
