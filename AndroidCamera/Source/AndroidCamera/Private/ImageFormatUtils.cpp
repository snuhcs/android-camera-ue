#include "ImageFormatUtils.h"

namespace ImageFormatUtils
{
    const int kMaxChannelValue = 262143;

    void YUV420SPToARGB8888(char* Input, int Width, int Height, int* Output)
    {
        int frameSize = Width * Height;
        for (int j = 0, yp = 0; j < Height; j++) {
            int uvp = frameSize + (j >> 1) * Width;
            int u = 0;
            int v = 0;

            for (int i = 0; i < Width; i++, yp++) {
                int y = 0xff & Input[yp];
                if ((i & 1) == 0) {
                    v = 0xff & Input[uvp++];
                    u = 0xff & Input[uvp++];
                }

                Output[yp] = YUV2RGB(y, u, v);
            }
        }
    }

    int YUV2RGB(int y, int u, int v)
    {
        // Adjust and check YUV values
        y = (y - 16) < 0 ? 0 : (y - 16);
        u -= 128;
        v -= 128;

        // This is the floating point equivalent. We do the conversion in integer
        // because some Android devices do not have floating point in hardware.
        // nR = (int)(1.164 * nY + 2.018 * nU);
        // nG = (int)(1.164 * nY - 0.813 * nV - 0.391 * nU);
        // nB = (int)(1.164 * nY + 1.596 * nV);
        int y1192 = 1192 * y;
        int r = (y1192 + 1634 * v);
        int g = (y1192 - 833 * v - 400 * u);
        int b = (y1192 + 2066 * u);

        // Clipping RGB values to be inside boundaries [ 0 , kMaxChannelValue ]
        r = r > kMaxChannelValue ? kMaxChannelValue : (r < 0 ? 0 : r);
        g = g > kMaxChannelValue ? kMaxChannelValue : (g < 0 ? 0 : g);
        b = b > kMaxChannelValue ? kMaxChannelValue : (b < 0 ? 0 : b);

        return 0xff000000 | ((r << 6) & 0xff0000) | ((g >> 2) & 0xff00) | ((b >> 10) & 0xff);
    }

    void YUV420ToARGB8888(unsigned char* yData, unsigned char* uData, unsigned char* vData, int Width, int Height, int yRowStride, int uvRowStride, int uvPixelStride, int* Out)
    {
        int yp = 0;
        for (int j = 0; j < Height; j++) {
            int pY = yRowStride * j;
            int pUV = uvRowStride * (j >> 1);

            for (int i = 0; i < Width; i++) {
                int uv_offset = pUV + (i >> 1) * uvPixelStride;

                Out[yp++] = YUV2RGB(0xff & yData[pY + i], 0xff & uData[uv_offset], 0xff & vData[uv_offset]);
            }
        }
    }
}