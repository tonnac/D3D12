#include "d3dUtil.h"
#include <comdef.h>

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& fileName, int lineNumber)
	: ErrorCode(hr), FunctionName(functionName), Filename(fileName), LineNumber(lineNumber)
{}

std::wstring DxException::ToString() const
{
	_com_error err(ErrorCode);
	std::wstring msg = err.ErrorMessage();

	return FunctionName + L"failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}

