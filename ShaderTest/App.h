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

	//Shader Profile is index from DXManager::PixelShaderProfiles
	HRESULT UpdatePixelShader(const std::vector<BYTE>& Code, size_t ShaderProfile = 0, ID3DBlob** ErrorMessages = nullptr); 

	bool LoadTexture(const std::vector<BYTE>& Data, size_t Slot);

	bool clearOnDraw = false;
	float clearColor[4] = { 0, 0, 0, 0 };

private:
	ComPtr<ID3D12RootSignature> rootSignature;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = { };
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12GraphicsCommandList> commandList;

	DescriptorAllocator heap;

	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	ConstantBuffer<CBFrameValues> frameValues;
	std::vector<Texture> textures;

	LARGE_INTEGER perfStart, perfFreq;

	ComPtr<ID3DBlob> vertexShader;
	std::vector<BYTE> psBase; //shader includes

	//std::vector<byte> BuildPixelShader();
};