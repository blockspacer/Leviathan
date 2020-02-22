// ------------------------------------ //
#include "Graphics.h"

#include "AnimationTrack.h"
#include "Application/AppDefine.h"
#include "Application/GameConfiguration.h"
#include "Buffer.h"
#include "Common/StringOperations.h"
#include "Engine.h"
#include "FileSystem.h"
#include "GeometryHelpers.h"
#include "ObjectFiles/ObjectFileProcessor.h"
#include "PSO.h"
#include "Shader.h"
#include "Texture.h"
#include "Threading/ThreadingManager.h"
#include "Window.h"
#include "WindowRenderingResources.h"

#if GL_SUPPORTED
#include "DiligentCore/Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h"
#endif

#if VULKAN_SUPPORTED
#include "DiligentCore/Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"
#endif

#include "DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/SwapChain.h"

// part of the hack
#undef LOG_ERROR

#include "DiligentCore/Common/interface/RefCntAutoPtr.hpp"

// hack workaround
#undef LOG_ERROR
#define LOG_ERROR(x) Logger::Get()->Error(x);


#include "DiligentTools/TextureLoader/interface/TextureUtilities.h"


#if defined(__linux__)
#if VULKAN_SUPPORTED
#include "XLibInclude.h"
#include <xcb/xcb.h>
#endif
#endif


#include <SDL.h>
#include <SDL_syswm.h>

#include <future>
#include <optional>
#include <regex>

#ifdef __linux
#include "XLibInclude.h"
#endif

#include <filesystem>

using namespace Leviathan;
// ------------------------------------ //
namespace Leviathan {
#if VULKAN_SUPPORTED
struct XCBInfo {
    xcb_connection_t* connection = nullptr;
    uint32_t window = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    xcb_intern_atom_reply_t* atom_wm_delete_window = nullptr;
};
#endif
// ------------------------------------ //
} // namespace Leviathan
void DiligentErrorCallback(enum Diligent::DEBUG_MESSAGE_SEVERITY severity, const char* message,
    const char* function, const char* file, int line)
{
    // Forward to global logger if one exists
    auto log = Logger::Get();

    if(log) {
        switch(severity) {
        case Diligent::DEBUG_MESSAGE_SEVERITY_INFO: {
            log->Write(std::string("[INFO][DILIGENT] ") + message);
            return;
        }
        case Diligent::DEBUG_MESSAGE_SEVERITY_WARNING: {
            log->Write(std::string("[WARNING][DILIGENT] ") + message);
            return;
        }
        case Diligent::DEBUG_MESSAGE_SEVERITY_ERROR: {
            log->Write(std::string("[ERROR][DILIGENT] ") + message);
            return;
        }
        case Diligent::DEBUG_MESSAGE_SEVERITY_FATAL_ERROR:
        default: {
            log->Fatal(std::string("[DILIGENT] ") + message + ", at: " + file + "(" +
                       std::to_string(line) + ")");
            LOG_FATAL("fatal diligent message returned from Log::Fatal");
            return;
        }
        }
    }

    // TODO: what to do when can't log
}
// ------------------------------------ //
struct Graphics::Implementation {

    Implementation() {}

    // template<class T>
    // auto LoadResource(const bs::String& path)
    // {
    //     auto asset = bs::gResources().load<T>(path);

    //     if(!asset) {
    //         LOG_ERROR(std::string("Graphics: loading asset failed: ") + path.c_str());
    //         return decltype(asset)(nullptr);
    //     }

    //     if(RegisteredAssets.find(path) != RegisteredAssets.end()) {
    //         // Already registered, fine to just return
    //         return asset;
    //     }

    //     // Was not registered.

    //     bs::gResources().getResourceManifest("Default")->registerResource(
    //         asset.getUUID(), path);

    //     RegisteredAssets.insert(path);
    //     return asset;
    // }

    // std::unordered_set<bs::String> RegisteredAssets;

    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> RenderDevice;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> ImmediateContext;

    //! Currently set render target
    WindowRenderingResources* CurrentRenderTarget = nullptr;

    PSO* CurrentPSO = nullptr;

