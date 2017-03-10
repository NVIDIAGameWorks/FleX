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

#include "core.h"
#include "platform.h"
#include "types.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

using namespace std;

#ifdef WIN32

#include <windows.h>
#include <commdlg.h>
#include <mmsystem.h>

double GetSeconds()
{
	static LARGE_INTEGER lastTime;
	static LARGE_INTEGER freq;
	static bool first = true;
	
	if (first)
	{	
		QueryPerformanceCounter(&lastTime);
		QueryPerformanceFrequency(&freq);

		first = false;
	}
	
	static double time = 0.0;
	
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	
	__int64 delta = t.QuadPart-lastTime.QuadPart;
	double deltaSeconds = double(delta) / double(freq.QuadPart);
	
	time += deltaSeconds;

	lastTime = t;

	return time;

}

void Sleep(double seconds)
{
	::Sleep(DWORD(seconds*1000));
}


//// helper function to get exe path
//string GetExePath()
//{
//	const uint32_t kMaxPathLength = 2048;
//
//	char exepath[kMaxPathLength];
//
//	// get exe path for file system
//	uint32_t i = GetModuleFileName(NULL, exepath, kMaxPathLength);
//
//	// rfind first slash
//	while (i && exepath[i] != '\\' && exepath[i] != '//')
//		i--;
//
//	// insert null terminater to cut off exe name
//	return string(&exepath[0], &exepath[i+1]);
//}
//
//string FileOpenDialog(char *filter)
//{
//	HWND owner=NULL;
//
//	OPENFILENAME ofn;
//	char fileName[MAX_PATH] = "";
//	ZeroMemory(&ofn, sizeof(ofn));
//	ofn.lStructSize = sizeof(OPENFILENAME);
//	ofn.hwndOwner = owner;
//	ofn.lpstrFilter = filter;
//	ofn.lpstrFile = fileName;
//	ofn.nMaxFile = MAX_PATH;
//	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
//	ofn.lpstrDefExt = "";
//
//	string fileNameStr;
//
//	if ( GetOpenFileName(&ofn) )
//		fileNameStr = fileName;
//
//	return fileNameStr;
//}
//
//bool FileMove(const char* src, const char* dest)
//{
//	BOOL b = MoveFileEx(src, dest, MOVEFILE_REPLACE_EXISTING);
//	return b == TRUE;
//}
//
//bool FileScan(const char* pattern, vector<string>& files)
//{
//	HANDLE          h;
//	WIN32_FIND_DATA info;
//
//	// build a list of files
//	h = FindFirstFile(pattern, &info);
//
//	if (h != INVALID_HANDLE_VALUE)
//	{
//		do
//		{
//			if (!(strcmp(info.cFileName, ".") == 0 || strcmp(info.cFileName, "..") == 0))
//			{
//				files.push_back(info.cFileName);
//			}
//		} 
//		while (FindNextFile(h, &info));
//
//		if (GetLastError() != ERROR_NO_MORE_FILES)
//		{
//			return false;
//		}
//
//		FindClose(h);
//	}
//	else
//	{
//		return false;
//	}
//
//	return true;
//}


#else

// linux, mac platforms
#include <sys/time.h>

double GetSeconds()
{
	// Figure out time elapsed since last call to idle function
	static struct timeval last_idle_time;
	static double time = 0.0;	

	struct timeval time_now;
	gettimeofday(&time_now, NULL);

	if (last_idle_time.tv_usec == 0)
		last_idle_time = time_now;

	float dt = (float)(time_now.tv_sec - last_idle_time.tv_sec) + 1.0e-6*(time_now.tv_usec - last_idle_time.tv_usec);

	time += dt;
	last_idle_time = time_now;

	return time;
}

#endif


uint8_t* LoadFileToBuffer(const char* filename, uint32_t* sizeRead)
{
	FILE* file = fopen(filename, "rb");
	if (file)
	{
		fseek(file, 0, SEEK_END);
		long p = ftell(file);
		
		uint8_t* buf = new uint8_t[p];
		fseek(file, 0, SEEK_SET);
		uint32_t len = uint32_t(fread(buf, 1, p, file));
	
		fclose(file);
		
		if (sizeRead)
		{
			*sizeRead = len;
		}
		
		return buf;
	}
	else
	{
		std::cout << "Could not open file for reading: " << filename << std::endl;
		return NULL;
	}
}

string LoadFileToString(const char* filename)
{
	//std::ifstream file(filename);
	//return string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	uint32_t size;
	uint8_t* buf = LoadFileToBuffer(filename, &size);
	
	if (buf)
	{
		string s(buf, buf+size);
		delete[] buf;
		
		return s;
	}
	else
	{
		return "";
	}	
}

bool SaveStringToFile(const char* filename, const char* s)
{
	FILE* f = fopen(filename, "w");
	if (!f)
	{
		std::cout << "Could not open file for writing: " << filename << std::endl;		
		return false;
	}
	else 
	{
		fputs(s, f);
		fclose(f);
		
		return true;
	}
}


string StripFilename(const char* path)
{
	// simply find the last 
	const char* iter=path;
	const char* last=NULL;
	while (*iter)
	{
		if (*iter == '\\' || *iter== '/')
			last = iter;
		
		++iter;
	}
	
	if (last)
	{
		return string(path, last+1);
	}
	else
		return string();
	
}

string GetExtension(const char* path)
{
	const char* s = strrchr(path, '.');
	if (s)
	{
		return string(s+1);
	}
	else
	{
		return "";
	}
}

string StripExtension(const char* path)
{
	const char* s = strrchr(path, '.');
	if (s)
	{
		return string(path, s);
	}
	else
	{
		return string(path);
	}
}

string NormalizePath(const char* path)
{
	string p(path);
	replace(p.begin(), p.end(), '\\', '/');
	transform(p.begin(), p.end(), p.begin(), ::tolower);
	
	return p;
}

// strips the path from a file name
string StripPath(const char* path)
{
	// simply find the last 
	const char* iter=path;
	const char* last=NULL;
	while (*iter)
	{
		if (*iter == '\\' || *iter== '/')
			last = iter;
		
		++iter;
	}
	
	if (!last)
	{
		return string(path);
	}
	
	// eat the last slash
	++last;
	
	if (*last)
	{
		return string(last);
	}
	else
	{
		return string();	
	}
}

