#include <WinSock2.h>
#include <Windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <algorithm>
#include <d3d11.h>
#include <dwmapi.h>
#include <d3dcompiler.h>
#include <thread>
#include <timeapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")

#include "overlay.hpp"
#include "../deps/imgui/imgui_impl_dx11.h"
#include "../deps/imgui/imgui_impl_win32.h"
#include "../math/math.hpp"
#include "../core/logger.hpp"
#include "font.hpp"
#include "snow.hpp"
#include "../features/visuals.hpp"
#include "../features/rescan.hpp"
#include "../mesh/parse.hpp"
#include "../game/sdk.hpp"
#include "../core/globals.hpp"
#include "../core/detail.hpp"
#include "../ui/web_menu.hpp"



static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static HWND g_hwnd = nullptr;

static ID3D11Texture2D* g_pDepthStencilTexture = nullptr;
static ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
static ID3D11DepthStencilState* g_pDepthStencilState = nullptr;

static ID3D11VertexShader* g_pVertexShader = nullptr;
static ID3D11PixelShader* g_pPixelShader = nullptr;
static ID3D11InputLayout* g_pInputLayout = nullptr;
static ID3D11RasterizerState* g_pRasterizerStateSolid = nullptr;
static ID3D11RasterizerState* g_pRasterizerStateWireframe = nullptr;
static ID3D11BlendState* g_pBlendState = nullptr;

static ID3D11Buffer* g_pVertexBuffer = nullptr;
static ID3D11Buffer* g_pIndexBuffer = nullptr;
static int g_VertexBufferSize = 0;
static int g_IndexBufferSize = 0;

struct CustomVertex {
    float x, y, z, w;
    float r, g, b, a;
};

struct GlobalData {
    float time;
    float res_x;
    float res_y;
    float padding;
};

static ID3D11Buffer* g_pConstantBuffer = nullptr;
static ID3D11PixelShader* g_pWatermarkShader = nullptr;
static ID3D11PixelShader* g_pGraphShader = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

static LRESULT WINAPI overlay_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (ImGui::GetCurrentContext() != nullptr) {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
            return TRUE;
        }
    }
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

// [overlay::CreateRenderTarget]
static void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    

    D3D11_TEXTURE2D_DESC bbDesc;
    pBackBuffer->GetDesc(&bbDesc);
    pBackBuffer->Release();

    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width = bbDesc.Width;
    descDepth.Height = bbDesc.Height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    g_pd3dDevice->CreateTexture2D(&descDepth, NULL, &g_pDepthStencilTexture);


    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(descDSV));
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    g_pd3dDevice->CreateDepthStencilView(g_pDepthStencilTexture, &descDSV, &g_pDepthStencilView);
}

