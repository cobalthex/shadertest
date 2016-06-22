#include "pch.hpp"
#include "App.h"
#include "Utility.hpp"
#include "d3dx12.hpp"

void App::Initialize()
{
	assert(DXManager::IsInitialized());

	auto& dev = DXManager::GetDevice();
	auto& cmdAlloc = DXManager::GetCommandAllocator();

	//create the root signature
	//todo: add root params for constant buffers
	{
		D3D12_ROOT_SIGNATURE_DESC rootSigDesc;
		rootSigDesc.NumParameters = 0;
		rootSigDesc.pParameters = nullptr;
		rootSigDesc.NumStaticSamplers = 0;
		rootSigDesc.pStaticSamplers = nullptr;
		rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(dev->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
	}

	// Create the pipeline state, which includes compiling and loading shaders.
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

		// Enable better shader debugging with the graphics debugging tools
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

		wchar_t basePath[512];
		GetAssetsPath(basePath, _countof(basePath));
		auto vspath = std::wstring(basePath) + L"VertexShader.hlsl";
		(D3DCompileFromFile(vspath.c_str(), nullptr, nullptr, "main", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
		auto pspath = std::wstring(basePath) + L"PixelShader.hlsl";
		ThrowIfFailed(D3DCompileFromFile(pspath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = { };
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = rootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(dev->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
	}

	// Create the command list.
	ThrowIfFailed(dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), pipelineState.Get(), IID_PPV_ARGS(&commandList)));

	auto viewport = DXManager::GetViewport();
	frameValues.value.resolution.width = (UINT)viewport.Width;
	frameValues.value.resolution.height = (UINT)viewport.Height;
	frameValues.value.aspectRatio = viewport.Width / viewport.Height;

	heap = DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 32, true);
	
	//Create the quad
	struct Vertex
	{
		struct { float x, y; } position;
		struct { float u, v; } texcoord;
	};

	Vertex verts[] =
	{
		{ { -1, -1 }, { 0, 0 } },
		{ { -1,  1 }, { 0, 1 } },
		{ {  1, -1 }, { 1, 0 } },
		{ {  1,  1 }, { 1, 1 } },
	};

	const UINT vertexBufferSize = sizeof(verts);

	CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
	ThrowIfFailed(dev->CreateCommittedResource
	(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&vertexBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&vertexBuffer)
	));
	vertexBuffer->SetName(L"Fullscreen Quad");

	ComPtr<ID3D12Resource> uploadBuffer;
	ThrowIfFailed(dev->CreateCommittedResource
	(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuffer)
	));
	
	D3D12_SUBRESOURCE_DATA vdata = { };
	vdata.pData = verts;
	vdata.RowPitch = vertexBufferSize;
	vdata.SlicePitch = vdata.RowPitch;
	
	UpdateSubresources(commandList.Get(), vertexBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &vdata);

	//wait for upload
	CD3DX12_RESOURCE_BARRIER vertexBufferResourceBarrier =
		CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	commandList->ResourceBarrier(1, &vertexBufferResourceBarrier);

	// Initialize the vertex buffer view.
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(Vertex);
	vertexBufferView.SizeInBytes = vertexBufferSize;

	frameValues = ConstantBuffer<CBFrameValues>(heap, DXManager::GetFrameCount());

	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	DXManager::GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	DXManager::WaitForGpu();
}

void App::Destroy()
{

}

void App::Update()
{
	POINT pos;
	GetCursorPos(&pos);
	frameValues.value.mouse.x = pos.x;
	frameValues.value.mouse.y = pos.y;

	frameValues.Update();
}

void App::Draw()
{
	ThrowIfFailed(DXManager::GetCommandAllocator()->Reset());
	ThrowIfFailed(commandList->Reset(DXManager::GetCommandAllocator().Get(), pipelineState.Get()));

	PIXBeginEvent(commandList.Get(), 0, L"Draw");
	{
		// Set the graphics root signature and descriptor heaps to be used by this frame.
		commandList->SetGraphicsRootSignature(rootSignature.Get());
		ID3D12DescriptorHeap* ppHeaps[] = { heap };
		commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		// Bind the current frame's constant buffer to the pipeline
		//heap.GetHandle(DXManager::GetCurrentFrame()).Bind(commandList, 0);

		// Set the viewport and scissor rectangle
		commandList->RSSetViewports(1, &DXManager::GetViewport());
		commandList->RSSetScissorRects(1, &DXManager::GetScissorRect());

		// Indicate this resource will be in use as a render target.
		CD3DX12_RESOURCE_BARRIER renderTargetResourceBarrier =
			CD3DX12_RESOURCE_BARRIER::Transition(DXManager::GetRenderTarget().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->ResourceBarrier(1, &renderTargetResourceBarrier);

		// Record drawing commands.
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView = DXManager::GetRenderTargetView();
		//D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = DXManager::GetDepthStencilView();
		float clearColor[] = { 0.5f, 0.75f, 1.f, 1.f };
		commandList->ClearRenderTargetView(renderTargetView, clearColor, 0, nullptr);
		//commandList->ClearDepthStencilView(depthStencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		commandList->OMSetRenderTargets(1, &renderTargetView, false, nullptr /*&depthStencilView*/);

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
		commandList->DrawInstanced(4, 1, 0, 0);

		// Indicate that the render target will now be used to present when the command list is done executing.
		CD3DX12_RESOURCE_BARRIER presentResourceBarrier =
			CD3DX12_RESOURCE_BARRIER::Transition(DXManager::GetRenderTarget().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		commandList->ResourceBarrier(1, &presentResourceBarrier);
	}
	PIXEndEvent(commandList.Get());

	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	DXManager::GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}