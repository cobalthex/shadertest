#pragma once

using Microsoft::WRL::ComPtr;

//A CPU/GPU handle to a single descriptor
class DescriptorHandle
{
public:
	static const size_t Null = ~0ull;

	DescriptorHandle()
	{
		cpuHandle.ptr = Null;
		gpuHandle.ptr = Null;
	}

	DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle) : cpuHandle(CpuHandle) { gpuHandle.ptr = Null; }
	DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle)
		: cpuHandle(CpuHandle), gpuHandle(GpuHandle) { }

	inline DescriptorHandle operator+(size_t OffsetScaledByDescriptorSize) const
	{
		DescriptorHandle rval = *this;
		rval += OffsetScaledByDescriptorSize;
		return rval;
	}

	inline DescriptorHandle& operator+=(size_t OffsetScaledByDescriptorSize)
	{
		if (cpuHandle.ptr != Null)
			cpuHandle.ptr += OffsetScaledByDescriptorSize;
		if (gpuHandle.ptr != Null)
			gpuHandle.ptr += OffsetScaledByDescriptorSize;

		return *this;
	}

	inline D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const { return cpuHandle; }
	inline D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const { return gpuHandle; }

	inline operator D3D12_CPU_DESCRIPTOR_HANDLE() const { return cpuHandle; }
	inline operator D3D12_GPU_DESCRIPTOR_HANDLE() const { return gpuHandle; }

	inline bool IsNull() const { return cpuHandle.ptr == Null; }
	inline bool IsShaderVisible() const { return gpuHandle.ptr != Null; }

	inline void BindToTable(const ComPtr<ID3D12GraphicsCommandList>& CommandList, UINT RootParameterIndex) const
	{
		CommandList->SetGraphicsRootDescriptorTable(RootParameterIndex, gpuHandle);
	}

private:
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
};

//A fixed sized descriptor heap
class DescriptorHeap
{
public:
	DescriptorHeap() = default;
	DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT MaxCount, bool IsShaderVisible = true, const ComPtr<ID3D12Device>& Device = nullptr);
	~DescriptorHeap() = default;

	inline operator Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>&() { return heap; }
	inline operator const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>&() const { return heap; }

	inline operator ID3D12DescriptorHeap*() { return heap.Get(); }

	inline DescriptorHandle GetHandle(size_t Index) const { return firstHandle + (Index * descriptorSize); }
	bool DescriptorHeap::ValidateHandle(const DescriptorHandle& Handle) const;

	inline size_t GetDescriptorSize() const { return descriptorSize; }

	inline DescriptorHandle& Increment(DescriptorHandle& Handle, size_t Count = 1) { return (Handle += (Count * descriptorSize)); }

protected:
	D3D12_DESCRIPTOR_HEAP_DESC desc;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
	DescriptorHandle firstHandle;
	size_t descriptorSize;
};

//A simple allocator to automatically create descriptor handles
//todo: currently restricted to heap size -- auto create new heaps and copy old descriptors
class DescriptorAllocator : public DescriptorHeap
{
public:
	DescriptorAllocator() = default;
	DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT MaxCount = 512, bool IsShaderVisible = true, const ComPtr<ID3D12Device>& Device = nullptr)
		: DescriptorHeap(Type, MaxCount, IsShaderVisible, Device), nextFreeHandle(firstHandle), numFreeDescriptors(MaxCount) { }
	~DescriptorAllocator() = default;

	inline DescriptorHandle Allocate(size_t Count = 1)
	{
		assert(CountAvailable() >= Count); //"Descriptor Heap out of space - Increase heap size"
		DescriptorHandle retVal = nextFreeHandle;
		nextFreeHandle += descriptorSize * Count;
		numFreeDescriptors -= Count;
		return retVal;
	}

	inline size_t CountAvailable() const { return numFreeDescriptors; }

private:
	DescriptorHandle nextFreeHandle;
	size_t numFreeDescriptors;
};