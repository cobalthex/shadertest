#include "pch.hpp"
#include "Texture.hpp"
#include "Utility.hpp"
#include "d3dx12.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "StbImage.hpp"

Texture::Texture(const std::string& File, const ComPtr<ID3D12GraphicsCommandList>& CommandList, DescriptorAllocator& Allocator)
{
	auto& dev = DXManager::GetDevice();

	int width, height, cpx;
	auto pixels = stbi_load(File.c_str(), &width, &height, &cpx, 0);
	bool custom = false;

	// Describe and create a Texture2D
	D3D12_RESOURCE_DESC textureDesc = { };
	textureDesc.MipLevels = 1;
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	switch (cpx)
	{
	case 1:
		textureDesc.Format = DXGI_FORMAT_R8_UNORM;
		break;
	case 2:
		textureDesc.Format = DXGI_FORMAT_R8G8_UNORM;
		break;
	case 3:
	{
		cpx = 4;
		auto _px = new UINT[width * height];
		for (size_t i = 0; i < width * height; i++)
			_px[i] = ((UINT)pixels[i * 3] << 16) | 0xff;
		stbi_image_free(pixels);
		pixels = (UINT8*)_px;
		custom = true;
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
	textureData.pData = pixels;
	textureData.RowPitch = width * cpx;
	textureData.SlicePitch = textureData.RowPitch * height;

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

	if (custom)
		delete[] pixels;
	else
		stbi_image_free(pixels);
}