    //! All created windows.
    //! \todo Decide i fit is a good idea that these are non-owning pointers and the Window
    //! objects have a unique_ptr for the rendering resources
    std::vector<WindowRenderingResources*> ExistingWindows;


    //! Set once first backbuffer is created, all windows must have the same format
    std::optional<Diligent::TEXTURE_FORMAT> BackBufferFormat;

    //! Set once first depth buffer is created, all windows must have the same format
    std::optional<Diligent::TEXTURE_FORMAT> DepthBufferFormat;
};

#ifdef __linux
bool HasX11Error = false;

int LeviathanX11ErrorHandler(Display* display, XErrorEvent* event)
{
    std::stringstream str;
    str << "X error received: "
        << "type " << event->type << ", "
        << "serial " << event->serial << ", "
        << "error_code " << static_cast<int>(event->error_code) << ", "
        << "request_code " << static_cast<int>(event->request_code) << ", "
        << "minor_code " << static_cast<int>(event->minor_code);

    LOG_ERROR(str.str());
    HasX11Error = true;
    return 0;
}
#endif

DLLEXPORT Graphics::Graphics() {}

Graphics::~Graphics()
{
    LEVIATHAN_ASSERT(!Initialized, "Graphics not released before destructor");
}
// ------------------------------------------- //
bool Graphics::Init(AppDef* appdef)
{
    // StartUp SDL //
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {

        LOG_ERROR("Graphics: Init: SDL init failed, error: " + std::string(SDL_GetError()));
        return false;
    }

    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

    // Set needed opengl options for sdl
    // TODO: would be nice to always load the newest supported version
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    int displays = SDL_GetNumVideoDisplays();

    LOG_INFO("SDL: display count: " + Convert::ToString(displays));

    // Get display positions
    std::vector<SDL_Rect> displayBounds;

    for(int i = 0; i < displays; i++) {

        displayBounds.push_back(SDL_Rect());

        SDL_GetDisplayBounds(i, &displayBounds.back());

        const char* nameptr = SDL_GetDisplayName(i);

        const auto name = nameptr ? std::string(nameptr) : std::string("unnamed");

        // Video modes //
        int videomodecount = SDL_GetNumDisplayModes(i);

        std::vector<std::string> videomodes;

        for(int a = 0; a < videomodecount; a++) {


            SDL_DisplayMode mode = {SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0};

            if(SDL_GetDisplayMode(i, a, &mode) == 0) {

                videomodes.push_back(Convert::ToString(SDL_BITSPERPIXEL(mode.format)) +
                                     " bpp " + Convert::ToString(mode.w) + "x" +
                                     Convert::ToString(mode.h) + " at " +
                                     Convert::ToString(mode.refresh_rate) + "Hz");
            }
        }


        LOG_INFO("Display(" + Convert::ToString(i) + ", " + name + "): top left: (" +
                 Convert::ToString(displayBounds.back().x) + ", " +
                 Convert::ToString(displayBounds.back().y) +
                 ") size: " + Convert::ToString(displayBounds.back().w) + "x" +
                 Convert::ToString(displayBounds.back().h));

        // LOG_INFO("Supported modes(" + Convert::ToString(videomodes.size()) + "): ");
        // for(const auto& mode : videomodes){

        //     LOG_WRITE(" " + mode);
        // }
    }

    PrintDetectedSystemInformation();

    if(!InitializeDiligent(appdef)) {

        LOG_ERROR("Graphics: Init: failed to initialize diligent engine");
        return false;
    }

#ifdef __linux
    // Set X11 error handler to not crash on non-fatal errors
    XSetErrorHandler(LeviathanX11ErrorHandler);
#endif

    Initialized = true;
    return true;
}

