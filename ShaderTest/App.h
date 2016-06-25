#pragma once

#include "DXManager.hpp"
#include "DescriptorHeap.hpp"
#include "ConstantBuffer.hpp"
#include "Texture.hpp"

enum MouseState : UINT
{
	MS_None = 0,
	MS_Left = 1,
	MS_Middle = 2,
	MS_Right = 4,
	MS_X1 = 8,
	MS_X2 = 16,
};

struct CBFrameValues
{
	struct { UINT x, y, state, scroll; } mouse;
	struct { UINT width, height; } resolution;
	float aspectRatio;
	float time; //in seconds
};

class App
{
public:
	App() = default; //assumes DXManager has been initialized
	~App() = default;

	void Initialize();
	void Destroy();

	void Update();
	void Draw();

	HRESULT UpdatePixelShader(void* Code, size_t CodeSize, ID3DBlob** ErrorMessages);

private:
	ComPtr<ID3D12RootSignature> rootSignature;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = { };
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12GraphicsCommandList> commandList;

	DescriptorAllocator heap;

	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	ConstantBuffer<CBFrameValues> frameValues;

	ComPtr<ID3DBlob> vertexShader;

	LARGE_INTEGER perfStart, perfFreq;
};