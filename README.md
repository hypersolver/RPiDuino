# RPiDuino

[English](README.en.md)

RPiDuino 是 Raspberry Pi 的 Arduino IDE 板卡套件。用 `setup()` 和 `loop()` 寫草稿，可編譯成 Pi 從 microSD 啟動的裸機 IMG。Pi 3B+ 也可將草稿建置成 XDOS XAPP，利用 XDOS 的 LVGL 顯示與輸入服務。

## 在 Arduino IDE 加入開發板

1. Windows 使用者先安裝 Windows 版 Python 3，並準備預設的 WSL Ubuntu。Ubuntu 使用者可直接在終端機安裝建置工具。
2. 在 Ubuntu 或 WSL Ubuntu 執行以下指令。首次建置需要 Git 網路存取，工具會抓取固定版本的 Circle。

   ```sh
   sudo apt update
   sudo apt install git make python3 gcc-arm-none-eabi libstdc++-arm-none-eabi-dev gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
   ```

3. 開啟 Arduino IDE 2，在 **檔案 → 偏好設定 → 額外的開發板管理員網址** 貼上 RPiDuino 板卡索引。

   ```text
   https://raw.githubusercontent.com/hypersolver/RPiDuino/main/package_rpiduino_index.json
   ```

   [舊版 XDOS 板卡索引](https://raw.githubusercontent.com/hypersolver/raspberry-pi-xdos/main/package_rpiduino_index.json)仍保留，已使用舊網址的安裝可繼續使用。

4. 開啟 **開發板管理員**，搜尋 **RPiDuino** 並安裝 **0.1.4**。在 **工具 → 開發板 → RPiDuino** 選擇對應的 Pi 2、3/3B+、4 或 5。

從原始碼開發時，也可將 [hardware/rpiduino](hardware/rpiduino) 複製到 Arduino Sketchbook 的 `hardware` 目錄，重啟 IDE 後選擇該開發板。

## 建立第一個 Arduino 程式

在 **檔案 → 新增草稿** 貼上以下內容，另存為 `FirstBlink.ino`。也可以從 **檔案 → 範例 → RPiDuino Basics → Blink** 開啟套件範例。Pi 3/3B+ 請在 **工具 → Build target** 選 **Standalone IMG**。

```cpp
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
```

按 **驗證**，再用 **草稿 → 匯出編譯的二進位檔** 取得 IMG。第一次部署可走 microSD。依機型把 IMG 放進有對應開機韌體的 FAT 開機分割區，按照 [上傳及開機指南](docs/upload.md#從-microsd-啟動) 設定檔名和 `config.txt`。插卡開機後，板上的 ACT LED 應每半秒切換一次。XDOS 專案已在 Pi 3B+ 以 0.1.4 驗證 Arduino IDE 編譯、UART 上傳及 ACT LED 閃爍。

## 選擇上傳方式

| 方式 | 機型與前置條件 | 結果 |
| --- | --- | --- |
| microSD | Pi 2、3/3B+、4、5，準備對應開機韌體 | 從卡片直接啟動裸機 IMG |
| Debug UART | Pi 3/3B+，XDOS 測試卡與 3.3 V USB–TTL | IDE 上傳裸機 IMG，再重開機 |
| Ethernet | Pi 3/3B+，XDOS、有線區網與 Debug UART | 外部更新工具經區網傳 IMG，由 UART 送出控制指令 |
| USB-C CDC | Pi 4B／5，Raspberry Pi OS 測試卡與 CDC 接收程式 | IDE 上傳裸機 IMG，再重開機。實機流程待驗證 |
| XDOS XAPP | Pi 3B+，配對的 XDOS FILEQ128 與 Debug UART | IDE 上傳 `.xapp` 至 XDOS 的收件目錄，再從 Files 啟動 |

各方式的接線、設定與回復步驟見 [上傳及開機指南](docs/upload.md)。開始製作介面可看 [Arduino LVGL UI](docs/lvgl.md)。想在同一顆 Raspberry Pi 的多個核心上執行多支程式，可看 [XDOS 多核心與程式間通訊](docs/multicore.md)。 V2 原始碼試用方式請參考 [Arduino V2 測試指南](docs/arduino-v2.md)。

## 原始碼與發行

- [板卡平台與內建範例](hardware/rpiduino/bcm)
- [RGB565 Arduino 函式庫](libraries/RPiduino_LVGL_RGB565)
- [Board Manager 套件](packages)與[索引](package_rpiduino_index.json)

更新板卡套件時，先提升 `hardware/rpiduino/bcm/platform.txt` 的版本，再從倉庫根目錄執行 `python package.py`。將新版 ZIP 和索引一起提交，並保留舊版 ZIP 供既有安裝使用。

SDK 自 [Raspberry Pi XDOS](https://github.com/hypersolver/raspberry-pi-xdos) 分出。Circle 為固定版本的建置依賴，XDOS 韌體仍由原專案提供。[來源與授權](docs/provenance.md)
