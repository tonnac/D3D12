#include "d3dApp.h"
#include "MathHelper.h"
#include "UploadBuffer.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

struct ObjectConstatns
{
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
	XMFLOAT3 EyePos;
	float Height;
	XMFLOAT3 pad;
	float fLength;
};

enum class RenderLayer : int
{
	Solid = 0,
	Wireframe,
	Count
};

class BoxApp : public D3DApp
{
public:
	BoxApp(HINSTANCE hInstance);
	BoxApp(const BoxApp& rhs) = delete;
	BoxApp& operator=(const BoxApp& rhs) = delete;
	~BoxApp();

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& gt) override;
	virtual void Draw(const GameTimer& gt) override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildicosahedronGeometry();
	void BuildPSO();

private:
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	std::unique_ptr<UploadBuffer<ObjectConstatns>> mObjectCB = nullptr;

	std::unique_ptr<MeshGeometry> mIcoGeo = nullptr;

	ComPtr<ID3DBlob> mvsByteCode = nullptr;
	ComPtr<ID3DBlob> mpsByteCode = nullptr;
	ComPtr<ID3DBlob> mgsByteCode = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	std::array<ComPtr<ID3D12PipelineState>, (int)RenderLayer::Count> mPSO;

	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 35.0f;

	POINT mLastMousePos;

	bool isWireFrame = false;
	XMFLOAT3 mCenterPos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		BoxApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

BoxApp::BoxApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{

}
BoxApp::~BoxApp()
{}


bool BoxApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildDescriptorHeaps();
	BuildConstantBuffers();
	BuildRootSignature();
	BuildicosahedronGeometry();
	BuildShadersAndInputLayout();
	BuildPSO();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists((UINT)std::size(cmdLists), cmdLists);

	FlushCommandQueue();
	return true;
}
void BoxApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f* MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, p);
}
void BoxApp::Update(const GameTimer& gt)
{
	if (GetAsyncKeyState('1') & 0x8000)
	{
		isWireFrame = true;
	}
	else
		isWireFrame = false;

	float x = mRadius * sinf(mPhi)*cosf(mTheta);
	float z = mRadius * sinf(mPhi)*sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
	
	XMMATRIX scale = XMMatrixScaling(3.0f, 3.0f, 3.0f);

	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	world *= scale;
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;

	float Height = 1.5f; //* (cosf(gt.TotalTime()) * 0.5f + 0.5f);

	XMFLOAT2 len;
	XMVECTOR plen = XMVector3Length(pos - XMLoadFloat3(&mCenterPos));
	XMStoreFloat2(&len, plen);

	ObjectConstatns objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	objConstants.Height = Height;
	XMStoreFloat3(&objConstants.EyePos, pos);
	objConstants.fLength = len.x;
	mObjectCB->CopyData(0, objConstants);
}

void BoxApp::Draw(const GameTimer& gt)
{
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	if (!isWireFrame)
	{
		ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO[(int)RenderLayer::Solid].Get()));
	}
	else
	{
		ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO[(int)RenderLayer::Wireframe].Get()));
	}

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps((UINT)std::size(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	mCommandList->IASetVertexBuffers(0, 1, &mIcoGeo->VertexBufferView());
	mCommandList->IASetIndexBuffer(&mIcoGeo->IndexBufferView());
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

	mCommandList->DrawIndexedInstanced(mIcoGeo->DrawArgs["icosahedron"].IndexCount, 1, 0, 0, 0);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists((UINT)std::size(cmdsLists), cmdsLists);

	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	FlushCommandQueue();
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}
void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}
void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * CASTING(float, x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * CASTING(float, y - mLastMousePos.y));

		mTheta += dx;
		mPhi += dy;

		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		float dx = 0.005f * CASTING(float, x - mLastMousePos.x);
		float dy = 0.005f * CASTING(float, y - mLastMousePos.y);

		mRadius += dx - dy;

		mRadius = MathHelper::Clamp(mRadius, 6.0f, 50.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void BoxApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;

	md3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap));
}
void BoxApp::BuildConstantBuffers()
{
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstatns>>(md3dDevice.Get(), 1, true);

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstatns));

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
	int boxCBufIndex = 0;
	cbAddress += boxCBufIndex * objCBByteSize;
	
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstatns));

	md3dDevice->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}
