#include "ImageFile.h"
#include <objidl.h>
#include <gdiplusenums.h>
#include <gdiplus.h>
#include <assert.h>

#pragma comment (lib,"Gdiplus.lib")

ImageFile::ImageFile(unsigned int* FileBuff, int width, int height)
    :
    width(width), height(height)
{
    this->FileBuff = new unsigned int[width * height];
    memcpy(this->FileBuff, FileBuff, width * height * sizeof(unsigned int));
}

ImageFile::ImageFile(const std::wstring filename)
{
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR           gdiplusToken;
    // Initialize GDI+.
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
    {
        Gdiplus::Bitmap bmp(filename.c_str());

        assert(bmp.GetLastStatus() == Gdiplus::Status::Ok);

        height = bmp.GetHeight();
        width = bmp.GetWidth();

        FileBuff = new unsigned int[width * height];

        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
            {
                Gdiplus::Color col;
                bmp.GetPixel(x, y, &col);
                unsigned int color = (unsigned int)col.GetRed() |
                    col.GetGreen() << 8 | col.GetBlue() << 16 | col.GetAlpha() << 24;
                FileBuff[y * width + x] = color;
            }

    }
    Gdiplus::GdiplusShutdown(gdiplusToken);
}

unsigned int ImageFile::GetColorAt(int x, int y) const
{
    return FileBuff[y * width + x];
}

unsigned int* const ImageFile::GetFilePtr() const
{
    return FileBuff;
}

const int ImageFile::GetWidth() const noexcept
{
    return width;
}

const int ImageFile::GetHeight() const noexcept
{
    return height;
}


ImageFile::~ImageFile()
{
    if (FileBuff != nullptr)
    {
        delete[] FileBuff;
        FileBuff = nullptr;
    }
}