DLLEXPORT void Graphics::Release()
{
    if(Pimpl) {
        if(!Pimpl->ExistingWindows.empty()) {
            LOG_ERROR("Graphics: windows open on Release");

            for(const auto& resources : Pimpl->ExistingWindows) {
                resources->Invalidate();
            }

            Pimpl->ExistingWindows.clear();
        }
    }

    if(Initialized) {
        ShutdownDiligent();

        SDL_Quit();
    }

    Initialized = false;
    FirstWindowCreated = false;
    Pimpl.reset();
    LOG_INFO("Graphics: release done");
}
// ------------------------------------ //
void Graphics::PrintDetectedSystemInformation()
{
    // There is no longer any graphics engine printing this info so we need to do this
    // ourselves
    LOG_WRITE("TODO: CPU and GPU system info printing redo");

    std::stringstream sstream;

    sstream << "Start of graphics system information:\n"
            << "// ------------------------------------ //\n";

    sstream << "TODO: readd CPU and GPU detection\n";

    sstream << "// ------------------------------------ //";

    LOG_INFO(sstream.str());
}

DLLEXPORT std::string Graphics::GetUsedAPIName() const
{
    switch(SelectedAPI) {
    case GRAPHICS_API::D3D11: {
        return "DirectX11";
    }
    case GRAPHICS_API::D3D12: {
        return "DirectX12";
    }
    case GRAPHICS_API::OpenGL: {
        return "OpenGL";
    }
    case GRAPHICS_API::OpenGLES: {
        return "OpenGLES";
    }
    case GRAPHICS_API::Vulkan: {
        return "Vulkan";
    }
    case GRAPHICS_API::Metal: {
        return "Metal";
    }
    }

    return "error";
}

DLLEXPORT bool Graphics::IsOpenGLUsed() const
{
    switch(SelectedAPI) {
    case GRAPHICS_API::OpenGL:
    case GRAPHICS_API::OpenGLES: return true;
    default: return false;
    }
}
// ------------------------------------ //
bool Graphics::SelectPreferredGraphicsAPI(AppDef* appdef)
{
    const auto defaultRenderer = "Vulkan";

    std::string renderAPI;
    ObjectFileProcessor::LoadValueFromNamedVars<std::string>(
        appdef->GetValues(), "RenderAPI", renderAPI, defaultRenderer);

    LOG_INFO("Graphics: preferred rendering API: '" + renderAPI + "'");

    const std::regex vulkan(
        "Vulkan", std::regex_constants::ECMAScript | std::regex_constants::icase);
    const std::regex opengl(
        "OpenGL", std::regex_constants::ECMAScript | std::regex_constants::icase);

    const std::regex openglES(
        "OpenGLES", std::regex_constants::ECMAScript | std::regex_constants::icase);

    const std::regex metal(
        "Metal", std::regex_constants::ECMAScript | std::regex_constants::icase);

    const std::regex D3D11(
        "DirectX11", std::regex_constants::ECMAScript | std::regex_constants::icase);

    const std::regex D3D12(
        "DirectX12", std::regex_constants::ECMAScript | std::regex_constants::icase);

    if(std::regex_match(renderAPI, vulkan)) {
        SelectedAPI = GRAPHICS_API::Vulkan;
    } else if(std::regex_match(renderAPI, openglES)) {
        SelectedAPI = GRAPHICS_API::OpenGLES;
    } else if(std::regex_match(renderAPI, opengl)) {
        SelectedAPI = GRAPHICS_API::OpenGL;
    } else if(std::regex_match(renderAPI, metal)) {
        SelectedAPI = GRAPHICS_API::Metal;
    } else if(std::regex_match(renderAPI, D3D11)) {
        SelectedAPI = GRAPHICS_API::D3D11;
    } else if(std::regex_match(renderAPI, D3D12)) {
        SelectedAPI = GRAPHICS_API::D3D12;
    } else {
        LOG_ERROR("Graphics: unknown render API selected: " + renderAPI);
        return false;
    }

    return true;
}

