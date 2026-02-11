# X11 Desktop Mirror for ST7789 (Raspberry Pi 4 / 400)
Real-time desktop mirroring from X11 to a 240x240 SPI ST7789 display using the bcm2835 hardware library and MIT-SHM (XShm) for fast screen capture.
The program captures the X11 root window, scales it to 240x240, converts colors to RGB565, and sends only the modified regions to the display for better performance.

---

# What This Project Does
- Captures the X11 desktop in real time
- Scales the image to 240x240 resolution
- Converts RGB888 to RGB565
- Sends only changed areas (partial updates)
- Mirrors the screen to an SPI ST7789 display
- Uses XShm for high-speed framebuffer capture

---

![20260211_203803](https://github.com/user-attachments/assets/39aecac0-c8d6-44fd-9231-fee0193ed438)

![20260211_203821](https://github.com/user-attachments/assets/c12ff97e-c2f3-4aa9-8ee2-1cc81dd64005)

---

# Requirements
## Hardware
- Raspberry Pi (tested on Raspberry Pi 4 / 400)
- 240x240 SPI ST7789 display
- SPI enabled
- GPIO wiring:

| Function | GPIO |
|----------|------|
| DC       | 24   |
| RST      | 22   |
| BL       | 17   |

## Software
- Raspberry Pi OS with X11 (desktop environment required)
- SPI enabled (raspi-config)
- bcm2835 library
- X11 with MIT-SHM extension

Install dependencies:
```bash
sudo apt update
sudo apt install libx11-dev libxext-dev
```

Install bcm2835 library:
```bash
wget http://www.airspayce.com/mikem/bcm2835/bcm2835-latest.tar.gz
tar zxvf bcm2835-latest.tar.gz
cd bcm2835-*
./configure
make
sudo make install
```
---

# Compile
Makefile

```bash
make run
```
or 

```bash
gcc main.c -o mirror_screen.c -lbcm2835 -lX11 -lXext
./mirror_screen
```
---

# Run
Make sure:
- SPI is enabled
- X11 is running
- The display is properly connected

Expected output:

Mirroring Desktop (WIDTHxHEIGHT) to ST7789...

---

# Important Configuration

## Color Mode
```
st7789_cmd(0x3A);
st7789_data(0x55); // 16-bit RGB565
```

If set to 0x05, the image may appear duplicated or corrupted.

## Display Orientation
```
st7789_cmd(0x36);
st7789_data(0x00);
```

If the image appears flipped or rotated, try:
```
- 0x70
- 0xC0
```

## SPI Speed
```
bcm2835_spi_set_speed_hz(16000000);
```

You may increase it (e.g., 20000000), but stability depends on wiring and display quality.

---

# How It Works
1. Initializes SPI and ST7789
2. Connects to X11 display server
3. Captures screen using XShmGetImage
4. Scales image proportionally to 240x240
5. Converts RGB888 to RGB565
6. Compares with previous framebuffer
7. Sends only modified regions via SPI

This reduces SPI bandwidth usage and improves performance.

---

# Notes
- Runs in an infinite loop (Ctrl+C to stop)
- Requires root privileges (SPI access)
- Works only with X11 (not Wayland)
- CPU usage depends on desktop resolution

---

# Possible Improvements
- FPS limiting
- True double buffering
- Dynamic rotation support
- Multi-threaded capture
- Wayland support
- DMA-based SPI transfer

---

# License
Free to use for educational and personal projects.
