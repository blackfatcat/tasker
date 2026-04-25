#include "tasker.hpp"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <Windows.h>
#include <wrl/client.h>

struct Startup {};
struct Main {};
struct Render {};
struct Sync {};
struct Shutdown {};

using Microsoft::WRL::ComPtr;

struct Dx12Device {
    ComPtr<ID3D12Device> device;
    ComPtr<IDXGISwapChain3> swapchain;
    ComPtr<ID3D12CommandQueue> gfx_queue;
    // allocators, RTV heap, etc.
};

struct CommandListPool {
    ComPtr<ID3D12Device> device;
    std::vector<ComPtr<ID3D12GraphicsCommandList>> free_lists;

    ID3D12GraphicsCommandList* acquire();
    void release(ID3D12GraphicsCommandList*);
};

struct FrameState {
    uint32_t frame_index;
    // per-frame CBs, camera, etc.
};

struct RenderLists {
    ID3D12GraphicsCommandList* gbuffer = nullptr;
    ID3D12GraphicsCommandList* lighting = nullptr;
    ID3D12GraphicsCommandList* postfx = nullptr;
};

void main()
{
    tskr::Tasker tasker;

    tasker.add_schedules<Startup>(tskr::ExecutionPolicy::Single)
        .add_schedules<tskr::Parallel<Main, Render>, Sync>(tskr::ExecutionPolicy::Repeat, 4, 0b0011)
        .add_schedules<Shutdown>(tskr::ExecutionPolicy::Single)
        .register_resource(Dx12Device{})
        .register_resource(CommandListPool{})
        .register_resource(FrameState{})
        .register_resource(RenderLists{})
        .run();
}