bool Graphics::CheckAndInitializeSelectedAPI()
{
    LOG_INFO("Graphics: selected render API: " + GetUsedAPIName());

    switch(SelectedAPI) {
#if D3D11_SUPPORTED
    case GRAPHICS_API::D3D11: {
        DEBUG_BREAK;

        // Fallback for this is GRAPHICS_API::OpenGL
        break;
    }
#endif // D3D11_SUPPORTED

#if D3D12_SUPPORTED
    case GRAPHICS_API::D3D12: {
        DEBUG_BREAK;

        // Fallback for this is GRAPHICS_API::D3D11
        break;
    }
#endif // D3D12_SUPPORTED

#if GL_SUPPORTED
    case GRAPHICS_API::OpenGL: {
        // This has no initialization actions
        // This has no fallback
        break;
    }
#endif // GL_SUPPORTED

#if GLES_SUPPORTED
    case GRAPHICS_API::OpenGLES: {
        DEBUG_BREAK;
        break;
    }
#endif // GLES_SUPPORTED

#if VULKAN_SUPPORTED
    case GRAPHICS_API::Vulkan: {
        LOG_INFO("Attempting to create a vulkan device and context");

        Diligent::EngineVkCreateInfo EngVkAttribs;

        EngVkAttribs.DebugMessageCallback = &DiligentErrorCallback;

        // TODO: configure validation
        // #ifdef _DEBUG
        EngVkAttribs.EnableValidation = true;
        // #endif
        auto* pFactoryVk = Diligent::GetEngineFactoryVk();
        pFactoryVk->CreateDeviceAndContextsVk(
            EngVkAttribs, &Pimpl->RenderDevice, &Pimpl->ImmediateContext);

        if(!Pimpl->RenderDevice || !Pimpl->ImmediateContext) {
            LOG_ERROR("Graphics: vulkan device creation failed, using fallback");

#if D3D12_SUPPORTED
            SelectedAPI = GRAPHICS_API::D3D12;
            return CheckAndInitializeSelectedAPI();
#endif
#if GL_SUPPORTED
            SelectedAPI = GRAPHICS_API::OpenGL;
            return CheckAndInitializeSelectedAPI();
#endif

            LOG_ERROR("No fallback found");
            return false;
        }

        LOG_INFO("Graphics: vulkan device and context created");
        break;
    }
#endif // VULKAN_SUPPORTED

#if METAL_SUPPORTED
    case GRAPHICS_API::Metal: {
        DEBUG_BREAK;
        break;
    }
#endif // METAL_SUPPORTED
    default:
        LOG_ERROR("Graphics: selected API is unavailable on current platform");
        return false;
    }

    return true;
}

bool Graphics::InitializeDiligent(AppDef* appdef)
{
    if(!SelectPreferredGraphicsAPI(appdef))
        return false;

    Pimpl = std::make_unique<Implementation>();

    return CheckAndInitializeSelectedAPI();
}

