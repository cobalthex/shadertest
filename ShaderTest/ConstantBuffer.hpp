#pragma once

#include "DXManager.hpp"
#include "Utility.hpp"
#include "DescriptorHeap.hpp"

template <typename TValue>
class ConstantBuffer
{
public:
	ConstantBuffer() = default;
	ConstantBuffer(DescriptorAllocator& Allocator, size_t BufferCount = 1) //heap must have at least BufferCount free descriptors
		: cbvData(nullptr)
	{
		auto dev = DXManager::GetDevice();

		ThrowIfFailed(dev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(alignedSize * BufferCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&cbuffer))
		);

		auto addr = cbuffer->GetGPUVirtualAddress();
		for (size_t i = 0; i < BufferCount; i++)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC desc = { };
			desc.BufferLocation = addr;
			desc.SizeInBytes = alignedSize;
			dev->CreateConstantBufferView(&desc, Allocator.Allocate());
			addr += desc.SizeInBytes;
		}

		ThrowIfFailed(cbuffer->Map(0, nullptr, &cbvData));
		ZeroMemory(cbvData, alignedSize * BufferCount);
	}

	~ConstantBuffer()
	{
		if (cbvData != nullptr)
		{
			cbuffer->Unmap(0, nullptr);
			cbvData = nullptr;
		}
	}

	inline D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const { return cbuffer->GetGPUVirtualAddress(); }

	inline void Update(size_t Index = 0) { CopyMemory((UINT8*)cbvData + (alignedSize * Index), &value, sizeof(value)); } //update the constant buffer with data from value

	TValue value;

protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> cbuffer;
	void* cbvData;

	static const size_t alignedSize = (sizeof(TValue) + 255) & ~255;
};