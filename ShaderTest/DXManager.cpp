#include "pch.hpp"
#include "DXManager.hpp"
#include "Utility.hpp"

namespace
{
	bool isInitialized = false;

	const UINT FrameCount = 2;

	UINT currentFrame = 0;
	UINT64 fenceValues[FrameCount] = { 0 };
	HANDLE fenceEvent;
	ComPtr<ID3D12Fence> fence;

	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;
	ComPtr<IDXGISwapChain3> swapChain;
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D12CommandAllocator> commandAllocators[FrameCount];
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12RootSignature> rootSignature;

	ComPtr<ID3D12Resource> renderTargets[FrameCount];
	DescriptorHeap rtvHeap;
};

void DXManager::Initialize(HWND Window, bool UseWarpDevice)
{
	isInitialized = false;

	RECT wndRect;
	GetClientRect(Window, &wndRect);
	UINT wndWidth = wndRect.right - wndRect.left;
	UINT wndHeight = wndRect.bottom - wndRect.top;

#pragma region Factory

#if defined(_DEBUG)
	// Enable the D3D12 debug layer.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			debugController->EnableDebugLayer();
	}
#endif

	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

#pragma endregion

#pragma region Adapter

	ComPtr<IDXGIAdapter1> adapter;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		//don't use software renderer (use warp instead)
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		//Check if D3D12 is supported
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			break;
	}

#pragma endregion

#pragma region Device

	if (UseWarpDevice)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice
		(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device)
		));
	}
	else
	{
		ThrowIfFailed(D3D12CreateDevice
		(
			adapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device)
		));
	}

#pragma endregion

#pragma region Command queue

	D3D12_COMMAND_QUEUE_DESC queueDesc = { };
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

#pragma endregion

#pragma region Swap chain

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { };
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = wndWidth;
	swapChainDesc.Height = wndHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> _swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd
	(
		commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
		Window,
		&swapChainDesc,
		nullptr,
		nullptr,
		&_swapChain
	));

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(factory->MakeWindowAssociation(Window, DXGI_MWA_NO_ALT_ENTER));
	ThrowIfFailed(_swapChain.As(&swapChain));

#pragma endregion

#pragma region Frame resources

	rtvHeap = DescriptorHeap
	(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		FrameCount,
		false,
		device
	);

	auto rtvHandle = rtvHeap.GetHandle(0);
	for (UINT i = 0; i < FrameCount; i++)
	{
		//render targets
		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
		device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle.GetCpuHandle());
		rtvHeap.Increment(rtvHandle);

		//command allocators
		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[i])));
	}

	viewport = { 0.0f, 0.0f, (float)wndWidth, (float)wndHeight, 0.0f, 1.0f };
	scissorRect = { 0, 0, (LONG)wndWidth, (LONG)wndHeight };

#pragma endregion

#pragma region Fence

	currentFrame = swapChain->GetCurrentBackBufferIndex();
	ThrowIfFailed(device->CreateFence(fenceValues[currentFrame], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	fenceValues[currentFrame]++;

	// Create an event handle to use for frame synchronization
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent == nullptr)
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));

	// Wait for the command list to execute; we are reusing the same command 
	// list in our main loop but for now, we just want to wait for setup to 
	// complete before continuing.
	WaitForGpu();

#pragma endregion

	isInitialized = true;
}

void DXManager::Destroy()
{
	isInitialized = false;
	WaitForGpu();
	CloseHandle(fenceEvent);
}

bool DXManager::IsInitialized()
{
	return isInitialized;
}

HWND DXManager::GetWindow()
{
	HWND hwnd = NULL;
	swapChain->GetHwnd(&hwnd);
	return hwnd;
}

UINT64 DXManager::GetCurrentFrame()
{
	return currentFrame;
}

UINT64 DXManager::GetFrameCount()
{
	return FrameCount;
}

ComPtr<ID3D12Device>& DXManager::GetDevice()
{
	return device;
}

ComPtr<IDXGISwapChain3>& DXManager::GetSwapChain()
{
	return swapChain;
}

ComPtr<ID3D12CommandAllocator>& DXManager::GetCommandAllocator()
{
	return commandAllocators[currentFrame];
}
ComPtr<ID3D12CommandQueue>& DXManager::GetCommandQueue()
{
	return commandQueue;
}

ComPtr<ID3D12Resource>& DXManager::GetRenderTarget()
{
	return renderTargets[currentFrame];
}
DescriptorHandle DXManager::GetRenderTargetView()
{
	return rtvHeap.GetHandle(currentFrame);
}

D3D12_VIEWPORT DXManager::GetViewport()
{
	return viewport;
}

D3D12_RECT DXManager::GetScissorRect()
{
	return scissorRect;
}

void DXManager::Begin()
{
	ThrowIfFailed(commandAllocators[currentFrame]->Reset());
}

void DXManager::WaitForGpu()
{
	//Schedule a Signal command in the queue
	ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValues[currentFrame]));

	//Wait until the fence has been crossed
	ThrowIfFailed(fence->SetEventOnCompletion(fenceValues[currentFrame], fenceEvent));
	WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);

	//Increment the fence value for the current frame
	InterlockedIncrement(&fenceValues[currentFrame]);
}

void NextFrame()
{
	// Schedule a Signal command in the queue.
	const UINT64 currentFenceValue = fenceValues[currentFrame];
	ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFenceValue));

	// Advance the frame index.
	currentFrame = (currentFrame + 1) % FrameCount;

	// Check to see if the next frame is ready to start.
	if (fence->GetCompletedValue() < fenceValues[currentFrame])
	{
		ThrowIfFailed(fence->SetEventOnCompletion(fenceValues[currentFrame], fenceEvent));
		WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);
	}

	// Set the fence value for the next frame.
	fenceValues[currentFrame] = currentFenceValue + 1;
}

void DXManager::End()
{
	ThrowIfFailed(swapChain->Present(1, 0));

	//todo: handle device removed
	NextFrame();
}


void DXManager::Resize(int Width, int Height)
{
	if (swapChain == nullptr)
		return;

	WaitForGpu();

	for (UINT i = 0; i < FrameCount; i++)
	{
		renderTargets[i].Reset();
		fenceValues[i] = fenceValues[currentFrame];
	}

	ThrowIfFailed(swapChain->ResizeBuffers(FrameCount, Width, Height, DXGI_FORMAT_UNKNOWN, 0));

	auto rtvHandle = rtvHeap.GetHandle(0);
	for (UINT i = 0; i < FrameCount; i++)
	{
		//render targets
		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
		device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle.GetCpuHandle());
		rtvHeap.Increment(rtvHandle);
	}

	viewport.Width = (float)Width;
	viewport.Height = (float)Height;

	scissorRect.right = scissorRect.left + Width;
	scissorRect.bottom = scissorRect.top + Height;

	currentFrame = swapChain->GetCurrentBackBufferIndex();
}