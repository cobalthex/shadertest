#pragma once

#include "DescriptorHeap.hpp"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace DXManager
{
	void Initialize(HWND Window, bool UseWarpDevice = false);
	void Destroy();
	void Resize(int Width, int Height);

	bool IsInitialized();

	HWND GetWindow(); //get the window that the device is rendering to

	ComPtr<ID3D12Device>& GetDevice();
	ComPtr<IDXGISwapChain3>& GetSwapChain();
	ComPtr<ID3D12CommandAllocator>& GetCommandAllocator(); //Get the current frame's command allocator
	ComPtr<ID3D12CommandQueue>& GetCommandQueue();

	ComPtr<ID3D12Resource>& GetRenderTarget(); //Get the current frame's render target
	DescriptorHandle GetRenderTargetView(); //Get the current frame's render target view
	D3D12_VIEWPORT GetViewport(); //Get the default (full screen) viewport
	D3D12_RECT GetScissorRect(); //Get the default (full screen) scissor rect

	UINT64 GetCurrentFrame(); //get the current frame
	UINT64 GetFrameCount(); //get the number of render frames

	void WaitForGpu();

	void Begin();
	void End();
};