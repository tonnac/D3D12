#include "d3dApp.h"
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "FrameResource.h"
#include "GeometryGenerator.h"
#include "Waves.h"


using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

const int gNumFrameResources = 3;


struct RenderItem
{
	RenderItem() = default;

	XMFLOAT4X4 World = MathHelper::Identity4x4();

	int NumFramesDirty = gNumFrameResources;

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	UINT ObjCbIndex = -1;

	Material * Mat = nullptr;
	MeshGeometry* Geo = nullptr;

	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation  = 0;
};

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	AlphaTestedTreeSprites,
	Count
};

class TreeBillboardsApp : public D3DApp
{
public:
	TreeBillboardsApp(HINSTANCE hInstance);
	TreeBillboardsApp(const TreeBillboardsApp& rhs) = delete;
	TreeBillboardsApp& operator=(const TreeBillboardsApp& rhs) = delete;
	~TreeBillboardsApp();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateWaves(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);

	void LoadTexture();
	void BuildDescriptorHeap();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildLandGeometry();
	void BuildWavesGeometryBuffers();
	void BuildTreeSpritesGeometry();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	float GetHillsHeight(float x, float z) const;
	XMFLOAT3 GetHillsNormal(float x, float z) const;

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
private:

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	UINT mCbvSrvDescriptorSize = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	ComPtr<ID3D12DescriptorHeap> mSrvHeap;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mTreeSpriteInputLayout;

	RenderItem * mWavesRitem = nullptr;

	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

	std::unique_ptr<Waves> mWaves;

	PassConstants mMainPassCB;

	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.5f*XM_PI;
	float mPhi = XM_PIDIV2 - 0.1f;
	float mRadius = 50.0f;

	POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdList, int showCmd)
{

#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		TreeBillboardsApp theApp(hInstance);
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

TreeBillboardsApp::TreeBillboardsApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
}
TreeBillboardsApp::~TreeBillboardsApp()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

bool TreeBillboardsApp::Initialize()
{	 
	if (!D3DApp::Initialize())
		return false;

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mWaves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

	LoadTexture();
	BuildRootSignature();
	BuildDescriptorHeap();
	BuildShadersAndInputLayout();
	BuildLandGeometry();
	BuildWavesGeometryBuffers();
	BuildTreeSpritesGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists((UINT)std::size(cmdLists), cmdLists);

	FlushCommandQueue();

	return true;
}	 
	 
void TreeBillboardsApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void TreeBillboardsApp::LoadTexture()
{
	auto waterTex = std::make_unique<Texture>();
	waterTex->Name = "waterTex";
	waterTex->Filename = L"../../Textures/water1.dds";
	DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(), mCommandList.Get(), waterTex->Filename.c_str(),
		waterTex->Resource, waterTex->UploadHeap);

	auto grassTex = std::make_unique<Texture>();
	grassTex->Name = "grassTex";
	grassTex->Filename = L"../../Textures/grass.dds";
	DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(), mCommandList.Get(), grassTex->Filename.c_str(),
		grassTex->Resource, grassTex->UploadHeap);

	auto boxTex = std::make_unique<Texture>();
	boxTex->Name = "boxTex";
	boxTex->Filename = L"../../Textures/WireFence.dds";
	DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(), mCommandList.Get(), boxTex->Filename.c_str(),
		boxTex->Resource, boxTex->UploadHeap);

	auto treeArrayTex = std::make_unique<Texture>();
	treeArrayTex->Name = "treeArrayTex";
	treeArrayTex->Filename = L"../../Textures/treeArray2.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(), mCommandList.Get(), treeArrayTex->Filename.c_str(),
		treeArrayTex->Resource, treeArrayTex->UploadHeap));


	mTextures["grassTex"] = std::move(grassTex);
	mTextures["waterTex"] = std::move(waterTex);
	mTextures["boxTex"] = std::move(boxTex);
	mTextures[treeArrayTex->Name] = std::move(treeArrayTex);
}

