#include <stdlib.h>     //exit()
#include <signal.h>     //signal()

#include "Debug.h"
#include "EPD_2in13b_V3.h"

#include "dev_spi.h"

#include "FreeImage.h"

void  Handler(int signo)
{
    //System Exit
    printf("\r\nHandler:exit\r\n");
    EPD_2IN13B_V3_Exit();

    exit(0);
}

void imageLoad(Image & image, const char * filename) {

    FREE_IMAGE_FORMAT formato = FreeImage_GetFileType(filename,0);
    FIBITMAP* imagen = FreeImage_Load(formato, filename);

    RGBQUAD bgColor = {0xFF, 0xFF, 0xFF, 0x00};
    FIBITMAP *result = FreeImage_Composite(imagen, false, &bgColor, nullptr);

//    const char * outName = "test.bmp";
//    FREE_IMAGE_FORMAT out_fif = FreeImage_GetFIFFromFilename(outName);
//    FreeImage_Save(out_fif, result, outName, 0);

    unsigned Width = FreeImage_GetWidth(imagen);
    unsigned Height = FreeImage_GetHeight(imagen);

    if(Width != EPD_2IN13B_V3_WIDTH) {
        return;
    }
    if(Height != EPD_2IN13B_V3_HEIGHT) {
        return;
    }

    for (uint16_t h = 0; h < Height; h++) {
        for (uint16_t w = 0; w < Width; w++) {
            RGBQUAD val;
            FreeImage_GetPixelColor(result, w, h, &val);

            if(val.rgbRed == 0x00 && val.rgbBlue == 0x00 && val.rgbGreen == 0x00) {
                image.set(w, h, ImageColor::Black);
            } else if(val.rgbRed == 0xff && val.rgbBlue == 0x00 && val.rgbGreen == 0x00) {
                image.set(w, h, ImageColor::Accent);
            }
        }
    }
}


int main(int argc, char* argv[])
{
    signal(SIGINT, Handler);
    setvbuf(stdout, nullptr, _IOLBF, 0);

    if (argc < 2) {
        fprintf(stderr, "Usage: %s IMAGE_FILE\n", argv[0]);
        return 1;
    }

    const char * imagePath = argv[1];
    printf("Displaying image: %s\n", imagePath);

    EPD_2IN13B_V3_Init();

    Image image(EPD_2IN13B_V3_WIDTH, EPD_2IN13B_V3_HEIGHT);

    image.clear();
    EPD_2IN13B_V3_Display(image);

    imageLoad(image, imagePath);
    EPD_2IN13B_V3_Display(image);

    printf("Goto Sleep...\r\n");
    EPD_2IN13B_V3_Sleep();

    printf("close 5V, Module enters 0 power consumption ...\r\n");
    EPD_2IN13B_V3_Exit();
}