void BoxApp::BuildRootSignature()
{
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
}
void BoxApp::BuildShadersAndInputLayout()
{
	HRESULT hr = S_OK;

	mvsByteCode = d3dUtil::CompileShader(L"PixelShader.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = d3dUtil::CompileShader(L"PixelShader.hlsl", nullptr, "PS", "ps_5_0");
	mgsByteCode = d3dUtil::CompileShader(L"PixelShader.hlsl", nullptr, "GS", "gs_5_0");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void BoxApp::BuildicosahedronGeometry()
{
	constexpr float X = 0.525731f;
	constexpr float Z = 0.850651f;

	std::array<Vertex, 12> vertices;
	std::array<std::uint16_t, 60> indicies;

	XMFLOAT3 pos[12] =
	{
		XMFLOAT3(-X, 0.0f, Z),  XMFLOAT3(X, 0.0f, Z),
		XMFLOAT3(-X, 0.0f, -Z), XMFLOAT3(X, 0.0f, -Z),
		XMFLOAT3(0.0f, Z, X),   XMFLOAT3(0.0f, Z, -X),
		XMFLOAT3(0.0f, -Z, X),  XMFLOAT3(0.0f, -Z, -X),
		XMFLOAT3(Z, X, 0.0f),   XMFLOAT3(-Z, X, 0.0f),
		XMFLOAT3(Z, -X, 0.0f),  XMFLOAT3(-Z, -X, 0.0f)
	};

	for (size_t i = 0; i < vertices.size(); ++i)
	{
		vertices[i].Pos = pos[i];
		if (i % 2 == 0)
			vertices[i].Color = XMFLOAT4(Colors::White);
		else
			vertices[i].Color = XMFLOAT4(Colors::Black);
	}

	XMVECTOR center = XMVectorZero();
	for (size_t i = 0; i < vertices.size(); ++i)
	{
		center += XMLoadFloat3(&vertices[i].Pos);
	}
	center /= (float)vertices.size();
	XMStoreFloat3(&mCenterPos, center);

	std::uint16_t k[60] =
	{
		1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
		1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
		3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
		10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
	};

	for (size_t i = 0; i < indicies.size(); ++i)
	{
		indicies[i] = k[i];
	}


	const UINT vbByteSize = sizeof(Vertex) * (UINT)vertices.size();
	const UINT ibByteSize = sizeof(std::uint16_t) * (UINT)indicies.size();

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "icosahedron";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, geo->VertexBufferCPU.GetAddressOf()));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, geo->IndexBufferCPU.GetAddressOf()));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indicies.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), indicies.data(), ibByteSize, geo->IndexBufferUploader);

	geo->IndexBufferByteSize = ibByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->VertexBufferByteSize = vbByteSize;
	geo->VertexByteStride = sizeof(Vertex);

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indicies.size();
	submesh.BaseVertexLocation = 0;
	submesh.StartIndexLocation = 0;

	geo->DrawArgs["icosahedron"] = submesh;
	mIcoGeo = std::move(geo);
}

void BoxApp::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS =
	{
		RE_CAST(BYTE*,mvsByteCode->GetBufferPointer()),
		mvsByteCode->GetBufferSize()
	};
	psoDesc.PS =
	{
		RE_CAST(BYTE*, mpsByteCode->GetBufferPointer()),
		mpsByteCode->GetBufferSize()
	};
	psoDesc.GS =
	{
		RE_CAST(BYTE*, mgsByteCode->GetBufferPointer()),
		mgsByteCode->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO[(int)RenderLayer::Solid])));

	auto wireframePSo = psoDesc;
	wireframePSo.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&wireframePSo, IID_PPV_ARGS(&mPSO[(int)RenderLayer::Wireframe])));
}