void Graphics::ShutdownDiligent()
{
    if(Pimpl->ImmediateContext) {
        Pimpl->ImmediateContext->Flush();
    }
}
// ------------------------------------------- //
std::unique_ptr<WindowRenderingResources> Graphics::RegisterCreatedWindow(Window& window)
{
    LOG_INFO("Graphics: creating rendering resources for a window");

    Diligent::SwapChainDesc SCDesc;
    // Uint32        NumDeferredCtx = 0;

    auto windowResources = std::make_unique<WindowRenderingResources>(*this);

    switch(SelectedAPI) {
#if D3D11_SUPPORTED
    case GRAPHICS_API::D3D11: {
        DEBUG_BREAK;
        break;
    }
#endif // D3D11_SUPPORTED

#if D3D12_SUPPORTED
    case GRAPHICS_API::D3D12: {
        DEBUG_BREAK;
        break;
    }
#endif // D3D12_SUPPORTED

#if GL_SUPPORTED
    case GRAPHICS_API::OpenGL: {
        auto* pFactoryOpenGL = Diligent::GetEngineFactoryOpenGL();

        Diligent::EngineGLCreateInfo CreationAttribs;
        CreationAttribs.pNativeWndHandle = reinterpret_cast<void*>(window.GetNativeHandle());

#if defined(__linux__)
        CreationAttribs.pDisplay = reinterpret_cast<Display*>(window.GetWindowXDisplay());
#endif

        LEVIATHAN_ASSERT(
            !Pimpl->RenderDevice, "opengl multiple windows probably needs fixing");

        pFactoryOpenGL->CreateDeviceAndSwapChainGL(CreationAttribs, &Pimpl->RenderDevice,
            &Pimpl->ImmediateContext, SCDesc, &windowResources->WindowsSwapChain);

        if(!Pimpl->RenderDevice || !Pimpl->ImmediateContext) {
            LOG_FATAL("Graphics: opengl device creation failed");
            return nullptr;
        }

        if(!&windowResources->WindowsSwapChain) {
            LOG_FATAL("OpenGL swapchain creation failed");
            return nullptr;
        }

        break;
    }
#endif // GL_SUPPORTED

#if GLES_SUPPORTED
    case GRAPHICS_API::OpenGLES: {
        DEBUG_BREAK;
        break;
    }
#endif // GLES_SUPPORTED

#if VULKAN_SUPPORTED
    case GRAPHICS_API::Vulkan: {
        LOG_INFO("Attempting to create a vulkan swap chain");

        XCBInfo info;

        int32_t width, height;
        window.GetSize(width, height);
        info.width = width;
        info.height = height;
        info.window = window.GetNativeHandle();
        // info.atom_wm_delete_window

#if defined(__linux__)
        // Convert the SDL's window connection to xcb connection that is needed by diligent
        info.connection =
            XGetXCBConnection(reinterpret_cast<Display*>(window.GetWindowXDisplay()));
#endif

        auto* pFactoryVk = Diligent::GetEngineFactoryVk();

        pFactoryVk->CreateSwapChainVk(Pimpl->RenderDevice, Pimpl->ImmediateContext, SCDesc,
            &info, &windowResources->WindowsSwapChain);

        if(!windowResources->WindowsSwapChain) {
            LOG_FATAL("Vulkan swapchain creation failed. TODO: it would still technically be "
                      "possible to fallback to opengl");
            return nullptr;
        }

        LOG_INFO("Graphics: vulkan swap chain created for window");
        break;
    }
#endif // VULKAN_SUPPORTED

#if METAL_SUPPORTED
    case GRAPHICS_API::Metal: {
        DEBUG_BREAK;
        break;
    }
#endif // METAL_SUPPORTED
    default: LOG_FATAL("Graphics: invalid graphics API selected in window creation");
    }

    if(windowResources) {
        if(!windowResources->WindowsSwapChain) {
            LOG_FATAL("Graphics: logic error: missing check for swap chain creation fail");
            return nullptr;
        }

        Pimpl->ExistingWindows.push_back(windowResources.get());

        // Store back buffer format (if unset), and also depth
        if(!Pimpl->BackBufferFormat) {
            Pimpl->BackBufferFormat =
                windowResources->WindowsSwapChain->GetDesc().ColorBufferFormat;
            Pimpl->DepthBufferFormat =
                windowResources->WindowsSwapChain->GetDesc().DepthBufferFormat;
        } else {
            if(*Pimpl->BackBufferFormat !=
                windowResources->WindowsSwapChain->GetDesc().ColorBufferFormat) {
                LOG_FATAL("Additional created window has different backbuffer format than the "
                          "main window");
            }

            if(*Pimpl->DepthBufferFormat !=
                windowResources->WindowsSwapChain->GetDesc().ColorBufferFormat) {
                LOG_FATAL(
                    "Additional created window has different depthbuffer format than the "
                    "main window");
            }
        }
    }

    return windowResources;
}

void Graphics::UnRegisterWindow(Window& window)
{
    const WindowRenderingResources* unregistered = window.GetRenderResources().get();
    if(unregistered == Pimpl->CurrentRenderTarget) {
        LOG_FATAL("destroying currently bound render target window is unhandled");
        Pimpl->CurrentRenderTarget = nullptr;
    }

    Engine::Get()->AssertIfNotMainThread();

    for(auto iter = Pimpl->ExistingWindows.begin(); iter != Pimpl->ExistingWindows.end();
        ++iter) {
        if(*iter == unregistered) {

            Pimpl->ExistingWindows.erase(iter);
            return;
        }
    }

    LOG_ERROR("Graphics: UnRegisterWindow: given a window with unknown rendering resources");

    // if(Pimpl) {
    //     Pimpl->OurApp->waitUntilFrameFinished();
    // }
}
// ------------------------------------ //
DLLEXPORT Diligent::TEXTURE_FORMAT Graphics::GetBackBufferFormat() const
{
    return *Pimpl->BackBufferFormat;
}

