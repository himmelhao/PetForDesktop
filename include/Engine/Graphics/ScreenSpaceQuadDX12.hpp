#pragma once

#include "Engine/Graphics/WindowDX12.hpp"
#include "Engine/Log.hpp"
#include "d3dx12/d3dx12.h"
#include <d3d12.h> // for D3D12 interface
#include <wrl.h>   // for Microsoft::WRL::ComPtr

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// see: https://www.braynzarsoft.net/viewtutorial/q16390-directx-12-index-buffers
// TODO full screen Triangle
class ScreenSpaceQuad
{
protected:
    // GPU memory for vertex data
    ComPtr<ID3D12Resource> m_vertexBuffer;

    // GPU memory for index data
    ComPtr<ID3D12Resource> m_indexBuffer;

public:
    ScreenSpaceQuad(Window& win, float minPos = -1.f, float maxPos = 1.f)
    {
        // TODO: Pos vec2 ?
        const float vBuffer[] = {
            // positions        // texture coords
            maxPos, maxPos, 0.0f, 1.0f, 1.0f, // top right
            maxPos, minPos, 0.0f, 1.0f, 0.0f, // bottom right
            minPos, minPos, 0.0f, 0.0f, 0.0f, // bottom left
            minPos, maxPos, 0.0f, 0.0f, 1.0f  // top left
        };

        createVertexBuffer(win, minPos, maxPos, vBuffer);

        const unsigned int iBuffer[] = {
            0, 1, 3, // first triangle
            1, 2, 3  // second triangle
        };

        createIndexBuffer(win, iBuffer);
    }

    HRESULT createVertexBuffer(Window& win, float minPos, float maxPos, const float* vBuffer)
    {
        HRESULT hr;

        const const int vBufferSize = sizeof(vBuffer);

        // create default heap
        // default heap is memory on the GPU. Only the GPU has access to this memory
        // To get data into this heap, we will have to upload the data using
        // an upload heap
        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC   buffer = CD3DX12_RESOURCE_DESC::Buffer(vBufferSize);
        V_RETURN(win.getDevice()->CreateCommittedResource(
            &heapProperties, D3D12_HEAP_FLAG_NONE, &buffer,
            D3D12_RESOURCE_STATE_COPY_DEST, // we will start this heap in the copy destination state since we will copy
                                            // data from the upload heap to this heap
            nullptr, IID_PPV_ARGS(&m_vertexBuffer)))

        V_RETURN(m_vertexBuffer->SetName(L"Vertex Buffer Resource Heap"))

        // create upload heap
        // upload heaps are used to upload data to the GPU. CPU can write to it, GPU can read from it
        // We will upload the vertex buffer using this heap to the default heap
        ComPtr<ID3D12Resource> vBufferUploadHeap;
        heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        buffer         = CD3DX12_RESOURCE_DESC::Buffer(vBufferSize);
        V_RETURN(win.getDevice()->CreateCommittedResource(
            &heapProperties, D3D12_HEAP_FLAG_NONE, &buffer,
            D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default
                                               // heap
            nullptr, IID_PPV_ARGS(&vBufferUploadHeap)))
        V_RETURN(vBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap"))

        // store vertex buffer in upload heap
        D3D12_SUBRESOURCE_DATA vertexData = {};
        vertexData.pData                  = reinterpret_cast<const BYTE*>(vBuffer);
        vertexData.RowPitch               = vBufferSize;
        vertexData.SlicePitch             = vBufferSize;

        // we are now creating a command with the command list to copy the data from
        // the upload heap to the default heap
        UpdateSubresources(win.getCommandList(), m_vertexBuffer.Get(), vBufferUploadHeap.Get(), 0, 0, 1, &vertexData);

        // transition the vertex buffer data from copy destination state to vertex buffer state
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        win.getCommandList()->ResourceBarrier(1, &barrier);

        return 0;
    }

    HRESULT createIndexBuffer(Window& win, const unsigned int* iBuffer)
    {
        HRESULT         hr;
        const const int iBufferSize = sizeof(iBuffer);

        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC   buffer = CD3DX12_RESOURCE_DESC::Buffer(iBufferSize);
        // create default heap to hold index buffer
        V_RETURN(win.getDevice()->CreateCommittedResource(
            &heapProperties, D3D12_HEAP_FLAG_NONE, &buffer,
            D3D12_RESOURCE_STATE_COPY_DEST, // start in the copy destination state
            nullptr,                        // optimized clear value must be null for this type of resource
            IID_PPV_ARGS(&m_indexBuffer)))

        // we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are
        // looking at
        V_RETURN(m_indexBuffer->SetName(L"Index Buffer Resource Heap"))

        // create upload heap to upload index buffer
        ComPtr<ID3D12Resource> iBufferUploadHeap;
        heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        buffer         = CD3DX12_RESOURCE_DESC::Buffer(iBufferSize);
        V_RETURN(win.getDevice()->CreateCommittedResource(
            &heapProperties, D3D12_HEAP_FLAG_NONE, &buffer,
            D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default
                                               // heap
            nullptr, IID_PPV_ARGS(&iBufferUploadHeap)))
        V_RETURN(iBufferUploadHeap->SetName(L"Index Buffer Upload Resource Heap"))

        // store vertex buffer in upload heap
        D3D12_SUBRESOURCE_DATA indexData = {};
        indexData.pData                  = reinterpret_cast<const BYTE*>(iBuffer); // pointer to our index array
        indexData.RowPitch               = iBufferSize;                            // size of all our index buffer
        indexData.SlicePitch             = iBufferSize;                            // also the size of our index buffer

        // we are now creating a command with the command list to copy the data from
        // the upload heap to the default heap
        UpdateSubresources(win.getCommandList(), m_indexBuffer.Get(), iBufferUploadHeap.Get(), 0, 0, 1, &indexData);

        // transition the vertex buffer data from copy destination state to vertex buffer state
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        win.getCommandList()->ResourceBarrier(1, &barrier);

        return 0;
    }

    ~ScreenSpaceQuad()
    {
    }

    void use()
    {
    }

    void draw()
    {
    }
};