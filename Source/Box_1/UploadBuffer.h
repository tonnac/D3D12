#pragma once

#include "d3dUtil.h"

template <class X>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer = false) :
		mIsConstantBuffer(isConstantBuffer)
	{
		if (isConstantBuffer)
			mElementByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(X));

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(elementCount * mElementByteSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(mUploadBuffer.GetAddressOf())));

		ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));

	}
	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator= (const UploadBuffer& rhs) = delete;
	~UploadBuffer()
	{
		if (mUploadBuffer != nullptr)
			mUploadBuffer->Unmap(0, nullptr);
		
		mMappedData = nullptr;
	}

	ID3D12Resource* Resource()const
	{
		return mUploadBuffer.Get();
	}

	void CopyData(int elementIndex, const X& data)
	{
		memcpy(&mMappedData[elementIndex * mElementByteSize], &data, sizeof(X));
	}
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
	BYTE* mMappedData = nullptr;

	UINT mElementByteSize = 0;
	bool mIsConstantBuffer = false;
};