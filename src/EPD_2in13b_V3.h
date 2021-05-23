#pragma once

#include "stdint.h"

// Display resolution
#define EPD_2IN13B_V3_WIDTH       104
#define EPD_2IN13B_V3_HEIGHT      212

#include <vector>


enum class ImageColor : uint8_t {
    White,
    Black,
    Accent ///< Usually Red or Yellow
};

struct Image {

    Image(uint16_t WIDTH, uint16_t HEIGHT) {
        m_width = WIDTH;
        m_height = HEIGHT;
        m_widthPadded = (WIDTH % 8 == 0) ? (WIDTH / 8) : (WIDTH / 8 + 1);
        uint16_t Imagesize = m_widthPadded * HEIGHT;

        m_blackImage.resize(Imagesize, 0xFF);
        m_accentImage.resize(Imagesize, 0xFF);
    }

    uint16_t m_width;
    uint16_t m_height;
    uint16_t m_widthPadded;
    std::vector<uint8_t> m_blackImage;
    std::vector<uint8_t> m_accentImage;

    void clear() {
        std::fill(m_blackImage.begin(), m_blackImage.end(), 0xFF);
        std::fill(m_accentImage.begin(), m_accentImage.end(), 0xFF);
    }

    void set(uint16_t x, uint16_t y, ImageColor Color) {
        uint32_t Addr = x / 8 + y * m_widthPadded;

        switch (Color) {
            case ImageColor::White : {
                m_blackImage[Addr] |= (0x80 >> (x % 8));
                m_accentImage[Addr] |= (0x80 >> (x % 8));
                break;
            }
            case ImageColor::Black : {
                m_blackImage[Addr] &= ~(0x80 >> (x % 8));
                break;
            }
            case ImageColor::Accent : {
                m_accentImage[Addr] &= ~(0x80 >> (x % 8));
                break;
            }
        }
    }
};

int EPD_2IN13B_V3_Init();
void EPD_2IN13B_V3_Exit();
void EPD_2IN13B_V3_Sleep();

void EPD_2IN13B_V3_Display(const Image & image);
