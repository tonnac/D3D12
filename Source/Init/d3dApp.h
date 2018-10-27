#pragma once
#include <Windows.h>
#include <iostream>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include "d3dx12.h"

#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")

using Microsoft::WRL::ComPtr;

#define ThrowExp(x)											\
{															\
	HRESULT __hr = (x);										\
	if(FAILED(__hr))										\
	{														\
		MessageBox(nullptr, (LPCWSTR)__FUNCTION__, L"Error", MB_OK);	\
	}														\
}

class d3dApp
{
public:
	d3dApp(HINSTANCE hInst, int Width, int Height);
	virtual ~d3dApp();
public:
	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void Init();
	bool InitMainWindow();
	void InitDirect3D();
	void FlushCommandQueue();
	void CreateCommandObject();
	void CreateSwapChain();
	void CreateRtvAndDsvDescriptorHeaps();
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;
	void OnResize();
	static d3dApp* getApp();
	void Run();
	virtual void Draw();
	ID3D12Resource* CurrentBackBuffer()const;
protected:
	HWND	  mhMainWnd;
	HINSTANCE mhAppInst;
protected:
	ComPtr<ID3D12Device> md3dDevice;
	ComPtr<IDXGIFactory4> mdxgiFactory;
	ComPtr<IDXGISwapChain> mdxgiSwapchain;

	static d3dApp* mApp;

	static const int SwapChainBufferCount = 2;
	int mCurrBackBuffer = 0;

	ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
	ComPtr<ID3D12Resource> mDepthStencilBuffer;

	ComPtr<ID3D12Fence> mFence;

	ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	ComPtr<ID3D12CommandQueue> mCommandQueue;
	ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
	ComPtr<ID3D12GraphicsCommandList> mCommandList;

	D3D12_VIEWPORT mScreenViewPort;
	D3D12_RECT mScissorRect;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	UINT mCurrentFence = 0;

	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	UINT m4xMsaaQuality = 0;
	bool m4xMsaaState = false;

	int mClientWidth;
	int mClientHeight;
};