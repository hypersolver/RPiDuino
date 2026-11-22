# RPiDuino 首批範例

[English](examples.en.md)

在 Arduino IDE 選 **工具 → 開發板 → RPiDuino → RPiDuino 3/3B+**，再從 **檔案 → 範例 → RPiDuino Basics** 選取下列草稿。這份清單適用於目前的原始碼安裝，請依 [RPiDuino 入門](../README.md) 將板卡平台加入 Sketchbook。

## 裸機 IMG 入門

在 **工具 → Build target** 選 **Standalone IMG**。按 **驗證**，再用 **草稿 → 匯出編譯的二進位檔**。依 [上傳及開機指南](upload.md) 用 microSD 或配對的 XDOS Debug UART 流程啟動。

| 範例 | 在 Pi 3B+ 上的預期行為 |
| --- | --- |
| `BareMinimum` | 最小的 `setup()` 和 `loop()`。用於驗證編譯和匯出。程式本身沒有可觀察的輸出。 |
| `Blink` | 保留原有範例，用 `delay()` 讓 ACT LED 每半秒切換。 |
| `BlinkWithoutDelay` | 用 `millis()` 讓 ACT LED 每半秒切換，等待期間可以繼續執行其他工作。 |
| `ButtonPullupDebounce` | 每按一下按鈕切換 ACT LED 一次。放開或長按都不會再切換，輸入穩定 30 ms 後才接受變化。 |

關閉 Pi 電源再接按鈕。將常開按鈕接在 **BCM GPIO17（實體 pin 11）** 與 **GND（實體 pin 6）** 之間，草稿啟用內部上拉。請勿將 GPIO 接至 5 V。

## XDOS V2 並行與 LVGL

用已配對的 MC4FQ 主系統，將 **Build target** 改為 **XDOS XAPP V2 (MC4FQ)**。接上 HDMI 螢幕，依 [Arduino V2 建置與上傳](arduino-v2.md) 分別編譯、上傳每個草稿。上傳後記下各自的 `.XAPP` 檔名，在 XDOS UART 用 `XAPP2 <檔名>` 啟動。

- **XDOSV2 → DualSketch**。先啟動 `V2Counter`，再啟動 `V2Monitor`。畫面上左右兩個視窗以不同節奏累計數字。用 `XAPP2-STATUS` 查看兩個槽。使用 `XAPP2-CLOSE <槽號>` 關掉其中一個，再啟動該草稿，另一個計數器會持續執行。
- **XDOSV2 → MessageTunnelDashboard**。使用具備 IPC 匯出的測試核心。先啟動 `TunnelDisplay`，再啟動 `TunnelSender`。儀表板顯示實際收到的樣本值與其對應的 0–99 進度條，進度條在樣本值到達 100 的倍數時回到起點。關掉顯示端再開，應繼續收到傳送端的新值。UART 會印出每次啟動後第一與第五次 Label 更新。

MC4FQ 可用相同方式啟動第三、第四個相容 XAPP，四槽用完時第五次啟動會回報 `-8`。可以重複啟動 `V2Counter` 或 `V2Monitor` 試槽位，重複啟動的視窗會出現在相同初始位置，可用滑鼠拖開。Dashboard 的通道只允許一個接收端。V2 的 `setup()` 與 `loop()` 由 core 0 執行，程式可透過 XDOS 工作 API 借用其他核心。

## 怎麼看 CPU

`XAPP2-STATUS` 的 `slots` 是程式實例編號，`cores` 是工作核心的狀態。比方說，槽 0 和槽 1 都是 `app`，而核心 1–3 仍顯示 `lvgl`，就是這次 DualSketch 的正常結果。

要標出實際 CPU，在要確認的函式內呼叫 `xdos_this_core()`。在 DualSketch 的 `loop()` 量到的是 core 0。若草稿使用 `xdos_core_lend_any()` 借核，請讓工作函式回傳自身量到的核心編號，再由 core 0 把該編號顯示在 LVGL。

## Pi 3B+ 驗證結果

四個裸機 IMG 均已編譯。MC4FQ 測試核心上，兩支 DualSketch 草稿可並行並各自重開。重複啟動它們可占滿四槽，第五次啟動回報 `-8`。IPC 顯示端的 LVGL Label 透過 UART 回讀為 `1`、`5`，重開後為 `19`、`23`。本次未目視確認 HDMI，按鈕也僅驗證編譯。