DLLEXPORT Diligent::TEXTURE_FORMAT Graphics::GetDepthBufferFormat() const
{
    return *Pimpl->DepthBufferFormat;
}
// ------------------------------------ //
DLLEXPORT bool Graphics::Frame()
{
    // TODO: vsync

    // Once we swap the diligent chains the render target gets unbound
    Pimpl->CurrentRenderTarget = nullptr;
    Pimpl->CurrentPSO = nullptr;

    // Swap all swapchains
    // TODO: allow hiding and stuff
    for(const auto& resources : Pimpl->ExistingWindows) {
        // TODO: could only swap a window if it was rendered to
        resources->Present();
    }

    return true;
}
// ------------------------------------ //
// Rendering operations
DLLEXPORT void Graphics::SetActiveRenderTarget(WindowRenderingResources* target)
{
    if(target && !target->WindowsSwapChain) {
        LOG_ERROR("Graphics: SetActiveRenderTarget: no swap chain on target");
        target = nullptr;
    }

    if(!target) {
        LOG_WARNING("Graphics: SetActiveRenderTarget: unsetting render target without setting "
                    "a new one is not implemented");
    }

    if(Pimpl->CurrentRenderTarget != target) {

        Pimpl->CurrentRenderTarget = target;

        auto* backBuffer =
            Pimpl->CurrentRenderTarget->WindowsSwapChain->GetCurrentBackBufferRTV();
        auto* depth = Pimpl->CurrentRenderTarget->WindowsSwapChain->GetDepthBufferDSV();

        Pimpl->ImmediateContext->SetRenderTargets(
            1, &backBuffer, depth, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
}

DLLEXPORT void Graphics::ClearRTColour(const Float4& colour)
{
    if(!Pimpl->CurrentRenderTarget)
        throw InvalidState("no render target set");

    auto* backBuffer = Pimpl->CurrentRenderTarget->WindowsSwapChain->GetCurrentBackBufferRTV();
    Pimpl->ImmediateContext->ClearRenderTarget(backBuffer, colour.operator const float*(),
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

DLLEXPORT void Graphics::ClearRTDepth()
{
    if(!Pimpl->CurrentRenderTarget)
        throw InvalidState("no render target set");

    auto* depth = Pimpl->CurrentRenderTarget->WindowsSwapChain->GetDepthBufferDSV();
    Pimpl->ImmediateContext->ClearDepthStencil(depth, Diligent::CLEAR_DEPTH_FLAG, 1.f, 0,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}
// ------------------------------------ //
DLLEXPORT void Graphics::SetActivePSO(PSO& pso)
{
    if(Pimpl->CurrentPSO == &pso)
        return;

    Pimpl->ImmediateContext->SetPipelineState(pso.GetInternal().RawPtr());
}
// ------------------------------------ //
DLLEXPORT void* Graphics::MapBuffer(
    Rendering::Buffer& buffer, Diligent::MAP_TYPE mappingtype, Diligent::MAP_FLAGS mapflags)
{
    void* mappedData = nullptr;
    Pimpl->ImmediateContext->MapBuffer(
        buffer.GetInternal(), mappingtype, mapflags, mappedData);

    return mappedData;
}

DLLEXPORT void Graphics::UnMapBuffer(Rendering::Buffer& buffer, Diligent::MAP_TYPE mappingtype)
{
    Pimpl->ImmediateContext->UnmapBuffer(buffer.GetInternal(), mappingtype);
}
// ------------------------------------ //
DLLEXPORT void Graphics::WriteDynamicTextureData(Texture& texture, uint32_t miplevel,
    uint32_t slice, const Diligent::Box& updatebox,
    const Diligent::TextureSubResData& subresdata)
{
    Pimpl->ImmediateContext->UpdateTexture(texture.GetInternal(), miplevel, slice, updatebox,
        subresdata, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}
// ------------------------------------ //
DLLEXPORT void Graphics::CommitShaderResources(
    Diligent::IShaderResourceBinding* binding, Diligent::RESOURCE_STATE_TRANSITION_MODE mode)
{
    Pimpl->ImmediateContext->CommitShaderResources(binding, mode);
}
// ------------------------------------ //
DLLEXPORT void Graphics::Draw(const Diligent::DrawAttribs& attribs)
{
    Pimpl->ImmediateContext->Draw(attribs);
}

DLLEXPORT void Graphics::Draw(const Diligent::DrawIndexedAttribs& attribs)
{
    Pimpl->ImmediateContext->DrawIndexed(attribs);
}

DLLEXPORT void Graphics::DrawMesh(Mesh& mesh)
{
    uint32_t offset = 0;
    Diligent::IBuffer* pBuffs[] = {mesh.GetVertexBuffer()->GetInternal()};

    // Bind buffers for rendering
    Pimpl->ImmediateContext->SetVertexBuffers(0, 1, pBuffs, &offset,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
        Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
    Pimpl->ImmediateContext->SetIndexBuffer(mesh.GetIndexBuffer()->GetInternal(), 0,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Create draw operation
    Diligent::DrawIndexedAttribs drawAttrs;
    drawAttrs.IndexType = mesh.GetIndexType();
    drawAttrs.NumIndices = mesh.GetIndexCount();

    // Verify a bunch of extra stuff if enabled
    drawAttrs.Flags = DebugVerify ? Diligent::DRAW_FLAG_VERIFY_ALL : Diligent::DRAW_FLAG_NONE;

    // And draw with it
    Draw(drawAttrs);
}
// ------------------------------------ //
// Rendering resource creation
DLLEXPORT std::shared_ptr<PSO> Graphics::CreatePSO(const Diligent::PipelineStateDesc& desc)
{
    Diligent::RefCntAutoPtr<Diligent::IPipelineState> pso;

    Pimpl->RenderDevice->CreatePipelineState(desc, &pso);

    if(!pso)
        return nullptr;

    return std::make_shared<PSO>(pso);
}

DLLEXPORT CountedPtr<Shader> Graphics::CreateShader(
    const Diligent::ShaderCreateInfo& info, const ShaderVariationInfo& variations)
{
    // TODO: variations compile
    Diligent::RefCntAutoPtr<Diligent::IShader> shader;

    Pimpl->RenderDevice->CreateShader(info, &shader);

    if(!shader)
        return nullptr;

    return Shader::MakeShared<Shader>(shader);
}

DLLEXPORT std::shared_ptr<Rendering::Buffer> Graphics::CreateBuffer(
    const Diligent::BufferDesc& desc, Diligent::BufferData* initialdata /*= nullptr*/)
{
    Diligent::RefCntAutoPtr<Diligent::IBuffer> buffer;

    Pimpl->RenderDevice->CreateBuffer(desc, initialdata, &buffer);

    if(!buffer)
        return nullptr;

    return std::make_shared<Rendering::Buffer>(buffer);
}

DLLEXPORT CountedPtr<Texture> Graphics::CreateTexture(
    const Diligent::TextureDesc& desc, const Diligent::TextureData* data)
{
    Diligent::RefCntAutoPtr<Diligent::ITexture> texture;

    Pimpl->RenderDevice->CreateTexture(desc, data, &texture);

    if(!texture)
        return nullptr;

    return Texture::MakeShared<Texture>(texture, desc.Width, desc.Height);
}

DLLEXPORT CountedPtr<Texture> Graphics::CreateDynamicTexture(
    int width, int height, Diligent::TEXTURE_FORMAT format)
{
    Diligent::TextureDesc desc;
    desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
    desc.Width = width;
    desc.Height = height;
    desc.Usage = Diligent::USAGE_DYNAMIC;
    desc.BindFlags = Diligent::BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
    desc.Format = format;
    desc.MipLevels = 1;
    return CreateTexture(desc, nullptr);
}
// ------------------------------------ //
DLLEXPORT bool Graphics::IsVerticalUVFlipped() const
{
    // Assume only opengl and opengles are flipped
    return SelectedAPI == GRAPHICS_API::OpenGL || SelectedAPI == GRAPHICS_API::OpenGLES;

    // const auto capabilities = bs::ct::RenderAPI::instance().getCapabilities(0);

    // return capabilities.conventions.ndcYAxis != bs::Conventions::Axis::Down;
}
// ------------------------------------ //
// Resource loading helpers
DLLEXPORT CountedPtr<Shader> Graphics::LoadShaderByName(const std::string& name)
{
    DEBUG_BREAK;
    auto file = FileSystem::Get()->SearchForFile(Leviathan::FILEGROUP_OTHER,
        // Leviathan::StringOperations::RemoveExtension(name, true),
        Leviathan::StringOperations::RemovePath(name),
        // Leviathan::StringOperations::GetExtension(name)
        "asset");

    if(file.empty()) {
        LOG_ERROR("Graphics: LoadShaderByName: could not find resource with name: " + name);
        return nullptr;
    }
    return nullptr;
    // return
    // Pimpl->LoadResource<bs::Shader>(std::filesystem::absolute(file).string().c_str());
}

DLLEXPORT CountedPtr<Texture> Graphics::LoadTextureByName(const std::string& name)
{
    auto file = FileSystem::Get()->SearchForFile(Leviathan::FILEGROUP_TEXTURE,
        Leviathan::StringOperations::RemoveExtension(name, true),
        // Leviathan::StringOperations::RemovePath(name),
        Leviathan::StringOperations::GetExtension(name));

    if(file.empty()) {
        LOG_ERROR("Graphics: LoadTextureByName: could not find resource with name: " + name);
        return nullptr;
    }

    Diligent::TextureLoadInfo loadInfo;
    loadInfo.IsSRGB = true;
    Diligent::RefCntAutoPtr<Diligent::ITexture> texture;

    Diligent::CreateTextureFromFile(file.c_str(), loadInfo, Pimpl->RenderDevice, &texture);

    if(!texture) {
        LOG_ERROR("Graphics: LoadTextureByName: failed to load texture file");
        return nullptr;
    }

    return Texture::MakeShared<Texture>(texture);
}

DLLEXPORT CountedPtr<Mesh> Graphics::LoadMeshByName(const std::string& name)
{
    DEBUG_BREAK;
    auto file = FileSystem::Get()->SearchForFile(Leviathan::FILEGROUP_MODEL,
        // Leviathan::StringOperations::RemoveExtension(name, true),
        Leviathan::StringOperations::RemovePath(name),
        // Leviathan::StringOperations::GetExtension(name)
        "asset");

    if(file.empty()) {
        LOG_ERROR("Graphics: LoadMeshByName: could not find resource with name: " + name);
        return nullptr;
    }
    return nullptr;
    // return Pimpl->LoadResource<bs::Mesh>(std::filesystem::absolute(file).string().c_str());
}

DLLEXPORT CountedPtr<AnimationTrack> Graphics::LoadAnimationClipByName(const std::string& name)
{
    DEBUG_BREAK;
    auto file = FileSystem::Get()->SearchForFile(Leviathan::FILEGROUP_MODEL,
        // Leviathan::StringOperations::RemoveExtension(name, true),
        Leviathan::StringOperations::RemovePath(name),
        // Leviathan::StringOperations::GetExtension(name)
        "asset");

    if(file.empty()) {
        LOG_ERROR(
            "Graphics: LoadAnimationClipByName: could not find resource with name: " + name);
        return nullptr;
    }
    return nullptr;
    // return Pimpl->LoadResource<bs::AnimationClip>(
    // std::filesystem::absolute(file).string().c_str());
}

// ------------------------------------ //
// X11 errors
#ifdef __linux
DLLEXPORT bool Graphics::HasX11ErrorOccured()
{
    if(HasX11Error) {
        HasX11Error = false;
        return true;
    }

    return false;
}
#endif
