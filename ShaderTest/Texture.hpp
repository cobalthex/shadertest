#pragma once

#include "DXManager.hpp"
#include "DescriptorHeap.hpp"

//A simple 2D texture
class Texture
{
public:
	Texture() { }
	Texture(const std::string& File, const ComPtr<ID3D12GraphicsCommandList>& CommandList, DescriptorAllocator& Allocator);
	Texture(const std::vector<BYTE>& FileData, const ComPtr<ID3D12GraphicsCommandList>& CommandList, DescriptorAllocator& Allocator);
	Texture(void* FileData, UINT Size, const ComPtr<ID3D12GraphicsCommandList>& CommandList, DescriptorAllocator& Allocator);
	~Texture() = default;

	inline void Cleanup() { uploadHeap.Reset(); } //delete the texture upload heap (must happen after creation command list has executed)

	inline const DescriptorHandle& GetView() const { return view; }
	inline D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const { return texture->GetGPUVirtualAddress(); }

protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> texture;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadHeap; //todo: offload somehow (maybe texture gets its own command list)
	DescriptorHandle view;

	void InternalStbLoadPixels(UINT8* Pixels, int Width, int Height, int Components, const ComPtr<ID3D12GraphicsCommandList>& CommandList, DescriptorAllocator& Allocator);
};