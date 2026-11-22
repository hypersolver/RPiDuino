# XDOS and RPiDuino compatibility / 相容性

| Build target / 目標 | Paired runtime / 配對執行環境 | Status / 狀態 |
| --- | --- | --- |
| Standalone IMG | Circle at the pinned revision / 固定版 Circle | Arduino IDE build, Debug UART upload and ACT LED verified on Pi 3B+ with v0.1.4 / Pi 3B+ 已驗證 |
| XDOS XAPP in SDK 0.1.4 | Pi 3B+ FILEQ128, bundled host manifest and headers / 內附主系統符號清單與標頭 | Released / 已發布 |
| Arduino V2 XAPP | Pi 3B+ MC4FQ, matching V2 entry/packaging tools and host ABI inputs / 配對的 V2 啟動、打包工具及 ABI 資料 | Prototype entry point only. SDK integration and hardware acceptance are pending / 啟動器試作，待整合與實機驗收 |

XDOS owns the loader, core scheduling, firmware image and the XAPP ABI. The SDK owns the Arduino entry point, build recipes, package index, and examples. For each supported host build, keep its kernel image and ELF, exported `xapp-host.json`, XAPP API headers, and LVGL/FatFs configuration together. The existing XDOS XAPP build accepts `RPIDUINO_XDOS_HOST` and `RPIDUINO_XDOS_HEADERS` overrides for matching inputs. A host manifest override by itself does not add V2 packaging support.

XDOS 維護載入器、核心調度、開機映像與 XAPP ABI。RPiDuino 維護 Arduino 啟動器、建置設定、套件索引與範例。每版主系統請保留配對的 IMG、ELF、匯出的 `xapp-host.json`、XAPP API 標頭及 LVGL／FatFs 設定。既有配方可以用 `RPIDUINO_XDOS_HOST` 與 `RPIDUINO_XDOS_HEADERS` 指向配對資料。V2 另需整合啟動器與打包配方。

Before listing a V2 Arduino target in Boards Manager, verify that at least two separately built sketches remain active together, can be closed and restarted independently, and use LVGL through the XDOS UI execution context. Record the tested XDOS and SDK revisions.

在開發板管理員提供 Arduino V2 目標前，驗證至少兩支分開編譯的草稿可同時執行、各自關閉及重啟，並由 XDOS UI 執行環境存取 LVGL。記錄通過驗收的 XDOS 與 SDK 版本。

[FILEQ128 release](https://github.com/hypersolver/raspberry-pi-xdos/releases/tag/v0.6.2-rc3-xapp6-fileq128) · [MC4FQ release](https://github.com/hypersolver/raspberry-pi-xdos/releases/tag/v0.6.2-rc3-xapp6-mc4fq)
