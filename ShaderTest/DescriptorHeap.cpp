#pragma once

#include "pch.hpp"
#include "Utility.hpp"
#include "DescriptorHeap.hpp"
#include "DXManager.hpp"

DescriptorHeap::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT MaxCount, bool IsShaderVisible, const ComPtr<ID3D12Device>& Device)
{
	auto dev = Device == nullptr ? DXManager::GetDevice() : Device;

	desc.Type = Type;
	desc.NumDescriptors = MaxCount;
	desc.Flags = (IsShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	desc.NodeMask = 1;

	ThrowIfFailed(dev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(heap.ReleaseAndGetAddressOf())));

#if defined(_DEBUG)
	heap->SetName((L"Descriptor Heap (Type: " + std::to_wstring(Type) + L")").c_str());
#endif

	firstHandle = DescriptorHandle(heap->GetCPUDescriptorHandleForHeapStart(), heap->GetGPUDescriptorHandleForHeapStart());
	descriptorSize = dev->GetDescriptorHandleIncrementSize(desc.Type);
}

bool DescriptorHeap::ValidateHandle(const DescriptorHandle& Handle) const
{
	if (Handle.GetCpuHandle().ptr < firstHandle.GetCpuHandle().ptr ||
		Handle.GetCpuHandle().ptr >= firstHandle.GetCpuHandle().ptr + (desc.NumDescriptors * descriptorSize))
		return false;

	if (Handle.GetGpuHandle().ptr - firstHandle.GetGpuHandle().ptr !=
		Handle.GetCpuHandle().ptr - firstHandle.GetCpuHandle().ptr)
		return false;

	return true;
}
