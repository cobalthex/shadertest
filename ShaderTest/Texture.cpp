#include "pch.hpp"
#include "Texture.hpp"
#include "Utility.hpp"
#include "d3dx12.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "StbImage.hpp"

Texture::Texture(const std::string& File, const ComPtr<ID3D12GraphicsCommandList>& CommandList, DescriptorAllocator& Allocator)
{
	int width, height, cpx;
	auto pixels = stbi_load(File.c_str(), &width, &height, &cpx, 0);

	InternalStbLoadPixels(pixels, width, height, cpx, CommandList, Allocator);
	stbi_image_free(pixels);
}

Texture::Texture(const std::vector<BYTE>& FileData, const ComPtr<ID3D12GraphicsCommandList>& CommandList, DescriptorAllocator& Allocator)
{
	int width, height, cpx;
	auto pixels = stbi_load_from_memory(FileData.data(), (int)FileData.size(), &width, &height, &cpx, 0);

	InternalStbLoadPixels(pixels, width, height, cpx, CommandList, Allocator);
	stbi_image_free(pixels);
}
Texture::Texture(void* FileData, UINT Size, const ComPtr<ID3D12GraphicsCommandList>& CommandList, DescriptorAllocator& Allocator)
{
	int width, height, cpx;
	auto pixels = stbi_load_from_memory((UINT8*)FileData, (int)Size, &width, &height, &cpx, 0);
	
	InternalStbLoadPixels(pixels, width, height, cpx, CommandList, Allocator);
	stbi_image_free(pixels);
}

void Texture::InternalStbLoadPixels(UINT8* Pixels, int Width, int Height, int Components, const ComPtr<ID3D12GraphicsCommandList>& CommandList, DescriptorAllocator& Allocator)
{
	auto& dev = DXManager::GetDevice();

	UINT* _pixels = nullptr;

	// Describe and create a Texture2D
	D3D12_RESOURCE_DESC textureDesc = { };
	textureDesc.MipLevels = 1;
	textureDesc.Width = Width;
	textureDesc.Height = Height;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	switch (Components)
	{
	case 1:
		textureDesc.Format = DXGI_FORMAT_R8_UNORM;
		break;
	case 2:
		textureDesc.Format = DXGI_FORMAT_R8G8_UNORM;
		break;
	case 3:
	{
		Components = 4;
		_pixels = new UINT[Width * Height];
		for (size_t i = 0; i < Width * Height; i++)
			_pixels[i] = *(UINT*)(Pixels + (i * 3)) | 0xff000000;
	}
	case 4:
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	}

	ThrowIfFailed(dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&texture)));

	UINT64 requiredSize = 0;
	dev->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &requiredSize);

	// Create the GPU upload buffer.
	ThrowIfFailed(dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(requiredSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadHeap)));

	D3D12_SUBRESOURCE_DATA textureData = { };
	textureData.pData = _pixels != nullptr ? (void*)_pixels : (void*)Pixels;
	textureData.RowPitch = Width * Components;
	textureData.SlicePitch = textureData.RowPitch * Height;

	UpdateSubresources(CommandList.Get(), texture.Get(), uploadHeap.Get(), 0, 0, 1, &textureData);

	CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	view = Allocator.Allocate();

	// Describe and create an SRV for the texture
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = { };
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	dev->CreateShaderResourceView(texture.Get(), &srvDesc, view.GetCpuHandle());

	if (_pixels != nullptr)
		delete[] _pixels;
}