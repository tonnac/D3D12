#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "d3dUtil.h"
#include "GameTimer.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

class D3DApp
{
protected:
	D3DApp(HINSTANCE hInstance, LONG Width, LONG Height);
	D3DApp(const D3DApp& rhs) = delete;
	D3DApp& operator=(const D3DApp& rhs) = delete;
	virtual ~D3DApp();

public:
	static D3DApp* getApp();

	HINSTANCE AppInst() const;
	HWND	  MainWnd() const;
	float	  AspectRatio() const;

	bool	  Get4xMsaaState() const;
	void	  Set4xMsaaState(const bool& value);

	int		  Run();

	virtual	bool initialize();
	virtual LRESULT MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	virtual void CreateRtvAndDsvDescriptorHeaps();
	virtual void OnResize();
	virtual void Update(const GameTimer& timer) = 0;
	virtual void Draw(const GameTimer& timer) = 0;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) {}
	virtual void OnMouseUp(WPARAM btnState, int x, int y) {}
	virtual void OnMouseMove(WPARAM btnState, int x, int y) {}

protected:

	bool InitMainWindow();
	bool InitDirect3D();

	void CreateSwapChain();
	void CreateCommandObjects();

	void FlushCommandQueue();

	ID3D12Resource* CurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

	void CalculateFrameStats();

	void LogAdapters();
	void LogAdapterOutput(IDXGIAdapter* adapter);
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

protected:

	static D3DApp * mApp;

	HINSTANCE mhAppInst = nullptr;
	HWND	  mhMainWnd = nullptr;
	bool	  mAppPaused = false;
	bool	  mMinimized = false;
	bool	  mMaximized = false;
	bool	  mResizing = false;

	bool	  m4xMsaaState = false;
	UINT	  m4xMsaaQuality = 0;

	Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;
	Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence = 0;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	GameTimer mTimer;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapchain;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT	   mScissorRect;

	static const int SwapChainBufferCount = 2;
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapchainBuffer[SwapChainBufferCount];
	int mCurrBackBuffer = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	std::wstring mMainWndCaption = L"D3D App";
	LONG mClientWidth;
	LONG mClientHeight;
};