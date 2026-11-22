# RPiDuino

[繁體中文](README.md)

RPiDuino compiles Arduino sketches into bootable images for Raspberry Pi. The Pi 3B+ profile also builds Arduino sketches as XAPPs for the matching XDOS release.

## Install the board package

1. On Windows, install Python 3 for Windows and prepare Ubuntu as your default WSL distribution. On Ubuntu, use the native terminal.
2. Install the build tools in Ubuntu or WSL. The first build fetches the pinned Circle revision.

   ```sh
   sudo apt update
   sudo apt install git make python3 gcc-arm-none-eabi libstdc++-arm-none-eabi-dev gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
   ```

3. In Arduino IDE 2, open **File → Preferences → Additional Boards Manager URLs**. Add the RPiDuino board index.

   ```text
   https://raw.githubusercontent.com/hypersolver/RPiDuino/main/package_rpiduino_index.json
   ```

   Existing installations can continue using the [legacy XDOS index](https://raw.githubusercontent.com/hypersolver/raspberry-pi-xdos/main/package_rpiduino_index.json).

4. Search for **RPiDuino** in Boards Manager, install **0.1.4**, and select your Raspberry Pi model under **Tools → Board → RPiDuino**.

For source development, copy [hardware/rpiduino](hardware/rpiduino) into your Arduino sketchbook's `hardware` directory and restart the IDE.

## Write and run Blink

Create a sketch with **File → New Sketch**, save it as `FirstBlink.ino`, and paste the following. The same program is available under **File → Examples → RPiDuino Basics → Blink**.

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

On Pi 3/3B+, select **Tools → Build target → Standalone IMG**. Click **Verify** and choose **Sketch → Export Compiled Binary**. Follow the [upload guide](docs/upload.en.md) to boot the exported image. The ACT LED should change state every half second.

| Method | Target | Result |
| --- | --- | --- |
| microSD | Pi 2, 3/3B+, 4 and 5 | Boot a model-specific standalone image |
| Debug UART | Pi 3/3B+ with a compatible XDOS test card and 3.3 V USB–TTL adapter | Upload a standalone image and reboot |
| Ethernet | Pi 3/3B+ with compatible XDOS, wired network and Debug UART | Transfer the image over Ethernet and issue control commands over UART |
| USB-C CDC | Pi 4B and 5 with a Raspberry Pi OS test card and CDC receiver | Upload a standalone image and reboot. Hardware verification is pending |
| XDOS XAPP | Pi 3B+ with matching FILEQ128 and Debug UART | Upload an app to the XDOS inbox and start it from Files |

The Raspberry Pi XDOS project verified the 0.1.4 Blink build in Arduino IDE, uploaded it over UART to a Pi 3B+, and observed its ACT LED blinking.

## LVGL and multicore

Select **XDOS XAPP** and open **File → Examples → RPiDuino Basics → XDOS → XappLvgl** to build an LVGL click counter. Choose the Debug UART port, upload, and launch it from `SD:/XDOS/INBOX` in XDOS Files. Use the host's LVGL instance to create widgets and handle UI events. The optional [RGB565 library](libraries/RPiduino_LVGL_RGB565) also includes a RAM framebuffer example for custom display integration.

On Pi 3B+, MC4FQ supports up to four concurrent compatible V2 XAPPs. See the [SDK compatibility and release notes](docs/xdos-compatibility.md) for the Arduino V2 status and the inputs to pair with each host build. For inter-sketch communication, the proposed design uses an IPC message interface backed by shared memory. See [multicore and IPC design notes (繁體中文)](docs/multicore.md). See the [Arduino V2 source setup guide (繁體中文)](docs/arduino-v2.md) to try the experimental target.

To publish a new board package, bump the version in `hardware/rpiduino/bcm/platform.txt` and run `python package.py` from the repository root. Commit the generated index and archive together. Keep previous archives available.
