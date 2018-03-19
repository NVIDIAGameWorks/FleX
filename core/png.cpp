#include "png.h"

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb_image/stb_image.h"


bool PngLoad(const char* filename, PngImage& image)
{	
	int x, y, c;
	
	uint8_t* data = stbi_load(filename, &x, &y, &c, 4);
	
	if (data)
	{
		int s = x*y;
		
		image.m_data = new uint32_t[s];
		memcpy(image.m_data, data, s*sizeof(char)*4);
		
		image.m_width = (unsigned short)x;
		image.m_height = (unsigned short)y;
		
		stbi_image_free(data);
	
		return true;
	}
	else 
	{
		return false;
	}
}

void PngFree(PngImage& image)
{
	delete[] image.m_data;
}

bool HdrLoad(const char* filename, HdrImage& image)
{	
	int x, y, c;
	
	float* data = stbi_loadf(filename, &x, &y, &c, 4);
	
	if (data)
	{
		int s = x*y;
		
		image.m_data = new float[s*4];
		memcpy(image.m_data, data, s*sizeof(float)*4);
		
		image.m_width = (unsigned short)x;
		image.m_height = (unsigned short)y;
		
		stbi_image_free(data);
	
		return true;
	}
	else 
	{
		return false;
	}
}

void HdrFree(HdrImage& image)
{
	delete[] image.m_data;
}