void TreeBillboardsApp::BuildDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 4;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvHeap->GetCPUDescriptorHandleForHeapStart());

	auto grassTex = mTextures["grassTex"]->Resource;
	auto waterTex = mTextures["waterTex"]->Resource;
	auto fenceTex = mTextures["boxTex"]->Resource;
	auto treeArrayTex = mTextures["treeArrayTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = grassTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	md3dDevice->CreateShaderResourceView(grassTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = waterTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(waterTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = fenceTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(fenceTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	auto desc = treeArrayTex->GetDesc();
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Format = treeArrayTex->GetDesc().Format;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = -1;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	srvDesc.Texture2DArray.ArraySize = treeArrayTex->GetDesc().DepthOrArraySize;
	md3dDevice->CreateShaderResourceView(treeArrayTex.Get(), &srvDesc, hDescriptor);
}

void TreeBillboardsApp::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);
	UpdateCamera(gt);

	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
	UpdateWaves(gt);
	AnimateMaterials(gt);
}

void TreeBillboardsApp::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	ThrowIfFailed(cmdListAlloc->Reset());

	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

	mCommandList->RSSetViewports(1, &mScreenViewPort);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	ID3D12DescriptorHeap * descriptorHeaps[] = { mSrvHeap.Get() };
	mCommandList->SetDescriptorHeaps((UINT)std::size(descriptorHeaps), descriptorHeaps);

	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTested]);

	mCommandList->SetPipelineState(mPSOs["treeSprites"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites]);

	mCommandList->SetPipelineState(mPSOs["transparent"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Transparent]);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList * cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists((UINT)std::size(cmdsLists), cmdsLists);

	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	mCurrFrameResource->Fence = ++mCurrentFence;

	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void TreeBillboardsApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void TreeBillboardsApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void TreeBillboardsApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		mTheta += dx;
		mPhi += dy;

		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		float dx = 0.05f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.05f * static_cast<float>(y - mLastMousePos.y);

		mRadius += dx - dy;

		mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void TreeBillboardsApp::OnKeyboardInput(const GameTimer& gt)
{
}

void TreeBillboardsApp::UpdateCamera(const GameTimer& gt)
{
	mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
	mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
	mEyePos.y = mRadius * cosf(mPhi);

	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
}

void TreeBillboardsApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(e->ObjCbIndex, objConstants);

			e->NumFramesDirty--;
		}
	}
}

void TreeBillboardsApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mMaterials)
	{
		Material * mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;

			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			mat->NumFramesDirty--;
		}
	}
}

void TreeBillboardsApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();

	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void TreeBillboardsApp::UpdateWaves(const GameTimer& gt)
{
	static float t_base = 0.0f;
	if ((mTimer.TotalTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;

		int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
		int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);

		float r = MathHelper::RandF(0.2f, 0.5f);

		mWaves->Disturb(i, j, r);
	}

	mWaves->Update(gt.DeltaTime());

	auto currWavesVB = mCurrFrameResource->WavesVB.get();
	for (int i = 0; i < mWaves->VertexCount(); ++i)
	{
		Vertex v;

		v.Pos = mWaves->Position(i);
		v.Normal = mWaves->Normal(i);

		v.TexC.x = 0.5f + v.Pos.x / mWaves->Width();
		v.TexC.y = 0.5f - v.Pos.z / mWaves->Depth();

		currWavesVB->CopyData(i, v);
	}
	mWavesRitem->Geo->VertexBufferGPU = mCurrFrameResource->WavesVB.get()->Resource();
}

void TreeBillboardsApp::AnimateMaterials(const GameTimer& gt)
{
	auto waterMat = mMaterials["water"].get();

	float& tu = waterMat->MatTransform(3, 0);
	float& tv = waterMat->MatTransform(3, 1);

	tu += 0.1f * gt.DeltaTime();
	tv += 0.02f * gt.DeltaTime();

	if (tu >= 1.0f)
		tu -= 1.0f;

	if (tv >= 1.0f)
		tv -= 1.0f;

	waterMat->MatTransform(3, 0) = tu;
	waterMat->MatTransform(3, 1) = tv;

	waterMat->NumFramesDirty = gNumFrameResources;
}

void TreeBillboardsApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void TreeBillboardsApp::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Default.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Default.hlsl", defines, "PS", "ps_5_0");
	mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Default.hlsl", alphaTestDefines, "PS", "ps_5_0");

	mShaders["treeSpriteVS"] = d3dUtil::CompileShader(L"TreeSprite.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["treeSpriteGS"] = d3dUtil::CompileShader(L"TreeSprite.hlsl", nullptr, "GS", "gs_5_0");
	mShaders["treeSpritePS"] = d3dUtil::CompileShader(L"TreeSprite.hlsl", alphaTestDefines, "PS", "ps_5_0");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	mTreeSpriteInputLayout = 
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
}

void TreeBillboardsApp::BuildLandGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);

	std::vector<Vertex> vertices(grid.Vertices.size());
	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		auto& p = grid.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
		vertices[i].Normal = GetHillsNormal(p.x, p.z);
		vertices[i].TexC = grid.Vertices[i].TexC;
	}
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	
	std::vector<std::uint16_t> indices = grid.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "landGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;
	
	mGeometries["landGeo"] = std::move(geo);

	GeometryGenerator::MeshData box = geoGen.CreateBox(8.0f, 8.0f, 8.0f, 3);
	std::vector<Vertex> boxVertices(box.Vertices.size());

	for (size_t i = 0; i < box.Vertices.size(); ++i)
	{
		boxVertices[i].Pos = box.Vertices[i].Position;
		boxVertices[i].Normal = box.Vertices[i].Normal;
		boxVertices[i].TexC = box.Vertices[i].TexC;
	}

	const UINT vbByteSize1 = sizeof(Vertex) * (UINT)boxVertices.size();

	std::vector<std::uint16_t> boxIndices = box.GetIndices16();
	const UINT ibByteSize1 = (UINT)boxIndices.size() * sizeof(std::uint16_t);

	auto boX = std::make_unique<MeshGeometry>();
	boX->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, boX->VertexBufferCPU.GetAddressOf()));
	CopyMemory(boX->VertexBufferCPU->GetBufferPointer(), boxVertices.data(), vbByteSize1);
	ThrowIfFailed(D3DCreateBlob(vbByteSize, boX->IndexBufferCPU.GetAddressOf()));
	CopyMemory(boX->IndexBufferCPU->GetBufferPointer(), boxIndices.data(), ibByteSize1);

	boX->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), boxVertices.data(), vbByteSize1, boX->VertexBufferUploader);

	boX->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), boxIndices.data(), ibByteSize1, boX->IndexBufferUploader);

	SubmeshGeometry submesh1;
	submesh1.IndexCount = (UINT)boxIndices.size();
	submesh1.StartIndexLocation = 0;
	submesh1.BaseVertexLocation = 0;

	boX->IndexBufferByteSize = ibByteSize1;
	boX->VertexBufferByteSize = vbByteSize1;
	boX->VertexByteStride = sizeof(Vertex);
	boX->IndexFormat = DXGI_FORMAT_R16_UINT;

	boX->DrawArgs["box"] = submesh1;

	mGeometries["boxGeo"] = std::move(boX);
}

void TreeBillboardsApp::BuildWavesGeometryBuffers()
{
	std::vector<std::uint16_t> indices(3 * mWaves->TriangleCount()); // 3 indices per face
	assert(mWaves->VertexCount() < 0x0000ffff);

	int m = mWaves->RowCount();
	int n = mWaves->ColumnCount();
	int k = 0;
	for (int i = 0; i < m - 1; ++i)
	{
		for (int j = 0; j < n - 1; ++j)
		{
			indices[k] = i * n + j;
			indices[k + 1] = i * n + j + 1;
			indices[k + 2] = (i + 1)*n + j;

			indices[k + 3] = (i + 1)*n + j;
			indices[k + 4] = i * n + j + 1;
			indices[k + 5] = (i + 1)*n + j + 1;

			k += 6;
		}
	}

	UINT vbByteSize = mWaves->VertexCount() * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "waterGeo";

	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	mGeometries["waterGeo"] = std::move(geo);

}

