// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2013-2016 NVIDIA Corporation. All rights reserved.

#include "tga.h"
#include "core.h"
#include "types.h"

#include <stdio.h>
#include <string.h>

using namespace std;

struct TgaHeader
{
    uint8_t  identsize;          // size of ID field that follows 18 uint8_t header (0 usually)
    uint8_t  colourmaptype;      // type of colour map 0=none, 1=has palette
    uint8_t  imagetype;          // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

    uint16_t colourmapstart;     // first colour map entry in palette
    uint16_t colourmaplength;    // number of colours in palette
    uint8_t  colourmapbits;      // number of bits per palette entry 15,16,24,32

    uint16_t xstart;             // image x origin
    uint16_t ystart;             // image y origin
    uint16_t width;              // image width in pixels
	uint16_t height;             // image height in pixels
    uint8_t  bits;               // image bits per pixel 8,16,24,32
    uint8_t  descriptor;         // image descriptor bits (vh flip bits)
    
    // pixel data follows header  
};

namespace
{

	void memwrite(void* src, uint32_t size, unsigned char*& buffer)
	{
		memcpy(buffer, src, size);
		buffer += size;
	}
}



bool TgaSave(const char* filename, const TgaImage& image, bool rle)
{
	FILE* f = fopen(filename, "wb");
	if (f)
	{
		bool b = TgaSave(f, image, rle);
		fclose(f);

		return b;
	}

	return false;
}

bool TgaSave(FILE* f, const TgaImage& image, bool rle)
{
	TgaHeader header;

	header.identsize = 0;
	header.colourmaptype = 0;
	header.imagetype = rle?9:2;
	header.colourmapstart = 0;
	header.colourmaplength = 0;
	header.colourmapbits = 0;
	header.xstart = 0;
	header.ystart = 0;
	header.width = image.m_width;
	header.height = image.m_height;
	header.bits = 32;
	header.descriptor = 0;//uint16((1<<3)|(1<<5));

	if (f)
	{
		unsigned char* data = (unsigned char*)new uint32_t[image.m_width*image.m_height + sizeof(header)];
		unsigned char* writeptr = data;
				
		memwrite(&header.identsize, sizeof(header.identsize), writeptr);
		memwrite(&header.colourmaptype, sizeof(header.colourmaptype), writeptr);
		memwrite(&header.imagetype, sizeof(header.imagetype), writeptr);
		memwrite(&header.colourmapstart, sizeof(header.colourmapstart), writeptr);
		memwrite(&header.colourmaplength, sizeof(header.colourmaplength), writeptr);
		memwrite(&header.colourmapbits, sizeof(header.colourmapbits), writeptr);
		memwrite(&header.xstart, sizeof(header.xstart), writeptr);
		memwrite(&header.ystart, sizeof(header.ystart), writeptr);
		memwrite(&header.width, sizeof(header.width), writeptr);
		memwrite(&header.height, sizeof(header.height), writeptr);
		memwrite(&header.bits, sizeof(header.bits), writeptr);
		memwrite(&header.descriptor, sizeof(header.descriptor), writeptr);

		union texel
		{
			uint32_t u32;
			uint8_t u8[4];
		};
		
		texel* t = (texel*)image.m_data;
		for (int i=0; i < image.m_width*image.m_height; ++i)
		{
			texel tmp = t[i];
			swap(tmp.u8[0], tmp.u8[2]);
			memwrite(&tmp.u32, 4, writeptr);
		}
			 
		fwrite(data, writeptr-data, 1, f);
		//fclose(f); 

		delete[] data;

		return true;
	}	
	else
	{
		return false;
	}
}


bool TgaLoad(const char* filename, TgaImage& image)
{
	if (!filename)
		return false;

	FILE* aTGAFile = fopen(filename, "rb");
	if (aTGAFile == NULL)
	{
		printf("Texture: could not open %s for reading.\n", filename);
		return NULL;
	}

	char aHeaderIDLen;
	size_t err;
	err = fread(&aHeaderIDLen, sizeof(uint8_t), 1, aTGAFile);

	char aColorMapType;
	err = fread(&aColorMapType, sizeof(uint8_t), 1, aTGAFile);
	
	char anImageType;
	err = fread(&anImageType, sizeof(uint8_t), 1, aTGAFile);

	short aFirstEntryIdx;
	err = fread(&aFirstEntryIdx, sizeof(uint16_t), 1, aTGAFile);

	short aColorMapLen;
	err = fread(&aColorMapLen, sizeof(uint16_t), 1, aTGAFile);

	char aColorMapEntrySize;
	err = fread(&aColorMapEntrySize, sizeof(uint8_t), 1, aTGAFile);	

	short anXOrigin;
	err = fread(&anXOrigin, sizeof(uint16_t), 1, aTGAFile);

	short aYOrigin;
	err = fread(&aYOrigin, sizeof(uint16_t), 1, aTGAFile);

	short anImageWidth;
	err = fread(&anImageWidth, sizeof(uint16_t), 1, aTGAFile);	

	short anImageHeight;
	err = fread(&anImageHeight, sizeof(uint16_t), 1, aTGAFile);	

	char aBitCount = 32;
	err = fread(&aBitCount, sizeof(uint8_t), 1, aTGAFile);	

	char anImageDescriptor;// = 8 | (1<<5);
	err = fread((char*)&anImageDescriptor, sizeof(uint8_t), 1, aTGAFile);
	
	// supress warning
	(void)err;

	// total is the number of bytes we'll have to read
	uint8_t numComponents = aBitCount / 8;
	uint32_t numTexels = anImageWidth * anImageHeight;

	// allocate memory for image pixels
	image.m_width = anImageWidth;
	image.m_height = anImageHeight;
	image.m_data = new uint32_t[numTexels];

	// load the image pixels
	for (uint32_t i=0; i < numTexels; ++i)
	{
		union texel
		{
			uint32_t u32;
			uint8_t u8[4];
		};

		texel t;
		t.u32 = 0;

		if (!fread(&t.u32, numComponents, 1, aTGAFile))
		{
			printf("Texture: file not fully read, may be corrupt (%s)\n", filename);
		}

		// stores it as BGR(A) so we'll have to swap R and B.
		swap(t.u8[0], t.u8[2]);


		image.m_data[i] = t.u32;
	}

	// if bit 5 of the descriptor is set then the image is flipped vertically so we fix it up
	if (anImageDescriptor & (1 << 5))
	{

		// swap all the rows
		int rowSize = image.m_width*4;	

		uint8_t* buf = new uint8_t[image.m_width*4];
		uint8_t* start = (uint8_t*)image.m_data;
		uint8_t* end = &((uint8_t*)image.m_data)[rowSize*(image.m_height-1)];
		
		while (start < end)
		{
			memcpy(buf, end, rowSize);
			memcpy(end, start, rowSize);
			memcpy(start, buf, rowSize);

			start += rowSize;
			end -= rowSize;
		}

		delete[] buf;
	}

	fclose(aTGAFile);
	
	return true;
}

void TgaFree(const TgaImage& image)
{
	delete[] image.m_data;
}
