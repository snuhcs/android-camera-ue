
#pragma once

namespace ImageFormatUtils
{
    void YUV420SPToARGB8888(char *input, int width, int height, int *output);
    int YUV2RGB(int y, int u, int v);
    void YUV420ToARGB8888(unsigned char*yData,
        unsigned char*uData,
        unsigned char*vData,
                          int width,
                          int height,
                          int yRowStride,
                          int uvRowStride,
                          int uvPixelStride,
                          int *out);
}