void TreeBillboardsApp::BuildTreeSpritesGeometry()
{
	struct TreeSpriteVertex
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Size;
	};

	constexpr int treeCount = 16;
	std::array<TreeSpriteVertex, treeCount> vertices;
	for (UINT i = 0; i < treeCount; ++i)
	{
		float x = MathHelper::RandF(-45.0f, 45.0f);
		float z = MathHelper::RandF(-45.0f, 45.0f);
		float y = GetHillsHeight(x, z);

		y += 8.0f;

		vertices[i].Pos = XMFLOAT3(x, y, z);
		vertices[i].Size = XMFLOAT2(20.0f, 20.0f);
	}

	std::array<std::uint16_t, treeCount> indices;
	for (UINT i = 0; i < treeCount; ++i)
		indices[i] = i;

	const UINT vbByteSize = sizeof(TreeSpriteVertex) * (UINT)vertices.size();
	const UINT ibByteSize = sizeof(std::uint16_t) * (UINT)indices.size();

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "treeSpriteGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, geo->VertexBufferCPU.GetAddressOf()));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	ThrowIfFailed(D3DCreateBlob(ibByteSize, geo->IndexBufferCPU.GetAddressOf()));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;
	geo->VertexBufferByteSize = vbByteSize;
	geo->VertexByteStride = sizeof(TreeSpriteVertex);

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.BaseVertexLocation = 0;
	submesh.StartIndexLocation = 0;

	geo->DrawArgs["points"] = submesh;

	mGeometries["treeSpriteGeo"] = std::move(geo);
}

void TreeBillboardsApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(mPSOs["opaque"].GetAddressOf())));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(mPSOs["transparent"].GetAddressOf())));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
	alphaTestedPsoDesc.PS =
	{
		 reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()),
		 mShaders["alphaTestedPS"]->GetBufferSize()
	};
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(mPSOs["alphaTested"].GetAddressOf())));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePsoDesc = opaquePsoDesc;
	treeSpritePsoDesc.VS =
	{
		RE_CAST(BYTE*, mShaders["treeSpriteVS"]->GetBufferPointer()),
		mShaders["treeSpriteVS"]->GetBufferSize()
	};
	treeSpritePsoDesc.PS =
	{
		RE_CAST(BYTE*, mShaders["treeSpritePS"]->GetBufferPointer()),
		mShaders["treeSpritePS"]->GetBufferSize()
	};
	treeSpritePsoDesc.GS =
	{
		RE_CAST(BYTE*, mShaders["treeSpriteGS"]->GetBufferPointer()),
		mShaders["treeSpriteGS"]->GetBufferSize()
	};
	treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	treeSpritePsoDesc.InputLayout = { mTreeSpriteInputLayout.data(), (UINT)mTreeSpriteInputLayout.size() };
	treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(mPSOs["treeSprites"].GetAddressOf())));
}

void TreeBillboardsApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			1, (UINT)mAllRitems.size(), (UINT)mMaterials.size(), mWaves->VertexCount()));
	}
}

