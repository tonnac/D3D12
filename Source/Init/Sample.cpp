#include "d3dApp.h"
#include <DirectXColors.h>

class Sample : public d3dApp
{
public:
	Sample(HINSTANCE hInst, int Width, int Height) : d3dApp(hInst, Width, Height)
	{}
public:
	void Draw() override;
};


int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE prevInst, LPWSTR szCmdLine, int nCmdShow)
{
	Sample sd(hInst, 800, 600);
	sd.Init();
	sd.OnResize();
	sd.Run();
	
	return 0;
}


void Sample::Draw()
{
	ThrowExp(mDirectCmdListAlloc->Reset());

	ThrowExp(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::MidnightBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowExp(mCommandList->Close());

	ID3D12CommandList * cmdLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists((UINT)std::size(cmdLists), cmdLists);

	ThrowExp(mdxgiSwapchain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	FlushCommandQueue();
}