#include <stdlib.h>     //exit()
#include <signal.h>     //signal()
#include <filesystem>

#include "Debug.h"
#include "EPD_2in13b_V3.h"

#include "dev_spi.h"

#include "FreeImage.h"



int imageLoad(Image & image, const char * filename) {

    FREE_IMAGE_FORMAT formato = FreeImage_GetFileType(filename, 0);
    if(formato == FIF_UNKNOWN) {
        LogError("Failed to determine image format.");
        return 1;
    }

    FIBITMAP* initial = FreeImage_Load(formato, filename);
    if(!initial) {
        LogError("Failed to load image.");
        return 1;
    }

    FreeImage_FlipHorizontal(initial);

    unsigned width = FreeImage_GetWidth(initial);
    unsigned height = FreeImage_GetHeight(initial);

    FIBITMAP * rotated;
    if(width == image.m_height && height == image.m_width) {
        LogInfo("Found rotated image");
        rotated = FreeImage_Rotate(initial, 90);
    } else  {
        rotated = initial;
    }

    RGBQUAD bgColor = {0xFF, 0xFF, 0xFF, 0x00};
    FIBITMAP *composited = FreeImage_Composite(rotated, false, &bgColor, nullptr);

//    const char * outName = "test.bmp";
//    FREE_IMAGE_FORMAT out_fif = FreeImage_GetFIFFromFilename(outName);
//    FreeImage_Save(out_fif, result, outName, 0);


    width = FreeImage_GetWidth(composited);
    height = FreeImage_GetHeight(composited);
    if(width != image.m_width || height != image.m_height) {
        LogError("Image size mismatch, expected %dx%d got %dx%d", image.m_width, image.m_height, width, height);
        return 1;
    }


    for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = 0; x < width; x++) {
            RGBQUAD val;
            FreeImage_GetPixelColor(composited, x, y, &val);

            if(val.rgbRed == 0x00 && val.rgbBlue == 0x00 && val.rgbGreen == 0x00) {
                image.set(x, y, ImageColor::Black);
            } else if(val.rgbRed == 0xff && val.rgbBlue == 0x00 && val.rgbGreen == 0x00) {
                image.set(x, y, ImageColor::Accent);
            }
        }
    }

    return 0;
}



void deinitAll() {
    EPD_2IN13B_V3_Exit();
    FreeImage_DeInitialise();
}

void Handler(int signo)
{
    printf("\r\nHandler:exit\r\n");
    deinitAll();
    exit(0);
}

int main(int argc, char* argv[])
{
    signal(SIGINT, Handler);
    setvbuf(stdout, nullptr, _IOLBF, 0);
    FreeImage_Initialise();

    if (argc < 2) {
        fprintf(stderr, "Usage: %s IMAGE_FILE\n", argv[0]);
        return 1;
    }

    const char * freeimageVersion = FreeImage_GetVersion();
    printf("Using freeimage version: %s", freeimageVersion);

    const char * imagePath = argv[1];
    printf("Displaying image: %s\n", imagePath);

    EPD_2IN13B_V3_Init();

    Image image(EPD_2IN13B_V3_WIDTH, EPD_2IN13B_V3_HEIGHT);

    //image.clear();
    //EPD_2IN13B_V3_Display(image);

    int res = imageLoad(image, imagePath);
    if(!res) {
        EPD_2IN13B_V3_Display(image);
    }

    printf("Goto Sleep...\r\n");
    EPD_2IN13B_V3_Sleep();

    deinitAll();
}
