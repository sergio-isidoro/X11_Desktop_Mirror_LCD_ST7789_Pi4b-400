#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/shm.h>
#include <bcm2835.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// Hardware Definitions
#define DISP_W 240
#define DISP_H 240
#define PIN_DC 24
#define PIN_RST 22
#define PIN_BL 17

// Frame Buffers
uint16_t curr_fb[DISP_W * DISP_H] __attribute__((aligned(32)));
uint16_t last_fb[DISP_W * DISP_H] __attribute__((aligned(32)));

void st7789_cmd(uint8_t cmd) {
    bcm2835_gpio_write(PIN_DC, LOW);
    bcm2835_spi_transfer(cmd);
}

void st7789_data(uint8_t data) {
    bcm2835_gpio_write(PIN_DC, HIGH);
    bcm2835_spi_transfer(data);
}

void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    st7789_cmd(0x2A); // Column Address Set
    st7789_data(x0 >> 8); st7789_data(x0 & 0xFF);
    st7789_data(x1 >> 8); st7789_data(x1 & 0xFF);
    
    st7789_cmd(0x2B); // Row Address Set
    st7789_data(y0 >> 8); st7789_data(y0 & 0xFF);
    st7789_data(y1 >> 8); st7789_data(y1 & 0xFF);
    
    st7789_cmd(0x2C); // Write to RAM
}

void init_st7789() {
    bcm2835_gpio_write(PIN_RST, LOW);  bcm2835_delay(100);
    bcm2835_gpio_write(PIN_RST, HIGH); bcm2835_delay(100);

    st7789_cmd(0x01); bcm2835_delay(150); // Software Reset
    st7789_cmd(0x11); bcm2835_delay(150); // Sleep Out

    // Important: 0x55 defines 16-bit color. If it's 0x05, you can triple the image size.
    st7789_cmd(0x3A); st7789_data(0x55); 

    // Orientation: 0x00 is the default. If it's inverted, try 0x70 or 0xC0.
    st7789_cmd(0x36); st7789_data(0x00); 

    st7789_cmd(0x21); // Color Inversion (common in ST7789 IPS panels)
    st7789_cmd(0x13); // Normal Display Mode
    st7789_cmd(0x29); // Display ON
    bcm2835_delay(50);
}

int main() {
    if (!bcm2835_init()) return 1;
    
    bcm2835_spi_begin();
    // Reduce it to 16MHz for greater stability on the RPi4.
    bcm2835_spi_set_speed_hz(16000000); 
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    
    bcm2835_gpio_fsel(PIN_DC, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_RST, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_BL, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(PIN_BL, HIGH);

    init_st7789();

    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) { printf("Erro: X11 não encontrado.\n"); return 1; }
    
    Window root = DefaultRootWindow(dpy);
    XWindowAttributes attr;
    XGetWindowAttributes(dpy, root, &attr);

    XShmSegmentInfo shminfo;
    XImage *img = XShmCreateImage(dpy, attr.visual, attr.depth, ZPixmap, NULL, &shminfo, attr.width, attr.height);
    shminfo.shmid = shmget(IPC_PRIVATE, img->bytes_per_line * img->height, IPC_CREAT | 0777);
    shminfo.shmaddr = img->data = shmat(shminfo.shmid, 0, 0);
    XShmAttach(dpy, &shminfo);

    printf("Espelhando Desktop (%dx%d) para ST7789...\n", attr.width, attr.height);

    while (1) {
        XShmGetImage(dpy, root, img, 0, 0, AllPlanes);
        
        int x_min = DISP_W, y_min = DISP_H, x_max = -1, y_max = -1;
        int changed = 0;

        for (int y = 0; y < DISP_H; y++) {
            uint32_t py = (y * attr.height) / DISP_H;
            uint32_t *line = (uint32_t *)(img->data + py * img->bytes_per_line);
            
            for (int x = 0; x < DISP_W; x++) {
                uint32_t px = (x * attr.width) / DISP_W;
                uint32_t p = line[px];
                
                // RGB extraction (Assumes standard X11 BGRX or RGBX format)
                uint8_t r = (p >> 16) & 0xFF; 
                uint8_t g = (p >> 8) & 0xFF;
                uint8_t b = p & 0xFF;
                
                // Conversion to RGB565 and byte swapping (Big Endian for SPI)
                uint16_t color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                uint16_t final_color = (color >> 8) | (color << 8);
                
                if (final_color != last_fb[y * DISP_W + x]) {
                    curr_fb[y * DISP_W + x] = final_color;
                    if (x < x_min) x_min = x; 
                    if (x > x_max) x_max = x;
                    if (y < y_min) y_min = y; 
                    if (y > y_max) y_max = y;
                    changed = 1;
                }
            }
        }

        if (changed) {
            set_window(x_min, y_min, x_max, y_max);
            bcm2835_gpio_write(PIN_DC, HIGH);
            int block_w = (x_max - x_min) + 1;
            
            for (int r = y_min; r <= y_max; r++) {
                // Send line by line from the changed area.
                bcm2835_spi_transfern((char*)&curr_fb[r * DISP_W + x_min], block_w * 2);
            }
            memcpy(last_fb, curr_fb, sizeof(curr_fb));
        }
    }

    // Cleanup (Optional, the loop is infinite)
    XShmDetach(dpy, &shminfo);
    shmdt(shminfo.shmaddr);
    shmctl(shminfo.shmid, IPC_RMID, 0);
    bcm2835_spi_end();
    bcm2835_close();
    return 0;
}
