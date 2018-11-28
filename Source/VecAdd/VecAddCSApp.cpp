#include "d3dApp.h"
#include "MathHelper.h"
#include "UploadBuffer.h"


using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

struct Data
{
	XMFLOAT3 v1;
	XMFLOAT2 v2;
};

class VecAddCSApp : public D3DApp
{
public:
	VecAddCSApp(HINSTANCE hInstance);
	VecAddCSApp(const VecAddCSApp& rhs) = delete;
	VecAddCSApp& operator=(const VecAddCSApp& rhs) = delete;
	~VecAddCSApp();

	virtual bool Initialize()override;

private:
	virtual void Update(const GameTimer& gt)override
	{
		return;
	}
	virtual void Draw(const GameTimer& gt)override
	{
		return;
	}

	void DoComputeWork();

	void BuildBuffers();
	void BuildDescriptorHeap();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildPSOs();

private:

	const int NumDataElements = 32;

	UINT mCbvSrvDescriptorSize = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	ComPtr<ID3D12DescriptorHeap> mCbvSrvUavHeap;;

	ComPtr<ID3D12Resource> mInputBufferA = nullptr;
	ComPtr<ID3D12Resource> mInputUploadBufferA = nullptr;
	ComPtr<ID3D12Resource> mInputBufferB = nullptr;
	ComPtr<ID3D12Resource> mInputUploadBufferB = nullptr;
	ComPtr<ID3D12Resource> mOutputBuffer = nullptr;
	ComPtr<ID3D12Resource> mReadBackBuffer = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdList, int showCmd)
{

#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		VecAddCSApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return 0;
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

VecAddCSApp::VecAddCSApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
}
VecAddCSApp::~VecAddCSApp()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

bool VecAddCSApp::Initialize()
{	 
	if (!D3DApp::Initialize())
		return false;

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	BuildBuffers();
	BuildRootSignature();
	BuildDescriptorHeap();
	BuildShadersAndInputLayout();
	BuildPSOs();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists((UINT)std::size(cmdLists), cmdLists);

	FlushCommandQueue();

	DoComputeWork();

	return true;
}	 

void VecAddCSApp::BuildDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = 1;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mInputBufferA->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	srvDesc.Buffer.NumElements = 32;
	srvDesc.Buffer.StructureByteStride = sizeof(Data);

	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(mCbvSrvUavHeap.GetAddressOf())));

	md3dDevice->CreateShaderResourceView(mInputBufferA.Get(), &srvDesc, mCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart());
}


void VecAddCSApp::DoComputeWork()
{
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSOs["vecAdd"].Get()));

	mCommandList->SetComputeRootSignature(mRootSignature.Get());

	ID3D12DescriptorHeap * descriptorHeaps[] = { mCbvSrvUavHeap.Get() };
	mCommandList->SetDescriptorHeaps((UINT)std::size(descriptorHeaps), descriptorHeaps);

	CD3DX12_GPU_DESCRIPTOR_HANDLE heapHandle(mCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart());

	mCommandList->SetComputeRootDescriptorTable(0, heapHandle);
//	mCommandList->SetComputeRootShaderResourceView(0, mInputBufferA->GetGPUVirtualAddress());
	mCommandList->SetComputeRootShaderResourceView(1, mInputBufferB->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(2, mOutputBuffer->GetGPUVirtualAddress());

	mCommandList->Dispatch(1, 1, 1);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE));

	mCommandList->CopyResource(mReadBackBuffer.Get(), mOutputBuffer.Get());

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists((UINT)std::size(cmdsLists), cmdsLists);

	FlushCommandQueue();

	Data* mappedData = nullptr;
	ThrowIfFailed(mReadBackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));

	std::ofstream fout("results.txt");

	for (int i = 0; i < NumDataElements; ++i)
	{
		fout << "(" << mappedData[i].v1.x << ", " << mappedData[i].v1.y << ", " << mappedData[i].v1.z <<
			", " << mappedData[i].v2.x << ", " << mappedData[i].v2.y << ")" << std::endl;
	}

	mReadBackBuffer->Unmap(0, nullptr);

}


void VecAddCSApp::BuildRootSignature()
{
	CD3DX12_ROOT_PARAMETER slotRootParameter[3];
	CD3DX12_DESCRIPTOR_RANGE srvTable;
	srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	slotRootParameter[0].InitAsDescriptorTable(1, &srvTable);
	slotRootParameter[1].InitAsShaderResourceView(1);
	slotRootParameter[2].InitAsUnorderedAccessView(0);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter);

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

void VecAddCSApp::BuildShadersAndInputLayout()
{
	mShaders["vecAddCS"] = d3dUtil::CompileShader(L"VecAdd.hlsl", nullptr, "CS", "cs_5_0");
}


void VecAddCSApp::BuildPSOs()
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
	computePsoDesc.pRootSignature = mRootSignature.Get();
	computePsoDesc.CS =
	{
		reinterpret_cast<BYTE*>(mShaders["vecAddCS"]->GetBufferPointer()),
		mShaders["vecAddCS"]->GetBufferSize()
	};
	computePsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(mPSOs["vecAdd"].GetAddressOf())));
}

void VecAddCSApp::BuildBuffers()
{
	std::vector<Data> dataA(NumDataElements);
	std::vector<Data> dataB(NumDataElements);
	for (int i = 0; i < NumDataElements; ++i)
	{
		dataA[i].v1 = XMFLOAT3(i, i, i);
		dataA[i].v2 = XMFLOAT2(i, 0);

		dataB[i].v1 = XMFLOAT3(-i, i, 0.0f);
		dataB[i].v2 = XMFLOAT2(0, -i);
	}

	UINT64 byteSize = (UINT64)dataA.size() * sizeof(Data);

	mInputBufferA = d3dUtil::CreateDefaultBuffer(
		md3dDevice.Get(),
		mCommandList.Get(),
		dataA.data(),
		byteSize,
		mInputUploadBufferA);

	mInputBufferB = d3dUtil::CreateDefaultBuffer(
		md3dDevice.Get(),
		mCommandList.Get(),
		dataB.data(),
		byteSize,
		mInputUploadBufferB);

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&mOutputBuffer)));
	
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&mReadBackBuffer)));
}