void TreeBillboardsApp::BuildMaterials()
{
	auto grass = std::make_unique<Material>();
	grass->Name = "grass";
	grass->MatCBIndex = 0;
	grass->DiffuseSrvHeapIndex = 0;
	grass->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass->Roughness = .125f;

	auto water = std::make_unique<Material>();
	water->Name = "water";
	water->MatCBIndex = 1;
	water->DiffuseSrvHeapIndex = 1;
	water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.4f);
	water->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	water->Roughness = 0.0f;

	auto box = std::make_unique<Material>();
	box->Name = "box";
	box->MatCBIndex = 2;
	box->DiffuseSrvHeapIndex = 2;
	box->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	box->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	box->Roughness = 0.0f;

	auto treeSpirites = std::make_unique<Material>();
	treeSpirites->Name = "treeSpirites";
	treeSpirites->MatCBIndex = 3;
	treeSpirites->DiffuseSrvHeapIndex = 3;
	treeSpirites->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	treeSpirites->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	treeSpirites->Roughness = 0.125f;

	mMaterials["grass"] = std::move(grass);
	mMaterials["water"] = std::move(water);
	mMaterials["box"] = std::move(box);
	mMaterials["treeSprites"] = std::move(treeSpirites);
}

void TreeBillboardsApp::BuildRenderItems()
{
	auto wavesRitem = std::make_unique<RenderItem>();
	wavesRitem->World = MathHelper::Identity4x4();
	wavesRitem->ObjCbIndex = 0;
	XMStoreFloat4x4(&wavesRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	wavesRitem->Mat = mMaterials["water"].get();
	wavesRitem->Geo = mGeometries["waterGeo"].get();
	wavesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs["grid"].IndexCount;
	wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	mWavesRitem = wavesRitem.get();

	mRitemLayer[(int)RenderLayer::Transparent].push_back(wavesRitem.get());

	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->World = MathHelper::Identity4x4();
	gridRitem->ObjCbIndex = 1;
	gridRitem->Mat = mMaterials["grass"].get();
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	gridRitem->Geo = mGeometries["landGeo"].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());

	auto boxRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation(3.0f, 2.0f, -9.0f));
	boxRitem->ObjCbIndex = 2;
	boxRitem->Mat = mMaterials["box"].get();
	boxRitem->Geo = mGeometries["boxGeo"].get();
	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::AlphaTested].push_back(boxRitem.get());

	auto treeSpritesRitem = std::make_unique<RenderItem>();
	treeSpritesRitem->World = MathHelper::Identity4x4();
	treeSpritesRitem->ObjCbIndex = 3;
	treeSpritesRitem->Mat = mMaterials["treeSprites"].get();
	treeSpritesRitem->Geo = mGeometries["treeSpriteGeo"].get();
	treeSpritesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	treeSpritesRitem->IndexCount = treeSpritesRitem->Geo->DrawArgs["points"].IndexCount;
	treeSpritesRitem->StartIndexLocation = treeSpritesRitem->Geo->DrawArgs["points"].StartIndexLocation;
	treeSpritesRitem->BaseVertexLocation = treeSpritesRitem->Geo->DrawArgs["points"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites].push_back(treeSpritesRitem.get());

	mAllRitems.push_back(std::move(wavesRitem));
	mAllRitems.push_back(std::move(gridRitem));
	mAllRitems.push_back(std::move(boxRitem));
	mAllRitems.push_back(std::move(treeSpritesRitem));
}

void TreeBillboardsApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();

	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCbIndex*objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex*matCBByteSize;

		cmdList->SetGraphicsRootDescriptorTable(0, tex);
		cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
		cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
		//cmdList->DrawInstanced(4, 1, 0, 0);
		//cmdList->DrawInstanced(4, 1, 4, 0);
		//cmdList->DrawInstanced(4, 1, 8, 0);
		//cmdList->DrawInstanced(4, 1, 12, 0);
	}
}

float TreeBillboardsApp::GetHillsHeight(float x, float z) const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 TreeBillboardsApp::GetHillsNormal(float x, float z) const
{
	XMFLOAT3 n(
		-0.03f*z*cosf(0.1f*x) - 0.3f*cosf(0.1f*z),
		1.0f,
		-0.3f*sinf(0.1f*x) + 0.03f*x*sinf(0.1f*z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> TreeBillboardsApp::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f,
		8);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0.0f,
		8);

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp
	};
}