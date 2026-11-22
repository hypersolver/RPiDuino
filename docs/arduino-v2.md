# Pi 3B+ 的 Arduino V2 測試

RPiDuino 0.1.5 已提供 XDOS XAPP V2。FVCORE2 安裝、配對與實測見 [0.1.5 指南](v0.1.5.md)。下方保留 MC4FQ 與 IPC 測試流程。

## 建置

1. 將本倉庫的 `hardware/rpiduino` 複製至 Arduino Sketchbook 的 `hardware` 資料夾。Ubuntu／WSL 除了既有的 GNU ARM 工具鏈，另需 `python3-pyelftools`。
2. 準備同一次 MC4FQ 建置產出的 `kernel.elf` 和 `kernel.img`。將 XDOS 的 `xdos_sdk_v1.h`、`xdos_corejobs.h` 和由 `tools/export_xapp_headers.py` 匯出的配對標頭放進同一目錄的 `headers/`。
3. 啟動 Arduino IDE 前，設定 `RPIDUINO_XDOS_V2_HOST` 為上述目錄的絕對路徑。Windows 請用 Windows 路徑並重新啟動 IDE。
4. 選 **RPiDuino 3/3B+ → Build target → XDOS XAPP V2 (MC4FQ)**，再開啟 **RPiDuino Basics → XDOSV2** 的範例。Arduino CLI 的 FQBN 為 `rpiduino:bcm:rpiduino3:target=xappv2`。
5. 上傳後記下工具列出的 `.XAPP` 檔名。以 XDOS UART 的 `XAPP2 <檔名>` 啟動，並用 `XAPP2-STATUS` 和 `XAPP2-CLOSE <槽號>` 驗證槽的使用與回收。

兩組草稿在範例選單中的完整路徑及操作方式見 [首批範例](examples.md)。

## 驗收

先用 CPU 0 或不同的工作核心分別編譯、啟動 `V2Counter` 和 `V2Monitor`。DualSketch 需要具備 IPC 匯出的測試核心。兩個獨立的 V2 槽都應顯示為使用中，UART 應各出現一行首次更新的訊息。分別關閉再重新開啟，確認另一支程式持續運作。

`TunnelSender` 和 `TunnelDisplay` 另需具有 `xdos_v2_ipc_*` 匯出的 MC4FQ 測試核心。兩支草稿均使用 CPU 0 設定。先啟動顯示端，再啟動傳送端。傳送端在有空閒核心時借用它產生下一筆資料，回到 core 0 投遞給 XDOS 的共享記憶體 FIFO。顯示端在 LVGL 執行上下文讀取資料並更新 Label，首次及第五次更新會把 Label 內容回報到 UART。關閉並重開顯示端可檢查通道註冊和接收佇列是否重設。已排入的資料受佇列容量限制，傳送端在接收端不在時跳過該筆資料。

## 為 V2 草稿選 CPU

選 **工具 → Sketch CPU (XDOS V2)**，重新編譯並上傳。這個設定只影響 V2。CPU 0 是預設值，CPU 2 和 CPU 3 是獨占的工作核心。Auto 先試 CPU 3，再試 CPU 2。指定的核心無法借用，或 Auto 找不到可用核心時，XDOS UART 會顯示原因並拒絕啟動。CPU 1 保留給 LVGL。

`setup()` 和可選的 `extern "C" void xdos_sketch_service()` 都在 CPU 0 執行，服務回呼可處理 LVGL 與 IPC。工作核心上的 `loop()` 應只做可在該核心安全執行的工作，並定期返回。LVGL、FatFs 和本版 Message Tunnel API 由 CPU 0 呼叫。DualSketch 用共享的原子欄位把 `loop()` 實測值交給 CPU 0，由服務回呼送入 Message Tunnel、取出資料並更新 Label。

關閉時執行環境先停止和歸還工作核心，再卸載草稿。可提供 `extern "C" bool xdos_sketch_can_stop()` 等候草稿額外借出的工作結束，並在 `extern "C" void xdos_sketch_stop()` 清理。

## IPC 測試核心

在已套用 MC4FQ 與 XAPPMEM 配額配置的獨立 Circle 建置樹，使用 [核心補丁與共用佇列實作](../xdos-host/message-tunnel)：

```sh
python3 xdos-host/message-tunnel/apply_message_tunnel.py /path/to/circle/sample/xdos-sdboot/xapp_v2_runtime.cpp
make -C /path/to/circle/sample/xdos-sdboot
```

以此次建置產出的 IMG、ELF、標頭重新配對四個範例。既有 MC4FQ 發行映像沒有此測試核心的 IPC 匯出。

Pi 3B+ 已透過 UART 驗證四個範例的上傳和啟動。先測得 `V2Counter` 與 `V2Monitor` 兩槽並行及單獨重開，再測得 IPC 接收端更新 LVGL Label，回報值依序為 `1`、`5`，重開接收端後為 `19`、`23`。關閉後四槽皆回收。此紀錄來自 UART 與程式回讀，未另做 HDMI 目視驗收。

## Screen 清單與工作核心上的 delay()

選 **工具 → XAPP Screen (XDOS V2)** 的 **None**（預設）、**Window** 或 **Full**。編譯會在建置目錄的 XAPP 旁產生 .APP 清單，明列 Screen 和 SD 上的配對檔名。手動安裝時，依編譯輸出指定的檔名放入 `XDOS/INBOX`，再把 .APP 放入 `XDOS/APPS`。使用 UART 上傳時，工具會更新建置目錄的 .APP，使 Exec 指向實際傳輸的檔名。若要從桌面啟動，仍需將 .APP 放入 `XDOS/APPS`。

FVCORE2 或相容核心支援 Arduino V2 工作核心的 `delay()`。工作核心在延遲期間可以睡眠，收到關閉請求會提前返回。草稿可在 delay() 後呼叫 `rpiduino_xdos_stop_requested()`，並從 loop() 返回以便釋放核心。CPU0 的 Arduino delay() 沿用核心原本的實作。

## Screen manifests and borrowed-core delay

Select **Tools → XAPP Screen (XDOS V2)** and choose **None** (default), **Window**, or **Full**. The build generates an .APP beside the XAPP in the build directory. To install manually, use the executable filename shown by the build in `XDOS/INBOX`, and place the .APP in `XDOS/APPS`. UART upload updates the local .APP to refer to the uploaded filename. Copy that manifest to `XDOS/APPS` for a desktop shortcut.

On FVCORE2 or a compatible host, `delay()` sleeps on a borrowed core and returns early when the host requests closure. Check `rpiduino_xdos_stop_requested()` after delay and return from loop() when it is true. CPU0 continues to use the host's original delay implementation.
