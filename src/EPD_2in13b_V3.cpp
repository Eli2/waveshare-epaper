/*****************************************************************************
* | File      	:   EPD_2in13b_V3.c
* | Author      :   Waveshare team
* | Function    :   2.13inch e-paper b V3
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2020-04-13
* | Info        :
* -----------------------------------------------------------------------------
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "EPD_2in13b_V3.h"
#include "Debug.h"

#include <fcntl.h>

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>

#include "dev_spi.h"

constexpr const char * spiDevice = "/dev/spidev0.0";
constexpr const char * gpioChipname = "/dev/gpiochip0";

///  External reset pin (Low for reset)
constexpr uint16_t EPD_RST_PIN  = 17;
///  Data/Command control pin (High for data, and low for command)
constexpr uint16_t EPD_DC_PIN   = 25;
///  Busy state output pin (Low for busy)
constexpr uint16_t EPD_BUSY_PIN = 24;


static struct gpiod_chip *chip;
static struct gpiod_line *line_RST;
static struct gpiod_line *line_DC;
static struct gpiod_line *line_BUSY;


static void GPIOSetLine(struct gpiod_line *line, int value) {
    int ret = gpiod_line_set_value(line, value);
    if (ret < 0) {
      perror("Set line output failed\n");
    }
}

enum class Command : uint8_t {
    PSR  = 0x00, ///< Panel Setting
    POF  = 0x02, ///< Power OFF
    PON  = 0x04, ///< Power ON
    DSLP = 0x07, ///< Deep sleep
    DTM1 = 0x10, ///< Display Start Transmission 1 (White/Black Data)
    DRF  = 0x12, ///< Display Refresh
    DTM2 = 0x13, ///< Display Start transmission 2 (Red Data)
    CDI  = 0x50, ///< VCOM and data interval setting
    TRES = 0x61, ///< Resolution setting
    FLG  = 0x71, ///< Get Status (FLG)
};

static void SendCommand(Command command) {
    GPIOSetLine(line_DC, 0);
    DEV_HARDWARE_SPI_TransferByte(static_cast<uint8_t>(command));
}

static void SendData(uint8_t Data) {
    GPIOSetLine(line_DC, 1);
    DEV_HARDWARE_SPI_TransferByte(Data);
}

/******************************************************************************
function :	Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
static void WaitForIdle()
{
    int busy;
    Debug("e-Paper busy\r\n");
    int count = 0;
    do{
        SendCommand(Command::FLG);
        //busy = SYSFS_GPIO_Read(EPD_BUSY_PIN);
        busy = gpiod_line_get_value(line_BUSY);
        if(busy == -1) {
            perror("Get busy line failed\n");
            break;
        }
        count++;
        //busy =!(busy & 0x01);
    }while(busy == 0);
    Debug("e-Paper busy release %d\r\n", count);
    //DEV_Delay_ms(200);
}

/******************************************************************************
function :	Initialize the e-Paper register
parameter:
******************************************************************************/
int EPD_2IN13B_V3_Init()
{

    LogInfo("Init GPIO: %s", gpioChipname);
    {
        chip = gpiod_chip_open(gpioChipname);
        if (!chip) {
            perror("Chip open failed\n");
            return 1;
        }

        line_RST = gpiod_chip_get_line(chip, EPD_RST_PIN);
        if (!line_RST) {
            perror("Get rst line failed\n");
            return 1;
        }

        line_DC = gpiod_chip_get_line(chip, EPD_DC_PIN);
        if (!line_DC) {
            perror("Get dc line failed\n");
            return 1;
        }

        line_BUSY = gpiod_chip_get_line(chip, EPD_BUSY_PIN);
        if (!line_BUSY) {
            perror("Get busy line failed\n");
            return 1;
        }

        int ret;

        ret = gpiod_line_request_output(line_RST, "example1", 0);
        if (ret < 0) {
            perror("Request rst line as output failed\n");
            return 1;
        }

        ret = gpiod_line_request_output(line_DC, "example1", 0);
        if (ret < 0) {
            perror("Request dc line as output failed\n");
            return 1;
        }

        ret = gpiod_line_request_input(line_BUSY, "example1");
        if (ret < 0) {
            perror("Request busy line as input failed\n");
            return 1;
        }
    }

    LogInfo("Init SPI: %s", spiDevice);
    {
        DEV_HARDWARE_SPI_begin(spiDevice);
        DEV_HARDWARE_SPI_setSpeed(10000000);
    }

    LogInfo("Hardware reset");
    {
        GPIOSetLine(line_RST, 1);
        usleep(1000);
        GPIOSetLine(line_RST, 0);
        usleep(1000);
        GPIOSetLine(line_RST, 1);
        usleep(1000);
    }

    LogInfo("Display setup");
    {
        SendCommand(Command::PON);
        WaitForIdle();

        SendCommand(Command::PSR);
        SendData(0x0f);//LUT from OTP，128x296
        SendData(0x89);//Temperature sensor, boost and other related timing settings

        SendCommand(Command::TRES);
        SendData(0x68); // HRES
        SendData(0x00); // VRES
        SendData(0xD4); // VRES

        SendCommand(Command::CDI);
        SendData(0x77);// WBmode:VBDF 17|D7 VBDW 97 VBDB 57
                       // WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7;
    }

    return 0;
}

void EPD_2IN13B_V3_Exit()
{
    DEV_HARDWARE_SPI_end();
    GPIOSetLine(line_DC, 0);
    GPIOSetLine(line_RST, 0);

    gpiod_line_release(line_RST);
    gpiod_line_release(line_DC);
    gpiod_line_release(line_BUSY);
    gpiod_chip_close(chip);
}

void EPD_2IN13B_V3_Sleep()
{
    SendCommand(Command::CDI);
    SendData(0xf7);

    SendCommand(Command::POF);
    WaitForIdle();

    SendCommand(Command::DSLP);
    SendData(0xA5); // Deep sleep Check code

    // important, at least 2s
    usleep(2000 * 1000);
}

void EPD_2IN13B_V3_Display(const Image & image)
{    
    SendCommand(Command::DTM1);
    for (uint16_t j = 0; j < image.m_height; j++) {
        for (uint16_t i = 0; i < image.m_widthPadded; i++) {
            SendData(image.m_blackImage[i + j * image.m_widthPadded]);
        }
    }
    
    SendCommand(Command::DTM2);
    for (uint16_t j = 0; j < image.m_height; j++) {
        for (uint16_t i = 0; i < image.m_widthPadded; i++) {
            SendData(image.m_accentImage[i + j * image.m_widthPadded]);
        }
    }

    SendCommand(Command::DRF);
    WaitForIdle();
}


