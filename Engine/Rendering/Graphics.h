// Leviathan Game Engine
// Copyright (c) 2012-2020 Henri Hyyryläinen
#pragma once
#include "Define.h"
// ------------------------------------ //
#include "Application/AppDefine.h"
#include "Common/Types.h"
#include "Shader.h"

#include "bsfCore/RenderAPI/BsRenderWindow.h"

namespace Diligent {
struct PipelineStateDesc;
struct ShaderCreateInfo;
struct DrawAttribs;
struct DrawIndexedAttribs;
enum RESOURCE_STATE_TRANSITION_MODE : Uint8;
class IShaderResourceBinding;
struct BufferDesc;
struct BufferData;
} // namespace Diligent

namespace Leviathan {

namespace Rendering {
class Buffer;
class MappedBuffer;
} // namespace Rendering

class WindowRenderingResources;
class PSO;
class Mesh;

enum class GRAPHICS_API { D3D11, D3D12, Vulkan, OpenGL, OpenGLES, Metal };

//! \brief Main handler of graphics resources and rendering
class Graphics {
    friend Window;

    struct Implementation;

public:
    DLLEXPORT Graphics();
    DLLEXPORT ~Graphics();

    DLLEXPORT bool Init(AppDef* appdef);
    DLLEXPORT void Release();

    DLLEXPORT bool Frame();

    DLLEXPORT bool IsVerticalUVFlipped() const;

    inline auto GetUsedAPI() const
    {
        return SelectedAPI;
    }

    DLLEXPORT std::string GetUsedAPIName() const;

    //! \returns True when opengl is used. Needed by Window to know when to make a context
    DLLEXPORT bool IsOpenGLUsed() const;

    //! \returns The back buffer colour format
    //! \note Only valid after first window is created
    DLLEXPORT Diligent::TEXTURE_FORMAT GetBackBufferFormat() const;

    //! \returns The depth buffer format
    //! \note Only valid after first window is created
    DLLEXPORT Diligent::TEXTURE_FORMAT GetDepthBufferFormat() const;

    // ------------------------------------ //
    // Rendering operations
    //! \brief Sets the render target rendering operations will act on
    DLLEXPORT void SetActiveRenderTarget(WindowRenderingResources* target);

    DLLEXPORT void ClearRTColour(const Float4& colour);
    DLLEXPORT void ClearRTDepth();

    DLLEXPORT void SetActivePSO(PSO& pso);

    //! \note Prefer MappedBuffer for automatic unmapping
    //! \returns Non-nullptr on success
    DLLEXPORT void* MapBuffer(Rendering::Buffer& buffer, Diligent::MAP_TYPE mappingtype,
        Diligent::MAP_FLAGS mapflags);

    DLLEXPORT void UnMapBuffer(Rendering::Buffer& buffer, Diligent::MAP_TYPE mappingtype);


    DLLEXPORT void CommitShaderResources(Diligent::IShaderResourceBinding* binding,
        Diligent::RESOURCE_STATE_TRANSITION_MODE mode);

    DLLEXPORT void Draw(const Diligent::DrawAttribs& attribs);
    DLLEXPORT void Draw(const Diligent::DrawIndexedAttribs& attribs);
    DLLEXPORT void DrawMesh(Mesh& mesh);

    // ------------------------------------ //
    // Rendering resource creation
    DLLEXPORT std::shared_ptr<PSO> CreatePSO(const Diligent::PipelineStateDesc& desc);

    DLLEXPORT Shader::pointer CreateShader(
        const Diligent::ShaderCreateInfo& info, const ShaderVariationInfo& variations);

    DLLEXPORT std::shared_ptr<Rendering::Buffer> CreateBuffer(
        const Diligent::BufferDesc& desc, Diligent::BufferData* initialdata = nullptr);

    //! \brief Finds and loads a shader with the name
    //!
    //! If a full path or a valid relative path is specified a full search is not done.
    //! Unless a variant of the name with ".asset" is found, which is preferred to skip
    //! expensive importing.
    DLLEXPORT bs::HShader LoadShaderByName(const std::string& name);

    //! Works the same as LoadShaderByName
    DLLEXPORT bs::HTexture LoadTextureByName(const std::string& name);

    //! Works the same as LoadShaderByName
    DLLEXPORT bs::HMesh LoadMeshByName(const std::string& name);

    //! Works the same as LoadShaderByName
    DLLEXPORT bs::HAnimationClip LoadAnimationClipByName(const std::string& name);

#ifdef __linux
    //! \brief Returns true if our X11 error handler has been called. Remember to check this
    //! after every X11 call
    //!
    //! The value is reset to false after this call
    //! \note This is not thread safe. X11 is also not thread safe so only call on the main
    //! thread
    DLLEXPORT static bool HasX11ErrorOccured();
#endif

protected:
    //! \brief Called when Window objects are created to create the rendering resources for
    //! them
    std::unique_ptr<WindowRenderingResources> RegisterCreatedWindow(Window& window);

    //! \brief Called just before a window is destroyed. This needs to stop rendering to it
    //! \todo This needs to assign a new primary window if the primary window is destroyed
    void UnRegisterWindow(Window& window);

private:
    void PrintDetectedSystemInformation();

    bool InitializeBSF(AppDef* appdef);
    void ShutdownBSF();

    bool InitializeDiligent(AppDef* appdef);
    void ShutdownDiligent();

    bool SelectPreferredGraphicsAPI(AppDef* appdef);
    //! Check that API selection is good and perform some initialization for some APIs
    bool CheckAndInitializeSelectedAPI();

private:
    bool Initialized = false;
    bool FirstWindowCreated = false;

    //! If true enables extra rendering operation verifications
    bool DebugVerify = true;

    GRAPHICS_API SelectedAPI;

    std::unique_ptr<Implementation> Pimpl;
};
} // namespace Leviathan
