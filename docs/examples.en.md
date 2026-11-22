# First RPiDuino examples

[繁體中文](examples.md)

In Arduino IDE, select **Tools → Board → RPiDuino → RPiDuino 3/3B+**. Open the sketches from **File → Examples → RPiDuino Basics**. These instructions use the current source platform. See the [RPiDuino setup guide](../README.en.md) for manual installation into your sketchbook.

## Standalone IMG basics

Select **Tools → Build target → Standalone IMG**, click **Verify**, then choose **Sketch → Export Compiled Binary**. Follow the [upload and boot guide](upload.en.md) for microSD or compatible XDOS Debug UART deployment.

| Example | Expected result on Raspberry Pi 3B+ |
| --- | --- |
| `BareMinimum` | Empty `setup()` and `loop()` to check compilation and export. The sketch has no visible output. |
| `Blink` | The existing sketch toggles the ACT LED every half second using `delay()`. |
| `BlinkWithoutDelay` | The ACT LED toggles every half second using `millis()`, leaving the loop free to do other work. |
| `ButtonPullupDebounce` | Each press toggles the ACT LED. Holding or releasing the button does not toggle it again. A new input state must be stable for 30 ms. |

Power the Pi off before wiring. Connect a normally open button between **BCM GPIO17 (physical pin 11)** and **GND (physical pin 6)**. The sketch enables the internal pull-up. Keep 5 V off the GPIO pin.

## XDOS V2 concurrency and LVGL

Boot a matching MC4FQ host, connect an HDMI display, and select **Build target → XDOS XAPP V2 (MC4FQ)**. [Build and upload](arduino-v2.md) each sketch separately. Note each uploaded `.XAPP` filename and start it from the XDOS UART console with `XAPP2 <filename>`.

- **XDOSV2 → DualSketch**. Start `V2Counter` and then `V2Monitor`. Two side-by-side windows count at different intervals. Use `XAPP2-STATUS` to see the active slots. Close one with `XAPP2-CLOSE <slot>` and start it again while the other keeps counting.
- **XDOSV2 → MessageTunnelDashboard**. Boot the test kernel with IPC exports. Start `TunnelDisplay` before `TunnelSender`. The dashboard shows the received sample and its 0–99 bar position, which wraps at multiples of 100. Close and reopen the display while the sender stays active. The UART logs the label at the first and fifth updates after each start.

MC4FQ can launch a third and fourth compatible XAPP the same way. With all four slots occupied, a fifth launch returns `-8`. You can launch additional instances of the counter and monitor to try this. Duplicate windows start in the same position and can be dragged apart. The dashboard channel allows one receiver. Each V2 sketch's `setup()` and `loop()` runs on core 0, and a sketch can borrow a worker core through the XDOS job API.

## Identifying the CPU

In `XAPP2-STATUS`, `slots` identifies app instances and `cores` reports worker-core state. A normal DualSketch run shows slots 0 and 1 as `app` and cores 1–3 as `lvgl`.

Call `xdos_this_core()` inside the function whose CPU you want to identify. It reports core 0 inside a DualSketch `loop()`. For a borrowed core, have the job submitted through `xdos_core_lend_any()` return its own measured core number and display that result from core 0.

## Raspberry Pi 3B+ verification

All four standalone images compiled. On the MC4FQ test kernel, both DualSketch apps ran concurrently and could restart independently. Additional instances filled all four slots, and the fifth launch returned `-8`. The dashboard read back LVGL label values of `1`, `5`, then `19`, `23` after restarting the receiver. This run used UART readback. HDMI visual inspection and the physical button test are pending.
