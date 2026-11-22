# 用 Arduino 建立 LVGL UI

先在 Pi 3B+ 準備與板卡包相容的 XDOS FILEQ128。Arduino IDE 選 **RPiDuino 3/3B+ → 工具 → Build target → XDOS XAPP**。從 **檔案 → 範例 → RPiDuino Basics → XDOS → XappLvgl** 開啟按鈕計數器。驗證後選 Debug UART 的序列埠並按上傳，再從 XDOS Files 開啟 `SD:/XDOS/INBOX` 下的程式。點按按鈕便會更新計數。[XDOS 範例與版本配對](xdos-examples.md)

草稿在 `setup()` 建立 LVGL 元件並註冊事件，XDOS 提供 LVGL 執行環境。改用其他 XDOS 核心時，需要配對的 XAPP ABI 符號、核心指紋與對應標頭。

裸機顯示可參考 [RPiduino_LVGL_RGB565](../libraries/RPiduino_LVGL_RGB565)。`RAMFramebuffer` 示範把 Label 畫進 RAM。要接 HDMI，需由目標程式提供 LVGL、計時與板端 RGB565 framebuffer 或輸出回呼。
