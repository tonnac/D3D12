#include "d3dApp.h"
#include <stdlib.h>

d3dApp* d3dApp::mApp = nullptr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return d3dApp::getApp()->MsgProc(hwnd, msg, wParam, lParam);
}

d3dApp::d3dApp(HINSTANCE hInst, int Width, int Height) : mhAppInst(hInst), mClientWidth(Width), mClientHeight(Height)
{
	if (mApp == nullptr)
	{
		mApp = this;
	}
}
d3dApp::~d3dApp()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

LRESULT d3dApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
void d3dApp::Init()
{
	InitMainWindow();
	InitDirect3D();
}
bool d3dApp::InitMainWindow()
{
	WNDCLASS wd;
	ZeroMemory(&wd, sizeof(wd));
	wd.hInstance = mhAppInst;
	wd.lpfnWndProc = WndProc;
	wd.style = CS_HREDRAW | CS_VREDRAW;
	wd.lpszClassName = L"MainWnd";

	if (!RegisterClass(&wd))
	{
		return false;
	}

	RECT R = { 0, 0, mClientWidth, mClientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	mhMainWnd = CreateWindow(wd.lpszClassName, L"Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		width, height, 0, 0, mhAppInst, 0);

	if (mhMainWnd == nullptr)
	{
		return false;
	}
	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);
	return true;
}
void d3dApp::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG) 
	// Enable the D3D12 debug layer.
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowExp(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif
	ThrowExp(CreateDXGIFactory(IID_PPV_ARGS(&mdxgiFactory)));

	HRESULT hardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&md3dDevice));

	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> WarpAdapter;
		ThrowExp(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(WarpAdapter.GetAddressOf())));

		ThrowExp(D3D12CreateDevice(WarpAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&md3dDevice)));
	}

	ThrowExp(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowExp(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)));

	CreateCommandObject();
	CreateSwapChain();
	CreateRtvAndDsvDescriptorHeaps();
}
void d3dApp::FlushCommandQueue()
{
	++mCurrentFence;

	ThrowExp(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	if (mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		ThrowExp(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}
void d3dApp::CreateCommandObject()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;


	ThrowExp(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
	ThrowExp(md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mDirectCmdListAlloc)));
	ThrowExp(md3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mDirectCmdListAlloc.Get(), nullptr, IID_PPV_ARGS(&mCommandList)));
	mCommandList->Close();
}
void d3dApp::CreateSwapChain()
{
	mdxgiSwapchain.Reset();

	DXGI_SWAP_CHAIN_DESC sd;
	DXGI_MODE_DESC bd;
	bd.Width = mClientWidth;
	bd.Height = mClientHeight;
	bd.RefreshRate.Numerator = 60;
	bd.RefreshRate.Denominator = 1;
	bd.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	bd.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	bd.Format = mBackBufferFormat;
	sd.BufferDesc = bd;
	sd.SampleDesc.Count = (m4xMsaaState) ? 4 : 1;
	sd.SampleDesc.Quality = (m4xMsaaState) ? (m4xMsaaQuality - 1) : 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SwapChainBufferCount;
	sd.OutputWindow = mhMainWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	mdxgiFactory->CreateSwapChain(mCommandQueue.Get(), &sd, mdxgiSwapchain.GetAddressOf());
}
void d3dApp::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	rtvHeapDesc.NodeMask = 0;
	ThrowExp(md3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));


	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.NodeMask = 0;
	md3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf()));
}
D3D12_CPU_DESCRIPTOR_HANDLE d3dApp::CurrentBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mCurrBackBuffer, mRtvDescriptorSize);
}
D3D12_CPU_DESCRIPTOR_HANDLE d3dApp::DepthStencilView() const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}
void d3dApp::OnResize()
{
	FlushCommandQueue();

	ThrowExp(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	for (int i = 0; i < SwapChainBufferCount; ++i)
	{
		mSwapChainBuffer[i].Reset();
	}
	mDepthStencilBuffer.Reset();

	ThrowExp(mdxgiSwapchain->ResizeBuffers(SwapChainBufferCount, mClientWidth, mClientHeight, mBackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrBackBuffer = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; ++i)
	{
		ThrowExp(mdxgiSwapchain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

	D3D12_RESOURCE_DESC depthStencilDesc;

	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mClientWidth;
	depthStencilDesc.Height = mClientHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = mDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	ThrowExp(md3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_COMMON,
		&optClear, IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, DepthStencilView());
	
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	ThrowExp(mCommandList->Close());
	ID3D12CommandList * cmdLists[] = { mCommandList.Get() };

	mCommandQueue->ExecuteCommandLists((UINT)std::size(cmdLists), cmdLists);
	
	FlushCommandQueue();

	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<FLOAT>(mClientWidth);
	mScreenViewport.Height = static_cast<FLOAT>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;
	
	mScissorRect = { 0, 0, mClientWidth, mClientHeight };
}

d3dApp* d3dApp::getApp()
{
	return mApp;
}
void d3dApp::Run()
{
	MSG Msg = { 0, };

	while (Msg.message != WM_QUIT)
	{
		if (PeekMessage(&Msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
		else
		{
			Draw();
		}
	}
}
void d3dApp::Draw()
{
	return;
}
ID3D12Resource* d3dApp::CurrentBackBuffer()const
{
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}