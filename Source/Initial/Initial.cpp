#include "d3dApp.h"
#include <DirectXColors.h>

using namespace DirectX;

class InitDirect3DApp : public D3DApp
{
public:
	InitDirect3DApp(HINSTANCE hInstance, LONG Width, LONG Height);
	~InitDirect3DApp();

	virtual bool initialize() override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR szCmdLine, int nCmdShow)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		InitDirect3DApp d3dapp(hInstance, 800, 600);
		if (!d3dapp.initialize())
			return 0;

		return d3dapp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
	}

	return 0;
}

InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance, LONG Width, LONG Height) : 
	D3DApp(hInstance, Width, Height)
{
}

InitDirect3DApp::~InitDirect3DApp()
{
}

bool InitDirect3DApp::initialize()
{
	if (!D3DApp::initialize())
		return false;

	return true;
}

void InitDirect3DApp::OnResize()
{
	D3DApp::OnResize();
}

void InitDirect3DApp::Update(const GameTimer& gt)
{
	return;
}

void InitDirect3DApp::Draw(const GameTimer& gt)
{
	ThrowIfFailed(mCmdListAlloc->Reset());

	ThrowIfFailed(mCommandList->Reset(mCmdListAlloc.Get(), nullptr));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->RSSetScissorRects(1, &mScissorRect);
	mCommandList->RSSetViewports(1, &mScreenViewPort);

	mCommandList->ClearRenderTargetView(
		CurrentBackBufferView(),
		Colors::DarkTurquoise, 0, nullptr);
	mCommandList->ClearDepthStencilView(
		DepthStencilView(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList * cmdLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists((UINT)std::size(cmdLists), cmdLists);

	ThrowIfFailed(mSwapchain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	FlushCommandQueue();
}