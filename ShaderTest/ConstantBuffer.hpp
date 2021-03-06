#pragma once

#include "DXManager.hpp"
#include "Utility.hpp"
#include "DescriptorHeap.hpp"

template <typename TValue>
class ConstantBuffer
{
public:
	ConstantBuffer() = default;

	void Create()
	{
		ThrowIfFailed(DXManager::GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(alignedSize * DXManager::GetFrameCount()),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&cbuffer))
		);

		ThrowIfFailed(cbuffer->Map(0, nullptr, &cbvData));
		ZeroMemory(cbvData, alignedSize * DXManager::GetFrameCount());
	}
	//Create the constant buffer and bind it to a heap
	//heap must have at least BufferCount free descriptors
	//returns the base descriptor created
	DescriptorHandle Create(DescriptorAllocator& Allocator)
	{
		Create();

		auto dev = DXManager::GetDevice();
		auto addr = cbuffer->GetGPUVirtualAddress();

		auto dtors = Allocator.Allocate(fc);
		for (size_t i = 0; i < fc; i++)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC desc = { };
			desc.BufferLocation = addr;
			desc.SizeInBytes = alignedSize;
			dev->CreateConstantBufferView(&desc, dtor + Allocator.GetDescriptorSize());
			addr += desc.SizeInBytes;
		}

		return dtors;
	}
	ConstantBuffer(const ConstantBuffer&) = delete;
	ConstantBuffer& operator=(const ConstantBuffer&) = delete;
	ConstantBuffer& operator=(ConstantBuffer&&) = default;

	~ConstantBuffer()
	{
		if (cbuffer != nullptr && cbvData != nullptr)
		{
			cbuffer->Unmap(0, nullptr);
			cbvData = nullptr;
		}
	}

	inline D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const { return cbuffer->GetGPUVirtualAddress(); }

	inline void Update() { CopyMemory((UINT8*)cbvData + (alignedSize * DXManager::GetCurrentFrame()), &value, sizeof(value)); } //update the constant buffer with data from value

	TValue value;

protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> cbuffer;
	void* cbvData;

	static const size_t alignedSize = (sizeof(TValue) + 255) & ~255;
};