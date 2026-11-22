# RPiduino LVGL 9 RGB565 0.1.0

這是依 RPiduino 交接資料重建的 Arduino C++ 顯示函式庫。原始套件及 API 未取得，既有 sketch 需要依下列介面移植。

## 安裝與範例

1. 在 Arduino IDE 使用「Sketch → Include Library → Add .ZIP Library」匯入套件。
2. 選擇現有的 RPiduino board/core，並使用 LVGL 9.2.2 以上的 9.x 版本。啟用 LVGL 軟體繪圖、RGB565 和 Label。若系統已連結 LVGL，請使用系統的同一份 LVGL 與設定。
3. 開啟 `RAMFramebuffer` 範例。它將 Label 畫入 RAM，方便在除錯器檢視像素。要讓畫面出現在 HDMI，需改用板端提供的 RGB565 framebuffer，或接上板端顯示傳輸回呼。

## 顯示介面

- `createRGB565Display(config)` 建立一個使用部分更新的 RGB565 display。呼叫前先初始化 LVGL。可傳入同容量的第二份 draw buffer，並使用 `LV_DRAW_BUF_ALIGN` 對齊。每份容量至少要有一行，含 LVGL 的 stride padding。
- `RGB565Write` 收到更新範圍、像素指標和每行位元組數。回傳前必須消耗完像素。函式庫在回傳後通知 LVGL 可以重用 draw buffer。
- `writeRGB565Framebuffer` 按來源 stride 與目的 pitch 逐行複製；目的端需是可由 CPU 寫入、採用原生 RGB565 排列的畫面記憶體。板端負責提供正確的映射、色序及必要的 cache 維護。也可用自訂回呼銜接 Circle，並在回傳前完成傳輸。
- 設定、回呼 context 及 draw buffers 必須保持有效，直到由同一 LVGL 執行上下文呼叫 `lv_display_delete()`。自訂回呼內避免呼叫 LVGL 或刪除 display。

## XDOS 整合

XDOS 已有 LVGL 事件迴圈及顯示器時，請沿用其初始化、tick 和 timer handler。若 sketch 只要在既有桌面建立 widget，可使用現有 display 及其畫面。需要由本函式庫接管一個輸出時，請先取得 XDOS/Circle 顯示介面，並確保同一個 framebuffer 只有一個輸出擁有者。

此套件提供 Arduino 函式庫原始碼。RPiduino 板卡套件、XAPP ABI 的 LVGL 符號匯出及 Circle 驅動整合由目標環境提供。XDOS V0.5.5 的現有 XAPP 應使用該版本公開的 SDK API。