// [overlay::CleanupRenderTarget]
static void CleanupRenderTarget() {
    if (g_pDepthStencilView) { g_pDepthStencilView->Release(); g_pDepthStencilView = nullptr; }
    if (g_pDepthStencilTexture) { g_pDepthStencilTexture->Release(); g_pDepthStencilTexture = nullptr; }
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// [overlay::CreateDeviceD3D]
static bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

// [overlay::CleanupDeviceD3D]
static void CleanupDeviceD3D() {
    CleanupRenderTarget();

    if (g_pDepthStencilState) { g_pDepthStencilState->Release(); g_pDepthStencilState = nullptr; }
    if (g_pVertexShader) { g_pVertexShader->Release(); g_pVertexShader = nullptr; }
    if (g_pPixelShader) { g_pPixelShader->Release(); g_pPixelShader = nullptr; }
    if (g_pInputLayout) { g_pInputLayout->Release(); g_pInputLayout = nullptr; }
    if (g_pRasterizerStateSolid) { g_pRasterizerStateSolid->Release(); g_pRasterizerStateSolid = nullptr; }
    if (g_pRasterizerStateWireframe) { g_pRasterizerStateWireframe->Release(); g_pRasterizerStateWireframe = nullptr; }
    if (g_pBlendState) { g_pBlendState->Release(); g_pBlendState = nullptr; }
    if (g_pVertexBuffer) { g_pVertexBuffer->Release(); g_pVertexBuffer = nullptr; }
    if (g_pIndexBuffer) { g_pIndexBuffer->Release(); g_pIndexBuffer = nullptr; }

    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

namespace render {
    static std::vector<CustomVertex> g_SolidVertices;
    static std::vector<uint32_t> g_SolidIndices;
    
    static std::vector<CustomVertex> g_WireframeVertices;
    static std::vector<uint32_t> g_WireframeIndices;

    // [render::draw_chams_mesh]
    void draw_chams_mesh(const std::vector<mesh_parser::vertex_t>& vertices, const std::vector<mesh_parser::face_t>& faces, const ViewMatrix& vm, const CFrame& cframe, const Vec3& scale, ImU32 color, bool wireframe) {
        if (!g_pd3dDeviceContext || !g_pd3dDevice) return;
        if (vertices.empty() || faces.empty()) return;

        float r = ((color >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f;
        float g = ((color >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f;
        float b = ((color >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f;
        float a = ((color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;

        auto& target_vertices = wireframe ? g_WireframeVertices : g_SolidVertices;
        auto& target_indices = wireframe ? g_WireframeIndices : g_SolidIndices;

        uint32_t offset = static_cast<uint32_t>(target_vertices.size());

        float M[16];
        float cs00 = cframe.r00 * scale.x, cs01 = cframe.r01 * scale.y, cs02 = cframe.r02 * scale.z, cs03 = cframe.x;
        float cs10 = cframe.r10 * scale.x, cs11 = cframe.r11 * scale.y, cs12 = cframe.r12 * scale.z, cs13 = cframe.y;
        float cs20 = cframe.r20 * scale.x, cs21 = cframe.r21 * scale.y, cs22 = cframe.r22 * scale.z, cs23 = cframe.z;
        
        M[0] = vm.data[0]*cs00 + vm.data[1]*cs10 + vm.data[2]*cs20;
        M[1] = vm.data[0]*cs01 + vm.data[1]*cs11 + vm.data[2]*cs21;
        M[2] = vm.data[0]*cs02 + vm.data[1]*cs12 + vm.data[2]*cs22;
        M[3] = vm.data[0]*cs03 + vm.data[1]*cs13 + vm.data[2]*cs23 + vm.data[3];

        M[4] = vm.data[4]*cs00 + vm.data[5]*cs10 + vm.data[6]*cs20;
        M[5] = vm.data[4]*cs01 + vm.data[5]*cs11 + vm.data[6]*cs21;
        M[6] = vm.data[4]*cs02 + vm.data[5]*cs12 + vm.data[6]*cs22;
        M[7] = vm.data[4]*cs03 + vm.data[5]*cs13 + vm.data[6]*cs23 + vm.data[7];

        M[8] = vm.data[8]*cs00 + vm.data[9]*cs10 + vm.data[10]*cs20;
        M[9] = vm.data[8]*cs01 + vm.data[9]*cs11 + vm.data[10]*cs21;
        M[10] = vm.data[8]*cs02 + vm.data[9]*cs12 + vm.data[10]*cs22;
        M[11] = vm.data[8]*cs03 + vm.data[9]*cs13 + vm.data[10]*cs23 + vm.data[11];

        M[12] = vm.data[12]*cs00 + vm.data[13]*cs10 + vm.data[14]*cs20;
        M[13] = vm.data[12]*cs01 + vm.data[13]*cs11 + vm.data[14]*cs21;
        M[14] = vm.data[12]*cs02 + vm.data[13]*cs12 + vm.data[14]*cs22;
        M[15] = vm.data[12]*cs03 + vm.data[13]*cs13 + vm.data[14]*cs23 + vm.data[15];

        for (const auto& v : vertices) {
            float vx = v.position.x;
            float vy = v.position.y;
            float vz = v.position.z;

            float x = vx * M[0] + vy * M[1] + vz * M[2] + M[3];
            float y = vx * M[4] + vy * M[5] + vz * M[6] + M[7];
            float z = vx * M[8] + vy * M[9] + vz * M[10] + M[11];
            float w = vx * M[12] + vy * M[13] + vz * M[14] + M[15];

            target_vertices.push_back({ x, y, z, w, r, g, b, a });
        }

        for (const auto& f : faces) {
            target_indices.push_back(f.a + offset);
            target_indices.push_back(f.b + offset);
            target_indices.push_back(f.c + offset);
        }
    }

    // [render::render_notifications]
    void render_notifications() {
        float current_time = get_real_time();
        float fade_duration = 0.35f;

        ImVec2 ds = ImGui::GetIO().DisplaySize;
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        auto& notifs = get_notifications();

        float y_offset = 0.0f;
        for (auto it = notifs.begin(); it != notifs.end(); ) {
            float age = current_time - it->spawn_time;
            if (age > it->duration) { 
                it = notifs.erase(it); 
                continue; 
            }

            float anim = 1.0f;
            if (age < fade_duration) anim = age / fade_duration;
            else if (age > it->duration - fade_duration) anim = (it->duration - age) / fade_duration;
            
            anim = (std::max)(0.0f, (std::min)(1.0f, anim));
            float alpha = anim * anim * (3.0f - 2.0f * anim);
            
            ImVec2 txt_sz = ImGui::CalcTextSize(it->text.c_str());
            float pad = 12.0f;
            ImVec2 size(txt_sz.x + pad * 2.0f, 28.0f);
            ImVec2 pos((ds.x - size.x) * 0.5f, ds.y * 0.75f + y_offset - (1.0f - alpha) * 15.0f);

            dl->AddRectFilled(ImVec2(pos.x + 2, pos.y + 2), ImVec2(pos.x + size.x + 2, pos.y + size.y + 2), to_color(0, 0, 0, (int)(110 * alpha)), 6.0f);

            dl->AddCallback([](const ImDrawList*, const ImDrawCmd*) {
                if (g_pGraphShader) g_pd3dDeviceContext->PSSetShader(g_pGraphShader, nullptr, 0);
            }, nullptr);
            dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), to_color(255, 255, 255, (int)(248 * alpha)), 6.0f);
            dl->AddCallback(ImDrawCallback_ResetRenderState, nullptr);

            dl->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), to_color(70, 70, 70, (int)(255 * alpha)), 6.0f, 0, 1.5f);

            float ty = pos.y + (size.y - ImGui::GetFontSize()) * 0.5f;
            dl->AddText(ImVec2(pos.x + pad, ty), to_color(255, 255, 255, (int)(255 * alpha)), it->text.c_str());

            y_offset += (size.y + 8.0f);
            ++it;
        }
    }

    // [render::flush_chams_meshes]
    void flush_chams_meshes() {
        if (!g_pd3dDeviceContext || !g_pd3dDevice) return;

        auto flush_batch = [](std::vector<CustomVertex>& verts, std::vector<uint32_t>& inds, bool is_wireframe) {
            if (verts.empty() || inds.empty()) return;

            if (!g_pVertexBuffer || g_VertexBufferSize < verts.size()) {
                if (g_pVertexBuffer) g_pVertexBuffer->Release();
                g_VertexBufferSize = static_cast<int>(verts.size()) + 5000;
                D3D11_BUFFER_DESC desc = {};
                desc.Usage = D3D11_USAGE_DYNAMIC;
                desc.ByteWidth = sizeof(CustomVertex) * g_VertexBufferSize;
                desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                g_pd3dDevice->CreateBuffer(&desc, nullptr, &g_pVertexBuffer);
            }

            if (!g_pIndexBuffer || g_IndexBufferSize < inds.size()) {
                if (g_pIndexBuffer) g_pIndexBuffer->Release();
                g_IndexBufferSize = static_cast<int>(inds.size()) + 15000;
                D3D11_BUFFER_DESC desc = {};
                desc.Usage = D3D11_USAGE_DYNAMIC;
                desc.ByteWidth = sizeof(uint32_t) * g_IndexBufferSize;
                desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                g_pd3dDevice->CreateBuffer(&desc, nullptr, &g_pIndexBuffer);
            }

            if (!g_pVertexBuffer || !g_pIndexBuffer) {
                verts.clear();
                inds.clear();
                return;
            }

            D3D11_MAPPED_SUBRESOURCE mapped;
            if (SUCCEEDED(g_pd3dDeviceContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
                memcpy(mapped.pData, verts.data(), verts.size() * sizeof(CustomVertex));
                g_pd3dDeviceContext->Unmap(g_pVertexBuffer, 0);
            }
            if (SUCCEEDED(g_pd3dDeviceContext->Map(g_pIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
                memcpy(mapped.pData, inds.data(), inds.size() * sizeof(uint32_t));
                g_pd3dDeviceContext->Unmap(g_pIndexBuffer, 0);
            }

            Vector2 viewport_size = math::get_viewport_size();
            D3D11_VIEWPORT vp = {};
            vp.Width = viewport_size.x;
            vp.Height = viewport_size.y;
            vp.MinDepth = 0.0f;
            vp.MaxDepth = 1.0f;
            vp.TopLeftX = 0;
            vp.TopLeftY = 0;
            g_pd3dDeviceContext->RSSetViewports(1, &vp);

            UINT stride = sizeof(CustomVertex);
            UINT offset = 0;
            g_pd3dDeviceContext->IASetInputLayout(g_pInputLayout);
            g_pd3dDeviceContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
            g_pd3dDeviceContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
            g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            g_pd3dDeviceContext->VSSetShader(g_pVertexShader, nullptr, 0);
            g_pd3dDeviceContext->PSSetShader(g_pPixelShader, nullptr, 0);
            
            g_pd3dDeviceContext->RSSetState(is_wireframe ? g_pRasterizerStateWireframe : g_pRasterizerStateSolid);
            g_pd3dDeviceContext->OMSetDepthStencilState(g_pDepthStencilState, 1);
            
            float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
            g_pd3dDeviceContext->OMSetBlendState(g_pBlendState, blendFactor, 0xffffffff);

            g_pd3dDeviceContext->DrawIndexed(static_cast<UINT>(inds.size()), 0, 0);

            verts.clear();
            inds.clear();
        };

        flush_batch(g_SolidVertices, g_SolidIndices, false);
        flush_batch(g_WireframeVertices, g_WireframeIndices, true);
    }
}

namespace overlay {
    static std::atomic<float> g_ping_val{ 0.0f };
    static std::jthread g_ping_thread;

    // [overlay::initialize]
    bool initialize() {
        timeBeginPeriod(1);
        Vector2 viewport = math::get_viewport_size();
        int width = static_cast<int>(viewport.x);
        int height = static_cast<int>(viewport.y);

        if (width <= 0 || height <= 0) {
            HDC hDC = GetDC(NULL);
            width = GetDeviceCaps(hDC, HORZRES);
            height = GetDeviceCaps(hDC, VERTRES);
            ReleaseDC(NULL, hDC);
        }

        std::wstring random_class = detail::random_wstring(16);
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, overlay_wnd_proc, 0L, 0L,
                          GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                          random_class.c_str(), NULL };
        RegisterClassExW(&wc);

        g_hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
            wc.lpszClassName,
            L"Overlay",
            WS_POPUP,
            0, 0, width, height,
            NULL, NULL, wc.hInstance, NULL);

        if (!g_hwnd) return false;

        MARGINS margins = { -1 };
        DwmExtendFrameIntoClientArea(g_hwnd, &margins);
        SetLayeredWindowAttributes(g_hwnd, 0, 255, LWA_ALPHA);

        if (!CreateDeviceD3D(g_hwnd)) {
            CleanupDeviceD3D();
            UnregisterClassW(wc.lpszClassName, wc.hInstance);
            return false;
        }

        const char* shaderCode = R"(
        struct VS_IN {
            float4 pos : POSITION;
            float4 col : COLOR;
        };
        struct PS_IN {
            float4 pos : SV_POSITION;
            float4 col : COLOR;
        };
        PS_IN VS(VS_IN input) {
            PS_IN output;
            output.pos = input.pos;
            output.col = input.col;
            return output;
        }
        float4 PS(PS_IN input) : SV_Target {
            return input.col;
        }
        )";

        const char* watermarkShaderCode = R"(
        cbuffer GlobalData : register(b0) {
            float time;
            float res_x;
            float res_y;
            float padding;
        };

        struct PS_IN {
            float4 pos : SV_POSITION;
            float4 col : COLOR;
        };

        float3 hsv2rgb(float3 c) {
            float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
            float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
            return c.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
        }

        float4 PS(PS_IN input) : SV_Target {
            float2 uv = input.pos.xy / float2(res_x, res_y);
            float hue = frac(uv.x * 2.5 + time * 0.4);
            float3 base = hsv2rgb(float3(0.75 + sin(time * 0.2) * 0.05, 0.65, 0.9));
            float3 flow = hsv2rgb(float3(0.5 + 0.1 * sin(uv.x * 6.0 + time * 2.0), 0.7, 1.0));
            
            float3 combined = lerp(base, flow, 0.35 + 0.35 * sin(uv.x * 4.0 + time * 2.0));
            return float4(combined, input.col.a);
        }
        )";

        const char* graphShaderCode = R"(
        cbuffer GlobalData : register(b0) {
            float time;
            float res_x;
            float res_y;
            float padding;
        };

        struct PS_IN {
            float4 pos : SV_POSITION;
            float4 col : COLOR;
        };

        float4 PS(PS_IN input) : SV_Target {
            float2 uv = input.pos.xy / float2(res_x, res_y);
            float2 grid_uv = uv * float2(20.0, 10.0);
            grid_uv.x += time * 0.5;
            float2 grid = abs(frac(grid_uv - 0.5) - 0.5) / fwidth(grid_uv);
            float line_weight = min(grid.x, grid.y);
            float grid_val = 1.0 - min(line_weight, 1.0);
            
            float3 base_bg = float3(0.02, 0.02, 0.05);
            float3 grid_color = float3(0.1, 0.2, 0.4) * grid_val * 0.3;
            
            float scanline = sin(uv.y * 200.0 + time * 5.0) * 0.02;
            
            float3 final_color = base_bg + grid_color + scanline;
            float alpha = input.col.a;
            
            return float4(final_color, alpha);
        }
        )";

        ID3DBlob* pVSBlob = nullptr;
        ID3DBlob* pPSBlob = nullptr;
        ID3DBlob* pWMPSBlob = nullptr;
        ID3DBlob* pGRPSBlob = nullptr;
        ID3DBlob* pErrorBlob = nullptr;

        D3DCompile(shaderCode, strlen(shaderCode), "Shader", nullptr, nullptr, "VS", "vs_4_0", 0, 0, &pVSBlob, &pErrorBlob);
        if (pErrorBlob) pErrorBlob->Release();
        D3DCompile(shaderCode, strlen(shaderCode), "Shader", nullptr, nullptr, "PS", "ps_4_0", 0, 0, &pPSBlob, &pErrorBlob);
        if (pErrorBlob) pErrorBlob->Release();
        D3DCompile(watermarkShaderCode, strlen(watermarkShaderCode), "Watermark", nullptr, nullptr, "PS", "ps_4_0", 0, 0, &pWMPSBlob, &pErrorBlob);
        if (pErrorBlob) pErrorBlob->Release();
        D3DCompile(graphShaderCode, strlen(graphShaderCode), "Graph", nullptr, nullptr, "PS", "ps_4_0", 0, 0, &pGRPSBlob, &pErrorBlob);
        if (pErrorBlob) pErrorBlob->Release();

        if (pVSBlob && pPSBlob && pWMPSBlob && pGRPSBlob) {
            g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
            g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
            g_pd3dDevice->CreatePixelShader(pWMPSBlob->GetBufferPointer(), pWMPSBlob->GetBufferSize(), nullptr, &g_pWatermarkShader);
            g_pd3dDevice->CreatePixelShader(pGRPSBlob->GetBufferPointer(), pGRPSBlob->GetBufferSize(), nullptr, &g_pGraphShader);

            D3D11_INPUT_ELEMENT_DESC layout[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }
            };
            g_pd3dDevice->CreateInputLayout(layout, 2, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pInputLayout);
        }

        if (pVSBlob) pVSBlob->Release();
        if (pPSBlob) pPSBlob->Release();
        if (pWMPSBlob) pWMPSBlob->Release();
        if (pGRPSBlob) pGRPSBlob->Release();
        if (pErrorBlob) pErrorBlob->Release();

        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.ByteWidth = sizeof(GlobalData);
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        g_pd3dDevice->CreateBuffer(&cbDesc, nullptr, &g_pConstantBuffer);

        D3D11_DEPTH_STENCIL_DESC dsDesc = {};
        dsDesc.DepthEnable = true;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
        g_pd3dDevice->CreateDepthStencilState(&dsDesc, &g_pDepthStencilState);

        D3D11_RASTERIZER_DESC rsDesc = {};
        rsDesc.FillMode = D3D11_FILL_SOLID;
        rsDesc.CullMode = D3D11_CULL_NONE;
        g_pd3dDevice->CreateRasterizerState(&rsDesc, &g_pRasterizerStateSolid);

        rsDesc.FillMode = D3D11_FILL_WIREFRAME;
        g_pd3dDevice->CreateRasterizerState(&rsDesc, &g_pRasterizerStateWireframe);

        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = true;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        g_pd3dDevice->CreateBlendState(&blendDesc, &g_pBlendState);

        ShowWindow(g_hwnd, SW_SHOWDEFAULT);
        UpdateWindow(g_hwnd);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.IniFilename = nullptr;

        ImFontConfig font_config;
        font_config.FontDataOwnedByAtlas = false;

        float font_size = 16.0f;

        io.Fonts->AddFontFromMemoryTTF(
            (void*)Sora_VariableFont_wght,
            sizeof(Sora_VariableFont_wght),
            font_size,
            &font_config
        );

        ImGui::StyleColorsDark();

        ImGui_ImplWin32_Init(g_hwnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

        snow::initialize();
        mesh_parser::initialize();

        g_ping_thread = std::jthread([](std::stop_token stop) {
            while (!stop.stop_requested() && globals::running) {
                HANDLE h = IcmpCreateFile();
                if (h != INVALID_HANDLE_VALUE) {
                    IPAddr ip = 0x01010101; 
                    char d[] = "p";
                    char b[sizeof(ICMP_ECHO_REPLY) + sizeof(d) + 8];
                    if (IcmpSendEcho(h, ip, d, sizeof(d), NULL, b, sizeof(b), 1000) != 0) {
                        g_ping_val.store((float)((PICMP_ECHO_REPLY)b)->RoundTripTime);
                    }
                    IcmpCloseHandle(h);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });

        ui::web_menu::initialize(g_hwnd);

        return true;
    }



    struct performance_data {
        static constexpr int BUFFER_SIZE = 1500;
        float fps_history[BUFFER_SIZE] = { 0 };
        float intstat_history[BUFFER_SIZE] = { 0 };
        float ping_history[BUFFER_SIZE] = { 0 };
        int offset = 0;
        float lerped_max_fps = 360.0f;
        float lerped_max_intstat = 15.0f;
        float lerped_max_ping = 150.0f;

        // [performance_data::update]
        void update(float fps, float intstat, float ping) {
            fps_history[offset] = fps;
            intstat_history[offset] = intstat;
            ping_history[offset] = ping;
            offset = (offset + 1) % BUFFER_SIZE;
        }
    };

    static performance_data g_perf_data;

    // [overlay::draw_graph_section]
    static void draw_graph_section(ImDrawList* draw_list, float* data, int count, int offset, float& lerped_max, float base_max_val, const char* title, ImVec2 section_pos, ImVec2 section_size, float left_padding = 8.0f) {
        ImVec2 text_size = ImGui::CalcTextSize(title);
        draw_list->AddText(ImVec2(section_pos.x + left_padding, section_pos.y + (section_size.y - text_size.y) * 0.5f), ::render::to_color(200, 200, 200, 210), title);

        float graph_x = section_pos.x + left_padding + text_size.x + 8.0f; 
        float graph_w = 80.0f; 
        float graph_y = section_pos.y + 5;
        float graph_h = section_size.y - 12;
        
        const int draw_count = 45; 
        float step = graph_w / (draw_count - 1);
        int sample_window = count / draw_count; 

        float current_max = 0.001f;
        for (int i = 0; i < count; i++) {
            if (data[i] > current_max) current_max = data[i];
        }

        float adaptive_floor = base_max_val * 0.05f; 
        float delta = ImGui::GetIO().DeltaTime;
        float lerp_speed = (current_max > lerped_max) ? 12.0f : 3.0f;
        lerped_max = lerped_max + (current_max - lerped_max) * (delta * lerp_speed);
        lerped_max = (std::max)(lerped_max, adaptive_floor);

        auto get_sample = [&](int idx) {
            float avg = 0.0f;
            int start = (offset + idx * sample_window) % count;
            for (int j = 0; j < sample_window; j++) avg += data[(start + j) % count];
            return avg / sample_window;
        };

        ImU32 white_main = ::render::to_color(255, 255, 255, 255);
        ImU32 white_glow = ::render::to_color(255, 255, 255, 75);

        for (int i = 0; i < draw_count - 1; i++) {
            float v1 = get_sample(i) / lerped_max;
            float v2 = get_sample(i+1) / lerped_max;
            
            v1 = (v1 * 0.75f) + 0.125f;
            v2 = (v2 * 0.75f) + 0.125f;

            ImVec2 p1(graph_x + i * step, graph_y + graph_h - v1 * graph_h);
            ImVec2 p2(graph_x + (i + 1) * step, graph_y + graph_h - v2 * graph_h);
            
            draw_list->AddLine(p1, p2, white_glow, 3.5f);
            draw_list->AddLine(p1, p2, white_main, 1.8f);
        }
    }

    // [overlay::render_performance_graph]
    static void render_performance_graph(ImVec2& graph_pos, bool& graph_dragging) {
        if (!globals::settings::performance_graph.load()) return;

        ImGuiIO& io = ImGui::GetIO();
        float fps = io.Framerate;
        
        float cache_d = globals::settings::player_cache_delay.load();
        float aim_d = globals::settings::aim_thread_delay.load();
        float rescan_d = globals::settings::rescan_thread_delay.load();
        float intstat = (cache_d + aim_d + rescan_d) / 3.0f;

        float ping = g_ping_val.load();
        
        g_perf_data.update(fps, intstat, ping);

        float stats_text_w = ImGui::CalcTextSize(HIDE_STR("stats").c_str()).x;
        float ping_text_w = ImGui::CalcTextSize(HIDE_STR("ping").c_str()).x;
        
        float sec1_w = 10.0f + stats_text_w + 8.0f + 80.0f + 5.0f; 
        float sec2_w = 5.0f + ping_text_w + 8.0f + 80.0f + 10.0f;
        float total_width = sec1_w + sec2_w;

        ImVec2 size(total_width, 35.0f); 
        ImVec2 pos = graph_pos;

        // Drag logic when menu is open
        if (globals::menu_open.load()) {
            ImVec2 mouse = io.MousePos;
            bool hovered = mouse.x >= pos.x && mouse.y >= pos.y
                        && mouse.x <= pos.x + size.x && mouse.y <= pos.y + size.y;
            if (hovered && ImGui::IsMouseClicked(0)) {
                graph_dragging = true;
            }
            if (graph_dragging) {
                if (ImGui::IsMouseDown(0)) {
                    graph_pos.x += io.MouseDelta.x;
                    graph_pos.y += io.MouseDelta.y;
                    ImVec2 ds = io.DisplaySize;
                    if (graph_pos.x < 0.0f) graph_pos.x = 0.0f;
                    if (graph_pos.y < 0.0f) graph_pos.y = 0.0f;
                    if (graph_pos.x + size.x > ds.x) graph_pos.x = ds.x - size.x;
                    if (graph_pos.y + size.y > ds.y) graph_pos.y = ds.y - size.y;
                    pos = graph_pos;
                } else {
                    graph_dragging = false;
                }
            }
        } else {
            graph_dragging = false;
        }

        ImDrawList* draw_list = ImGui::GetForegroundDrawList();

        // Side detection: shadow falls away from nearest edge
        float screen_mid_x = io.DisplaySize.x * 0.5f;
        float shadow_ox = (pos.x + size.x * 0.5f < screen_mid_x) ? 2.0f : -2.0f;
        float shadow_oy = 2.0f;

        draw_list->AddRectFilled(ImVec2(pos.x + shadow_ox, pos.y + shadow_oy), ImVec2(pos.x + size.x + shadow_ox, pos.y + size.y + shadow_oy), ::render::to_color(0, 0, 0, 110), 6.0f);
        
        draw_list->AddCallback([](const ImDrawList* parent_list, const ImDrawCmd* cmd) {
            if (g_pGraphShader) g_pd3dDeviceContext->PSSetShader(g_pGraphShader, nullptr, 0);
        }, nullptr);

        draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ::render::to_color(255, 255, 255, 248), 6.0f);
        draw_list->AddCallback(ImDrawCallback_ResetRenderState, nullptr);

        draw_list->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), ::render::to_color(70, 70, 70, 255), 6.0f, 0, 1.5f);

        draw_graph_section(draw_list, g_perf_data.intstat_history, performance_data::BUFFER_SIZE, g_perf_data.offset, g_perf_data.lerped_max_intstat, 15.0f, HIDE_STR("stats").c_str(), pos, ImVec2(sec1_w, size.y), 10.0f);
        draw_graph_section(draw_list, g_perf_data.ping_history, performance_data::BUFFER_SIZE, g_perf_data.offset, g_perf_data.lerped_max_ping, 150.0f, HIDE_STR("ping").c_str(), ImVec2(pos.x + sec1_w, pos.y), ImVec2(sec2_w, size.y), 10.0f);
    }

    // [overlay::update_click_through]
    static void update_click_through(bool can_display_overlay) {
        static bool last_clickable = false;

        bool is_any_interaction = ImGui::GetCurrentContext() && (ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered());
        bool should_be_clickable = can_display_overlay && (globals::menu_open || is_any_interaction);

        if (should_be_clickable != last_clickable) {
            LONG_PTR exStyle = GetWindowLongPtr(g_hwnd, GWL_EXSTYLE);
            if (should_be_clickable) {
                exStyle &= ~WS_EX_TRANSPARENT;
            }
            else {
                exStyle |= WS_EX_TRANSPARENT;
            }
            SetWindowLongPtr(g_hwnd, GWL_EXSTYLE, exStyle);
            SetWindowPos(g_hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            last_clickable = should_be_clickable;
        }
    }

    // [overlay::is_overlay_allowed]
    static bool is_overlay_allowed(HWND roblox_hwnd, HWND overlay_hwnd) {
        HWND active_window = GetForegroundWindow();
        if (roblox_hwnd && IsWindow(roblox_hwnd) && active_window == roblox_hwnd) return true;
        if (overlay_hwnd && IsWindow(overlay_hwnd) && active_window == overlay_hwnd && ui::web_menu::is_open()) return true;

        return false;
    }

    // [overlay::render_loop]
    void render_loop() {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        {
            GlobalData gd;
            gd.time = (float)render::get_real_time();
            gd.res_x = ImGui::GetIO().DisplaySize.x;
            gd.res_y = ImGui::GetIO().DisplaySize.y;
            gd.padding = 0;

            D3D11_MAPPED_SUBRESOURCE mapped;
            if (SUCCEEDED(g_pd3dDeviceContext->Map(g_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
                memcpy(mapped.pData, &gd, sizeof(gd));
                g_pd3dDeviceContext->Unmap(g_pConstantBuffer, 0);
            }
            g_pd3dDeviceContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
        }

        static HWND rbx_hwnd = nullptr;
        static auto last_sync_time = std::chrono::steady_clock::now();
        static int last_w = 0, last_h = 0;
        static ImVec2 g_watermark_pos(30.0f, 55.0f);
        static bool g_watermark_dragging = false;
        static ImVec2 g_graph_pos(30.0f, 86.0f);
        static bool g_graph_dragging = false;
        auto now = std::chrono::steady_clock::now();

        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_sync_time).count() > 300) {
            rbx_hwnd = FindWindowW(NULL, L"Roblox");
            globals::ov_win.store(rbx_hwnd);

            if (rbx_hwnd) {
                Vector2 viewport = math::get_viewport_size();
                int width = static_cast<int>(viewport.x);
                int height = static_cast<int>(viewport.y);

                RECT client_rect;
                if (GetClientRect(rbx_hwnd, &client_rect)) {
                    POINT top_left = { 0, 0 };
                    ClientToScreen(rbx_hwnd, &top_left);

                    int client_w = client_rect.right - client_rect.left;
                    int client_h = client_rect.bottom - client_rect.top;

                    if (width != last_w || height != last_h) {
                        CleanupRenderTarget();
                        g_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
                        CreateRenderTarget();
                        
                        last_w = width;
                        last_h = height;
                        ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));

                        SetWindowPos(g_hwnd, HWND_TOPMOST, top_left.x, top_left.y, client_w, client_h, SWP_NOACTIVATE);
                    } else {
                        SetWindowPos(g_hwnd, HWND_TOPMOST, top_left.x, top_left.y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
                    }
                }
            }
            last_sync_time = now;
        }

        bool can_display_overlay = is_overlay_allowed(globals::ov_win.load(), g_hwnd);

        ui::web_menu::process_input(g_hwnd, rbx_hwnd, can_display_overlay);




        update_click_through(can_display_overlay);

        if (can_display_overlay) {
            if (globals::settings::watermark.load()) {
                float fps = ImGui::GetIO().Framerate;
                std::string fps_val = std::to_string(static_cast<int>(fps));
                
                float violet_w = ImGui::CalcTextSize(HIDE_STR("violet").c_str()).x;
                float fps_label_w = ImGui::CalcTextSize(HIDE_STR("fps").c_str()).x;
                float fps_val_w = ImGui::CalcTextSize(fps_val.c_str()).x;
                
                float padding = 10.0f;
                float inner_gap = 18.0f;
                float value_gap = 4.0f;
                
                float total_w = padding + violet_w + inner_gap + fps_label_w + value_gap + fps_val_w + padding;
                float fixed_w = (std::max)(130.0f, total_w);

                float h = (globals::settings::watermark_style.load() == 0) ? 26.0f : globals::settings::watermark_height.load();
                float rounding = (globals::settings::watermark_style.load() == 0) ? 6.0f : globals::settings::watermark_rounding.load();

                ImVec2 pos = g_watermark_pos;
                ImVec2 fixed_size(fixed_w, h);

                if (globals::menu_open.load()) {
                    ImGuiIO& io = ImGui::GetIO();
                    ImVec2 mouse = io.MousePos;
                    bool hovered = mouse.x >= pos.x && mouse.y >= pos.y
                                && mouse.x <= pos.x + fixed_size.x && mouse.y <= pos.y + fixed_size.y;
                    if (hovered && ImGui::IsMouseClicked(0)) {
                        g_watermark_dragging = true;
                    }
                    if (g_watermark_dragging) {
                        if (ImGui::IsMouseDown(0)) {
                            g_watermark_pos.x += io.MouseDelta.x;
                            g_watermark_pos.y += io.MouseDelta.y;
                            ImVec2 ds = io.DisplaySize;
                            if (g_watermark_pos.x < 0.0f) g_watermark_pos.x = 0.0f;
                            if (g_watermark_pos.y < 0.0f) g_watermark_pos.y = 0.0f;
                            if (g_watermark_pos.x + fixed_size.x > ds.x) g_watermark_pos.x = ds.x - fixed_size.x;
                            if (g_watermark_pos.y + fixed_size.y > ds.y) g_watermark_pos.y = ds.y - fixed_size.y;
                            pos = g_watermark_pos;
                        } else {
                            g_watermark_dragging = false;
                        }
                    }
                } else {
                    g_watermark_dragging = false;
                }

                ImDrawList* draw_list = ImGui::GetForegroundDrawList();

                float screen_mid_x = ImGui::GetIO().DisplaySize.x * 0.5f;
                float shadow_ox = (pos.x + fixed_size.x * 0.5f < screen_mid_x) ? 2.0f : -2.0f;
                float shadow_oy = 2.0f;

                ImU32 border_col = ::render::to_color(70, 70, 70, 255);

                draw_list->AddRectFilled(ImVec2(pos.x + shadow_ox, pos.y + shadow_oy), ImVec2(pos.x + fixed_size.x + shadow_ox, pos.y + fixed_size.y + shadow_oy), render::to_color(0, 0, 0, 110), rounding);

                draw_list->AddCallback([](const ImDrawList* parent_list, const ImDrawCmd* cmd) {
                    if (g_pGraphShader) g_pd3dDeviceContext->PSSetShader(g_pGraphShader, nullptr, 0);
                }, nullptr);
                draw_list->AddRectFilled(pos, ImVec2(pos.x + fixed_size.x, pos.y + fixed_size.y), render::to_color(255, 255, 255, 248), rounding);
                draw_list->AddCallback(ImDrawCallback_ResetRenderState, nullptr);

                draw_list->AddRect(pos, ImVec2(pos.x + fixed_size.x, pos.y + fixed_size.y), border_col, rounding, 0, 1.5f);

                float text_h = ImGui::GetFontSize();
                float text_y = pos.y + (fixed_size.y - text_h) * 0.5f;
                ImU32 violet_main = render::to_color(200, 130, 255, 255);
                ImU32 violet_glow = render::to_color(200, 130, 255, 80);
                ImU32 label_color = render::to_color(200, 200, 200, 210);
                ImU32 value_color = render::to_color(255, 255, 255, 255);

                ImVec2 violet_pos(pos.x + padding, text_y);
                draw_list->AddText(ImVec2(violet_pos.x, violet_pos.y), violet_glow, HIDE_STR("violet").c_str());
                draw_list->AddText(violet_pos, violet_main, HIDE_STR("violet").c_str());

                float fps_x = violet_pos.x + violet_w + inner_gap;
                draw_list->AddText(ImVec2(fps_x, text_y), label_color, HIDE_STR("fps").c_str());
                draw_list->AddText(ImVec2(fps_x + fps_label_w + value_gap, text_y), value_color, fps_val.c_str());
            }

            render_performance_graph(g_graph_pos, g_graph_dragging);

            render::render_notifications();
            static bool rendered = false;

            if (!rendered) {
                render::add_notification(HIDE_STR("Attached Successfully!"), 3.0f);
                rendered = true;
            }

            snow::update_and_render();
            visuals::run();

            Vector2 mouse_center = { 0, 0 };
            if (rbx_hwnd) {
                POINT pt;
                if (GetCursorPos(&pt)) {
                    ScreenToClient(rbx_hwnd, &pt);
                    mouse_center = { static_cast<float>(pt.x), static_cast<float>(pt.y) };
                }
            }

            if (globals::aim::aimbot_enabled.load() && globals::aim::aimbot_draw_fov.load()) {
                Vector2 center = mouse_center;
                float fov_radius = globals::aim::aimbot_fov.load();
                ImU32 fov_col = render::to_color(255, 255, 255, 255);

                float pulse = sinf(render::get_real_time() * 2.5f) * 1.5f;
                render::fov_circle(center, fov_radius + pulse, fov_col, 1.25f);
            }

            if (globals::aim::silentaim_enabled.load() && globals::aim::silentaim_draw_fov.load()) {
                Vector2 center = mouse_center;
                float fov_radius = globals::aim::silentaim_fov.load();
                ImU32 fov_col = render::to_color(255, 0, 0, 255);

                float pulse = sinf(render::get_real_time() * 2.5f) * 1.5f;
                render::fov_circle(center, fov_radius + pulse, fov_col, 1.25f);
            }
        }




        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, g_pDepthStencilView);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        if (g_pDepthStencilView) {
            g_pd3dDeviceContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        }
        
        render::flush_chams_meshes();


        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(0, 0);

        if (globals::settings::fps_limit_enabled.load()) {
            int limit = globals::settings::fps_limit.load();
            if (limit > 0 && limit < 1000) {
                static auto next_frame = std::chrono::steady_clock::now();
                auto target_duration = std::chrono::microseconds(1000000 / limit);
                auto now = std::chrono::steady_clock::now();

                if (now > next_frame + std::chrono::milliseconds(20)) {
                    next_frame = now;
                }

                while ((now = std::chrono::steady_clock::now()) < next_frame) {
                    auto diff_us = std::chrono::duration_cast<std::chrono::microseconds>(next_frame - now).count();
                    if (diff_us > 1200) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                    else if (diff_us > 400) {
                        std::this_thread::yield();
                    }
                }
                next_frame += target_duration;
            }
        }
    }
    // [overlay::shutdown]
    void shutdown() {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        ui::web_menu::shutdown();


        CleanupDeviceD3D();
        if (g_hwnd) {
            wchar_t class_name[256];
            GetClassNameW(g_hwnd, class_name, 256);
            DestroyWindow(g_hwnd);
            UnregisterClassW(class_name, GetModuleHandle(NULL));
        }
        timeEndPeriod(1);
    }
}
