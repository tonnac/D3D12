#include "d3dApp.h"
#include "MathHelper.h"
#include "UploadBuffer.h"


using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

struct Data
{
	XMFLOAT3 v1;
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

	const int NumDataElements = 64;

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

	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildPSOs();
	BuildBuffers();
	BuildDescriptorHeap();


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
	heapDesc.NumDescriptors = 2;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(mCbvSrvUavHeap.GetAddressOf())));

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Format = mInputBufferA->GetDesc().Format;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = NumDataElements;
	uavDesc.Buffer.StructureByteStride = sizeof(float) * 3;
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	md3dDevice->CreateUnorderedAccessView(mInputBufferA.Get(), nullptr, &uavDesc, handle);

	handle.Offset(1, mCbvSrvUavDescriptorSize);
	
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Format = mOutputBuffer->GetDesc().Format;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = NumDataElements;
	uavDesc.Buffer.StructureByteStride = sizeof(float);
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	md3dDevice->CreateUnorderedAccessView(mOutputBuffer.Get(), nullptr, &uavDesc, handle);
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

	heapHandle.Offset(1, mCbvSrvDescriptorSize);

	mCommandList->SetComputeRootDescriptorTable(1, heapHandle);
	//mCommandList->SetComputeRootUnorderedAccessView(0, mInputBufferA->GetGPUVirtualAddress());
	//mCommandList->SetComputeRootUnorderedAccessView(1, mOutputBuffer->GetGPUVirtualAddress());

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputBufferA.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputBufferB.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE));

	mCommandList->CopyResource(mInputBufferA.Get(), mInputBufferB.Get());

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputBufferA.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputBufferB.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_GENERIC_READ));

	mCommandList->Dispatch(2, 1, 1);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE));

	mCommandList->CopyResource(mReadBackBuffer.Get(), mOutputBuffer.Get());

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists((UINT)std::size(cmdsLists), cmdsLists);

	FlushCommandQueue();

	float* mappedData = nullptr;
	ThrowIfFailed(mReadBackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));

	std::ofstream fout("results.txt");

	for (int i = 0; i < NumDataElements; ++i)
	{
		fout << "[" << i << "]: " << mappedData[i] << std::endl;
	}

	mReadBackBuffer->Unmap(0, nullptr);

}


void VecAddCSApp::BuildRootSignature()
{
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];
	CD3DX12_DESCRIPTOR_RANGE Table;
	Table.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
	CD3DX12_DESCRIPTOR_RANGE Table1;
	Table1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);

//	slotRootParameter[0].InitAsUnorderedAccessView(0);
//	slotRootParameter[1].InitAsUnorderedAccessView(1);
	slotRootParameter[0].InitAsDescriptorTable(1, &Table);
	slotRootParameter[1].InitAsDescriptorTable(1, &Table1);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc((UINT)std::size(slotRootParameter), slotRootParameter);

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
	for (int i = 0; i < NumDataElements; ++i)
	{
		dataA[i].v1 = XMFLOAT3(MathHelper::RandF(0,9), MathHelper::RandF(0,9), MathHelper::RandF(0,9));
		XMFLOAT2 Length;
		XMStoreFloat2(&Length, XMVector3Length(XMLoadFloat3(&dataA[i].v1)));
		while (Length.x > 10.0f)
		{
			dataA[i].v1 = XMFLOAT3(MathHelper::RandF(0, 9), MathHelper::RandF(0, 9), MathHelper::RandF(0, 9));
			XMStoreFloat2(&Length, XMVector3Length(XMLoadFloat3(&dataA[i].v1)));
		}
	}


	UINT64 byteSize = (UINT64)dataA.size() * sizeof(Data);

	D3D12_RESOURCE_DESC bufDesc;
	ZeroMemory(&bufDesc, sizeof(D3D12_RESOURCE_DESC));

	bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufDesc.Alignment = 0;
	bufDesc.Width = byteSize;
	bufDesc.Height = 1;
	bufDesc.DepthOrArraySize = 1;
	bufDesc.MipLevels = 1;
	bufDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufDesc.SampleDesc.Count = 1;
	bufDesc.SampleDesc.Quality = 0;
	bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&bufDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(mInputBufferA.GetAddressOf())));

	mInputBufferB = d3dUtil::CreateDefaultBuffer(
		md3dDevice.Get(),
		mCommandList.Get(),
		dataA.data(),
		(UINT)byteSize,
		mInputUploadBufferB);

	UINT outByteSize = sizeof(float) * (UINT)dataA.size();

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(outByteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&mOutputBuffer)));
	
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(outByteSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&mReadBackBuffer)));
}
