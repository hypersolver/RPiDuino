# XDOS 多核心與程式間通訊

Pi 3B+ 的 XDOS MC4FQ 能同時執行最多四個符合該版 ABI 的 V2 XAPP。Arduino 草稿須先用相容的 V2 建置工具與同版 XDOS host SDK 編成 V2 XAPP，才能使用這個能力。RPiDuino 0.1.4 的內附 Arduino XAPP 流程以 FILEQ128 為目標。要試跑多程式，先依發行說明從 FILEQ128 升級並使用發行包配對的 V2 範例。裸機 IMG 模式則從單一開機映像執行一份 Arduino 草稿。[MC4FQ 發行及升級說明](https://github.com/hypersolver/raspberry-pi-xdos/releases/tag/v0.6.2-rc3-xapp6-mc4fq)

[SDK 與 XDOS ABI 相容性](xdos-compatibility.md)

## Arduino 程式之間的通訊設計

建議把 Message Tunnel 定義成程式看得到的 IPC 介面，底層以共享記憶體承載資料。原始碼的試驗版提供固定長度 FIFO 和兩個可配對執行的範例，需搭配提供 IPC 匯出的 MC4FQ 測試核心。操作方式見 [Arduino V2 測試指南](arduino-v2.md)。

- **控制訊息與感測值**用固定容量的訊息佇列。每條通道可先由一個程式寫入、一個程式讀取。雙向通訊用兩條佇列。定義訊息類型、序號與長度，並明訂佇列滿時回報忙碌或略過舊的遙測資料。
- **畫面與大量取樣資料**用共享緩衝區。訊息只帶緩衝區編號、有效長度與序號。產生者交付後，等消費者歸還所有權才重用緩衝區。
- **跨核心同步**用 acquire／release 原子操作發佈與讀取佇列索引，並依目標的快取與 DMA 行為處理同步。初版可在各 sketch 的 `loop()` 非阻塞輪詢，量到需要更低的喚醒延遲再接 XDOS 支援的事件或核心間中斷。
- **LVGL 與周邊**各指定一個擁有者。例如感測草稿送出新數值，由 UI 草稿在 XDOS 的 LVGL 執行上下文更新 Label。對 UART、SPI 或同一 GPIO 也可用同樣的所有權規則。

若部署需要隔離或故障防護，由 XDOS 仲介與驗證訊息，並依主系統的記憶體保護能力劃分存取權。共享記憶體本身只提供傳輸空間。
