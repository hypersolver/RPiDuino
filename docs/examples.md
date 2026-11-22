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
- **XDOSV2 → MessageTunnelDashboard**。使用具備 IPC 匯出的測試核心，兩支草稿皆選 CPU 0。先啟動 `TunnelDisplay`，再啟動 `TunnelSender`。儀表板顯示實際收到的樣本值與其對應的 0–99 進度條，進度條在樣本值到達 100 的倍數時回到起點。關掉顯示端再開，應繼續收到傳送端的新值。UART 會印出每次啟動後第一與第五次 Label 更新。

MC4FQ 可用相同方式啟動第三、第四個相容 XAPP，四槽用完時第五次啟動會回報 `-8`。可以重複啟動 `V2Counter` 或 `V2Monitor` 試槽位，重複啟動的視窗會出現在相同初始位置，可用滑鼠拖開。測試四個槽時請使用 CPU 0 版本。Dashboard 的通道只允許一個接收端。V2 的 `setup()` 在 CPU 0 執行，`loop()` 預設在 CPU 0，也可以依下節設定工作核心。

## 切換 DualSketch 使用的 CPU

選 **工具 → Sketch CPU (XDOS V2)**。把 `V2Counter` 編譯成 **CPU 2**，把 `V2Monitor` 編譯成 **CPU 3**，分開上傳並啟動。兩個畫面會各自顯示編譯設定、在 `loop()` 裡量到的 CPU 和計數，例如 `CPU 2 -> CPU 2 | Count: 5`。LVGL 由 CPU 0 更新，計數由各自的 `loop()` 產生。DualSketch 和 Dashboard 都需要有 IPC 匯出的測試核心。

選 **CPU 0** 可在 UI 核心執行 `loop()`。**Auto** 優先用 CPU 3，再試 CPU 2；兩顆都忙會拒絕啟動。指定的 CPU 忙碌時也會拒絕啟動。關閉占用該核心的程式後再重試。Arduino CLI 可用 `rpiduino:bcm:rpiduino3:target=xappv2,sketchcpu=core2`，另有 `core0`、`core3` 和 `auto`。

## Pi 3B+ 驗證結果

四個裸機 IMG 均已編譯。MC4FQ 測試核心上，兩支 DualSketch 草稿可並行並各自重開。重複啟動它們可占滿四槽，第五次啟動回報 `-8`。IPC 顯示端的 LVGL Label 透過 UART 回讀為 `1`、`5`，重開後為 `19`、`23`。本次未目視確認 HDMI，按鈕也僅驗證編譯。

Pi 3B+ 的 CPU 選項驗收另確認 `CPU 0 -> CPU 0`，以及 `CPU 2 -> CPU 2`、`CPU 3 -> CPU 3` 同時遞增至 `Count: 10`。當 CPU 3 忙碌時 Auto 選到 CPU 2，兩顆都空閒時選 CPU 3。兩顆都忙時 Auto 回報無可用工作核心，重複指定 CPU 2 或 CPU 3 也回報忙碌。失敗的載入未占用槽。CPU 與計數來自 UART 回讀的 LVGL Label。
