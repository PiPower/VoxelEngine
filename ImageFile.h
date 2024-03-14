#pragma once
#include <string>
class ImageFile
{
public:
	ImageFile(unsigned int* FileBuff, int width, int height);
	ImageFile(const std::wstring filename);
	unsigned int GetColorAt(int x, int y) const;
	unsigned int* const  GetFilePtr() const;
	const int GetWidth() const noexcept;
	const int GetHeight() const noexcept;
	~ImageFile();
private:
	int width;
	int height;
	unsigned int* FileBuff = nullptr;
};