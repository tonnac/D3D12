#pragma once
#include <windows.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include <string>
#include <cassert>
#include <wrl.h>
#include <vector>

#define CASTING(x,y) static_cast<x>((y))
#define RE_CASTING(x,y) reinterpret_cast<x>((y))

inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

class DxException
{
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& fileName, int lineNumber);

	std::wstring ToString() const;

	HRESULT ErrorCode = S_OK;
	std::wstring FunctionName;
	std::wstring Filename;
	int LineNumber = -1;
};


#ifndef ThrowifFailed
#define ThrowifFailed(x)												\
{																		\
	HRESULT hr__ = (x);													\
	std::wstring wfn = AnsiToWString(__FILE__);							\
	if(FAILED(hr__)) {throw DxException(hr__, L#x, wfn, __LINE__); }	\
}		
#endif

#ifndef ReleaseCom
#define ReleaseCom(x) {if(x){ x->Release(); x = nullptr;} }
#endif