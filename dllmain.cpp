// NFSC Shader Compiler & Loader
// Compiles and/or loads shaders outside the executable
// Put HLSL effect .fx files in the fx folder in the game directory
// Or put compiled shader objects with resource names next to executable

#include "stdafx.h"
#include "stdio.h"
#include <windows.h>
#include "includes\injector\injector.hpp"
#include <D3D9.h>
#include <d3dx9effect.h>

unsigned int CurrentShaderNum;
ID3DXEffectCompiler* pEffectCompiler;
ID3DXBuffer* pBuffer, *pEffectBuffer;
char FilenameBuf[2048];

char* ErrorString;

bool bConsoleExists(void)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
		return false;

	return true;
}

// custom print methods which include console attachment checks...
int __cdecl cusprintf(const char* Format, ...)
{
	va_list ArgList;
	int Result = 0;

	if (bConsoleExists())
	{
		__crt_va_start(ArgList, Format);
		Result = vprintf(Format, ArgList);
		__crt_va_end(ArgList);
	}

	return Result;
}

int __cdecl cus_puts(char* buf)
{
	if (bConsoleExists())
		return puts(buf);
	return 0;
}

bool CheckIfFileExists(const char* FileName)
{
	FILE *fin = fopen(FileName, "rb");
	if (fin == NULL)
	{
		char secondarypath[MAX_PATH];
		strcpy(secondarypath, "fx\\");
		strcat(secondarypath, FileName);
		fin = fopen(secondarypath, "rb");
		if (fin == NULL)
			return 0;
		strcpy((char*)FileName, secondarypath);
	}
	fclose(fin);
	return 1;
}

bool WriteFileFromMemory(const char* FileName, const void* buffer, long size)
{
	FILE *fout = fopen(FileName, "wb");
	if (fout == NULL)
		return 0;

	fwrite(buffer, 1, size, fout);

	fclose(fout);
	return 1;
}

HRESULT __stdcall D3DXCreateEffectFromResourceHook(LPDIRECT3DDEVICE9 pDevice,
	HMODULE           hSrcModule,
	LPCTSTR           pSrcResource,
	const D3DXMACRO         *pDefines,
	LPD3DXINCLUDE     pInclude,
	DWORD             Flags,
	LPD3DXEFFECTPOOL  pPool,
	LPD3DXEFFECT      *ppEffect,
	LPD3DXBUFFER      *ppCompilationErrors
)
{
	_asm mov CurrentShaderNum, edx
	char* LastUnderline;
	char* FxFilePath;
	//unsigned int CurrentEffectSize = 0;
	_asm mov eax, CurrentShaderNum
	_asm mov eax, ds:0x00A6408C[eax]
	_asm mov FxFilePath, eax
	strcpy(FilenameBuf, FxFilePath);

	LastUnderline = strrchr(FilenameBuf, '.');
	LastUnderline[1] = 'f';
	LastUnderline[2] = 'x';
	LastUnderline[3] = '\0';

	if (CheckIfFileExists(FilenameBuf))
	{
		HRESULT result;
		//D3DXCreateBuffer(2048, &pBuffer);
		result = D3DXCreateEffectCompilerFromFile(FilenameBuf, NULL, NULL, 0, &pEffectCompiler, &pBuffer);
		if (SUCCEEDED(result))
		{
			cusprintf("Compiling shader %s\n", FilenameBuf);
			result = pEffectCompiler->CompileEffect(0, &pEffectBuffer, &pBuffer);
			if (!SUCCEEDED(result))
			{
				cus_puts(*(char**)(pBuffer + 3));
				cusprintf("HRESULT: %X\n", result);
				return result;
			}
			cusprintf("Compilation successful!\n");

			// we better keep it in memory instead of writing to disk...

			//ppEffect = *(LPD3DXEFFECT*)(pEffectBuffer + 3);
			//CurrentEffectSize = *(unsigned int*)(pEffectBuffer + 2);
			//WriteFileFromMemory("temp_shader.cso", *(void**)(pEffectBuffer + 3), CurrentEffectSize);
			//result = D3DXCreateEffectFromFile(pDevice, "temp_shader.cso", pDefines, pInclude, Flags, pPool, ppEffect, &pBuffer);

			result = D3DXCreateEffect(pDevice, *(void**)(pEffectBuffer + 3), *(unsigned int*)(pEffectBuffer + 2), pDefines, pInclude, Flags, pPool, ppEffect, &pBuffer);
			if (!SUCCEEDED(result))
			{
				cusprintf("Effect creation failed: HRESULT: %X\n", result);
				cus_puts(*(char**)(pBuffer + 3));
			}
			//remove("temp_shader.cso");
			return result;
		}
		else
		{
			cusprintf("Error compiling shader: HRESULT: %X\n", result);
			cus_puts(*(char**)(pBuffer + 3));
		}
	}

	if (CheckIfFileExists(pSrcResource))
	{
		return D3DXCreateEffectFromFile(pDevice, pSrcResource, pDefines, pInclude, Flags, pPool, ppEffect, ppCompilationErrors);
	}


	return D3DXCreateEffectFromResource(pDevice, hSrcModule, pSrcResource, pDefines, pInclude, Flags, pPool, ppEffect, ppCompilationErrors);
}

int Init()
{
	injector::MakeCALL(0x0072B6D4, D3DXCreateEffectFromResourceHook, true);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		if (bConsoleExists())
		{
			freopen("CON", "w", stdout);
			freopen("CON", "w", stderr);
		}
		Init();
	}
	return TRUE;
}

