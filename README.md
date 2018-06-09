# pic32mzdask-ips
Driving IPS display on PIC32MZDA Starter Kit

Display is 240x240 pixels, 1.3 inch, ST7789. Display is available at https://www.aliexpress.com/wholesale?SearchText=ST7789+1.3

Connect LCD pins to J15 base board header pins as follows:

| PIC32MZDA Starter Kit      | IPS Display          |
| -------------------------- | -------------------- |
|  6 (GND)                   | GND                  |
|  1 (+3.3V)                 | VCC                  |
| 23 (SPI_SCLK)              | SCL                  |
| 19 (SPI_MOSI)              | SDA                  |
| 29 (I/O4)                  | RES                  |
| 31 (I/O5)                  | DC                   |
| No connection (leave open) | BLK                  |

UART baud rate: 115200

### Build under Linux

    $ export CROSS_COMPILE=mips-linux-gnu-
    $ git clone --recursive https://github.com/osfive/pic32mzdask-ips
    $ cd pic32mzdask-ips
    $ bmake

### Build under FreeBSD

    $ setenv CROSS_COMPILE mips-unknown-freebsd11.1-
    $ git clone --recursive https://github.com/osfive/pic32mzdask-ips
    $ cd pic32mzdask-ips
    $ make

![alt text](https://raw.githubusercontent.com/osfive/pic32mzdask-ips/master/images/pic32mzdask-ips.jpg)
