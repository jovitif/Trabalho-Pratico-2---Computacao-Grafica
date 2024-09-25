/**********************************************************************************
// Multi (Código Fonte)
//
// Criação:     27 Abr 2016
// Atualização: 15 Set 2023
// Compilador:  Visual C++ 2022
//
// Descrição:   Constrói cena usando vários buffers, um por objeto
//
**********************************************************************************/

#include "DXUT.h"



// ------------------------------------------------------------------------------

struct ObjectConstants
{
    XMFLOAT4X4 WorldViewProj =
    { 1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f };
};

enum class CameraMode { Perspective, Front, Top, Right };


// ------------------------------------------------------------------------------

class Multi : public App
{
private:
    ID3D12RootSignature* rootSignature = nullptr;
    ID3D12PipelineState* pipelineState = nullptr;
    vector<Object> scene;

    Timer timer;
    bool spinning = true;
    bool topView = false;

    XMFLOAT4X4 Identity = {};
    XMFLOAT4X4 View = {};
    XMFLOAT4X4 Proj = {};
    Object* selectedObj = nullptr;  // Para rastrear o objeto selecionado

    float theta = 0;
    float phi = 0;
    float radius = 0;
    float lastMousePosX = 0;
    float lastMousePosY = 0;

public:
    void Init();
    void Update();
    void Draw();
    void SetObjectColor(std::vector<Vertex>& vertices, const XMFLOAT4& color);
    void Finalize();
    void BuildRootSignature();
    void BuildPipelineState();
};

// Função para alterar a cor do objeto
void SetObjectColor(std::vector<Vertex>& vertices, const XMFLOAT4& color) {
    for (auto& v : vertices) {
        v.color = color;
    }
}


// ------------------------------------------------------------------------------

void Multi::Init()
{
    graphics->ResetCommands();

    // -----------------------------
    // Parâmetros Iniciais da Câmera
    // -----------------------------
 
    // controla rotação da câmera
    theta = XM_PIDIV4;
    phi = 1.3f;
    radius = 5.0f;

    // pega última posição do mouse
    lastMousePosX = (float) input->MouseX();
    lastMousePosY = (float) input->MouseY();

    // inicializa as matrizes Identity e View para a identidade
    Identity = View = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f };

    // inicializa a matriz de projeção
    XMStoreFloat4x4(&Proj, XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.0f), 
        window->AspectRatio(), 
        1.0f, 100.0f));

    // ----------------------------------------
    // Criação da Geometria: Vértices e Índices
    // ----------------------------------------

    Box box(2.0f, 2.0f, 2.0f);                    // Caixa
    Cylinder cylinder(1.0f, 0.5f, 3.0f, 20, 20);  // Cilindro
    Sphere sphere(1.0f, 20, 20);                  // Esfera
    Grid grid(3.0f, 3.0f, 20, 20);                // Grade
    GeoSphere geosphere(1.0f, 3);                 // Geoesfera (raio 1.0, 3 subdivisões)
    Quad quad(2.0f, 2.0f);                        // Quad (2x2)

    for (auto& v : box.vertices) v.color = XMFLOAT4(DirectX::Colors::DimGray);     // Caixa laranja
    for (auto& v : cylinder.vertices) v.color = XMFLOAT4(DirectX::Colors::DimGray); // Cilindro amarelo
    for (auto& v : sphere.vertices) v.color = XMFLOAT4(DirectX::Colors::DimGray);  // Esfera vermelha
    for (auto& v : grid.vertices) v.color = XMFLOAT4(DirectX::Colors::DimGray);    // Grade cinza
    for (auto& v : geosphere.vertices) v.color = XMFLOAT4(DirectX::Colors::DimGray);  // Geoesfera azul
    for (auto& v : quad.vertices) v.color = XMFLOAT4(DirectX::Colors::DimGray);      // Quad verde


    // ---------------------------------------------------------------
    // Alocação e Cópia de Vertex, Index e Constant Buffers para a GPU
    // ---------------------------------------------------------------

    // box
    Object boxObj;
    XMStoreFloat4x4(&boxObj.world,
        XMMatrixScaling(0.4f, 0.4f, 0.4f) *
        XMMatrixTranslation(-1.0f, 0.41f, 1.0f));

    boxObj.mesh = new Mesh();
    boxObj.mesh->VertexBuffer(box.VertexData(), box.VertexCount() * sizeof(Vertex), sizeof(Vertex));
    boxObj.mesh->IndexBuffer(box.IndexData(), box.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
    boxObj.mesh->ConstantBuffer(sizeof(ObjectConstants));
    boxObj.submesh.indexCount = box.IndexCount();
    scene.push_back(boxObj);

    // cylinder
    Object cylinderObj;
    XMStoreFloat4x4(&cylinderObj.world,
        XMMatrixScaling(0.5f, 0.5f, 0.5f) *
        XMMatrixTranslation(1.0f, 0.75f, -1.0f));

    cylinderObj.mesh = new Mesh();
    cylinderObj.mesh->VertexBuffer(cylinder.VertexData(), cylinder.VertexCount() * sizeof(Vertex), sizeof(Vertex));
    cylinderObj.mesh->IndexBuffer(cylinder.IndexData(), cylinder.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
    cylinderObj.mesh->ConstantBuffer(sizeof(ObjectConstants));
    cylinderObj.submesh.indexCount = cylinder.IndexCount();
    scene.push_back(cylinderObj);

    // sphere
    Object sphereObj;
    XMStoreFloat4x4(&sphereObj.world,
        XMMatrixScaling(0.5f, 0.5f, 0.5f) *
        XMMatrixTranslation(0.0f, 0.5f, 0.0f));

    sphereObj.mesh = new Mesh();
    sphereObj.mesh->VertexBuffer(sphere.VertexData(), sphere.VertexCount() * sizeof(Vertex), sizeof(Vertex));
    sphereObj.mesh->IndexBuffer(sphere.IndexData(), sphere.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
    sphereObj.mesh->ConstantBuffer(sizeof(ObjectConstants));
    sphereObj.submesh.indexCount = sphere.IndexCount();
    scene.push_back(sphereObj);

    // grid
    Object gridObj;
    gridObj.mesh = new Mesh();
    gridObj.world = Identity;
    gridObj.mesh->VertexBuffer(grid.VertexData(), grid.VertexCount() * sizeof(Vertex), sizeof(Vertex));
    gridObj.mesh->IndexBuffer(grid.IndexData(), grid.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
    gridObj.mesh->ConstantBuffer(sizeof(ObjectConstants));
    gridObj.submesh.indexCount = grid.IndexCount();
    scene.push_back(gridObj);

    // Geosphere
    Object geosphereObj;
    XMStoreFloat4x4(&geosphereObj.world,
        XMMatrixScaling(0.6f, 0.6f, 0.6f) *
        XMMatrixTranslation(2.0f, 1.0f, -2.0f));

    geosphereObj.mesh = new Mesh();
    geosphereObj.mesh->VertexBuffer(geosphere.VertexData(), geosphere.VertexCount() * sizeof(Vertex), sizeof(Vertex));
    geosphereObj.mesh->IndexBuffer(geosphere.IndexData(), geosphere.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
    geosphereObj.mesh->ConstantBuffer(sizeof(ObjectConstants));
    geosphereObj.submesh.indexCount = geosphere.IndexCount();
    scene.push_back(geosphereObj);

    // Quad
    Object quadObj;
    XMStoreFloat4x4(&quadObj.world,
        XMMatrixScaling(1.0f, 1.0f, 1.0f) *
        XMMatrixTranslation(-2.0f, 0.0f, 2.0f));

    quadObj.mesh = new Mesh();
    quadObj.mesh->VertexBuffer(quad.VertexData(), quad.VertexCount() * sizeof(Vertex), sizeof(Vertex));
    quadObj.mesh->IndexBuffer(quad.IndexData(), quad.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
    quadObj.mesh->ConstantBuffer(sizeof(ObjectConstants));
    quadObj.submesh.indexCount = quad.IndexCount();
    scene.push_back(quadObj);



 
    // ---------------------------------------

    BuildRootSignature();
    BuildPipelineState();    

    // ---------------------------------------
    graphics->SubmitCommands();

    timer.Start();
}



// ------------------------------------------------------------------------------
void Multi::Update()
{
    // sai com o pressionamento da tecla ESC
    if (input->KeyPress(VK_ESCAPE))
        window->Close();

    if (input->KeyPress('B')) {
      
    }

    // ativa ou desativa o giro do objeto
    if (input->KeyPress('S'))
    {
        spinning = !spinning;

        if (spinning)
            timer.Start();
        else
            timer.Stop();
    }

    // ativa ou desativa a visão de cima
    if (input->KeyPress('V'))
    {
        topView = !topView;
    }

    float mousePosX = (float)input->MouseX();
    float mousePosY = (float)input->MouseY();

    if (!topView) // Quando não está na visão de cima
    {
        if (input->KeyDown(VK_LBUTTON))
        {
            // cada pixel corresponde a 1/4 de grau
            float dx = XMConvertToRadians(0.25f * (mousePosX - lastMousePosX));
            float dy = XMConvertToRadians(0.25f * (mousePosY - lastMousePosY));

            // atualiza ângulos com base no deslocamento do mouse 
            theta += dx;
            phi += dy;

            // restringe o ângulo de phi ]0-180[ graus
            phi = phi < 0.1f ? 0.1f : (phi > (XM_PI - 0.1f) ? XM_PI - 0.1f : phi);
        }
        else if (input->KeyDown(VK_RBUTTON))
        {
            // cada pixel corresponde a 0.05 unidades
            float dx = 0.05f * (mousePosX - lastMousePosX);
            float dy = 0.05f * (mousePosY - lastMousePosY);

            // atualiza o raio da câmera com base no deslocamento do mouse 
            radius += dx - dy;

            // restringe o raio (3 a 15 unidades)
            radius = radius < 3.0f ? 3.0f : (radius > 15.0f ? 15.0f : radius);
        }

        lastMousePosX = mousePosX;
        lastMousePosY = mousePosY;

        // converte coordenadas esféricas para cartesianas
        float x = radius * sinf(phi) * cosf(theta);
        float z = radius * sinf(phi) * sinf(theta);
        float y = radius * cosf(phi);

        // constrói a matriz da câmera (view matrix)
        XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
        XMVECTOR target = XMVectorZero();
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
        XMStoreFloat4x4(&View, view);
    }
    else  // Quando a visão de cima está ativada
    {
        // Coloca a câmera acima dos objetos, olhando diretamente para baixo
        XMVECTOR pos = XMVectorSet(0.0f, 10.0f, 0.0f, 1.0f);  // Altura Y=10.0 (ajuste conforme necessário)
        XMVECTOR target = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f); // Centro da cena
        XMVECTOR up = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);    // Vetor "up" apontando para o eixo Z negativo
        XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
        XMStoreFloat4x4(&View, view);
    }

    // carrega matriz de projeção em uma XMMATRIX
    XMMATRIX proj = XMLoadFloat4x4(&Proj);

    // modifica matriz de mundo da esfera
    XMStoreFloat4x4(&scene[2].world,
        XMMatrixScaling(0.5f, 0.5f, 0.5f) *
        (spinning ? XMMatrixRotationY(float(timer.Elapsed())) : XMMatrixIdentity()) *
        XMMatrixTranslation(0.0f, 0.5f, 0.0f));

    // box
    XMStoreFloat4x4(&scene[0].world,
        XMMatrixScaling(0.4f, 0.4f, 0.4f) *
        (spinning ? XMMatrixRotationY(float(timer.Elapsed())) : XMMatrixIdentity()) *
        XMMatrixTranslation(-1.0f, 0.41f, 1.0f));

    // cilindro
    XMStoreFloat4x4(&scene[1].world,
        XMMatrixScaling(0.5f, 0.5f, 0.5f) *
        (spinning ? XMMatrixRotationY(float(timer.Elapsed())) : XMMatrixIdentity()) *
        XMMatrixTranslation(1.0f, 0.75f, -1.0f));


    // quad
    XMStoreFloat4x4(&scene[3].world,
        XMMatrixScaling(0.9f, 0.9f, 0.9f) *
        (spinning ? XMMatrixRotationY(float(timer.Elapsed())) : XMMatrixIdentity()) *
        XMMatrixTranslation(1.0f, 0.015f, -1.0f));

    // sla bola
    XMStoreFloat4x4(&scene[4].world,
        XMMatrixScaling(0.6f, 0.6f, 0.6f) *
        (spinning ? XMMatrixRotationY(float(timer.Elapsed())) : XMMatrixIdentity()) *
        XMMatrixTranslation(2.0f, 1.0f, -2.0f));

    XMStoreFloat4x4(&scene[5].world,
        XMMatrixScaling(1.0f, 1.0f, 1.0f) *
        (spinning ? XMMatrixRotationY(float(timer.Elapsed())) : XMMatrixIdentity()) *
        XMMatrixTranslation(-2.0f, 0.0f, 2.0f));


        


    // ajusta o buffer constante de cada objeto
    for (auto& obj : scene)
    {
        // carrega matriz de mundo em uma XMMATRIX
        XMMATRIX world = XMLoadFloat4x4(&obj.world);

        // constrói matriz combinada (world x view x proj)
        XMMATRIX WorldViewProj = world * XMLoadFloat4x4(&View) * proj;

        // atualiza o buffer constante com a matriz combinada
        ObjectConstants constants;
        XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(WorldViewProj));
        obj.mesh->CopyConstants(&constants);
    }
}

// ------------------------------------------------------------------------------

void Multi::Draw()
{
    // limpa o backbuffer
    graphics->Clear(pipelineState);
    
    // desenha objetos da cena
    for (auto& obj : scene)
    {
        // comandos de configuração do pipeline
        ID3D12DescriptorHeap* descriptorHeap = obj.mesh->ConstantBufferHeap();
        graphics->CommandList()->SetDescriptorHeaps(1, &descriptorHeap);
        graphics->CommandList()->SetGraphicsRootSignature(rootSignature);
        graphics->CommandList()->IASetVertexBuffers(0, 1, obj.mesh->VertexBufferView());
        graphics->CommandList()->IASetIndexBuffer(obj.mesh->IndexBufferView());
        graphics->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // ajusta o buffer constante associado ao vertex shader
        graphics->CommandList()->SetGraphicsRootDescriptorTable(0, obj.mesh->ConstantBufferHandle(0));

        // desenha objeto
        graphics->CommandList()->DrawIndexedInstanced(
            obj.submesh.indexCount, 1,
            obj.submesh.startIndex,
            obj.submesh.baseVertex,
            0);
    }
 
    // apresenta o backbuffer na tela
    graphics->Present();    
}

// ------------------------------------------------------------------------------

void Multi::Finalize()
{
    rootSignature->Release();
    pipelineState->Release();

    for (auto& obj : scene)
        delete obj.mesh;
}


// ------------------------------------------------------------------------------
//                                     D3D                                      
// ------------------------------------------------------------------------------

void Multi::BuildRootSignature()
{
    // cria uma única tabela de descritores de CBVs
    D3D12_DESCRIPTOR_RANGE cbvTable = {};
    cbvTable.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    cbvTable.NumDescriptors = 1;
    cbvTable.BaseShaderRegister = 0;
    cbvTable.RegisterSpace = 0;
    cbvTable.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // define parâmetro raiz com uma tabela
    D3D12_ROOT_PARAMETER rootParameters[1];
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[0].DescriptorTable.pDescriptorRanges = &cbvTable;

    // uma assinatura raiz é um vetor de parâmetros raiz
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 1;
    rootSigDesc.pParameters = rootParameters;
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.pStaticSamplers = nullptr;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // serializa assinatura raiz
    ID3DBlob* serializedRootSig = nullptr;
    ID3DBlob* error = nullptr;

    ThrowIfFailed(D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig,
        &error));

    if (error != nullptr)
    {
        OutputDebugString((char*)error->GetBufferPointer());
    }

    // cria uma assinatura raiz com um único slot que aponta para  
    // uma faixa de descritores consistindo de um único buffer constante
    ThrowIfFailed(graphics->Device()->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature)));
}

// ------------------------------------------------------------------------------

void Multi::BuildPipelineState()
{
    // --------------------
    // --- Input Layout ---
    // --------------------
    
    D3D12_INPUT_ELEMENT_DESC inputLayout[2] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // --------------------
    // ----- Shaders ------
    // --------------------

    ID3DBlob* vertexShader;
    ID3DBlob* pixelShader;

    D3DReadFileToBlob(L"Shaders/Vertex.cso", &vertexShader);
    D3DReadFileToBlob(L"Shaders/Pixel.cso", &pixelShader);

    // --------------------
    // ---- Rasterizer ----
    // --------------------

    D3D12_RASTERIZER_DESC rasterizer = {};
    //rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer.FillMode = D3D12_FILL_MODE_WIREFRAME;
    rasterizer.CullMode = D3D12_CULL_MODE_BACK;
    rasterizer.FrontCounterClockwise = FALSE;
    rasterizer.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.MultisampleEnable = FALSE;
    rasterizer.AntialiasedLineEnable = FALSE;
    rasterizer.ForcedSampleCount = 0;
    rasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // ---------------------
    // --- Color Blender ---
    // ---------------------

    D3D12_BLEND_DESC blender = {};
    blender.AlphaToCoverageEnable = FALSE;
    blender.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
    {
        FALSE,FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        blender.RenderTarget[i] = defaultRenderTargetBlendDesc;

    // ---------------------
    // --- Depth Stencil ---
    // ---------------------

    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
    depthStencil.DepthEnable = TRUE;
    depthStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencil.StencilEnable = FALSE;
    depthStencil.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStencil.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
    { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
    depthStencil.FrontFace = defaultStencilOp;
    depthStencil.BackFace = defaultStencilOp;
    
    // -----------------------------------
    // --- Pipeline State Object (PSO) ---
    // -----------------------------------

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
    pso.pRootSignature = rootSignature;
    pso.VS = { reinterpret_cast<BYTE*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
    pso.PS = { reinterpret_cast<BYTE*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
    pso.BlendState = blender;
    pso.SampleMask = UINT_MAX;
    pso.RasterizerState = rasterizer;
    pso.DepthStencilState = depthStencil;
    pso.InputLayout = { inputLayout, 2 };
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    pso.SampleDesc.Count = graphics->Antialiasing();
    pso.SampleDesc.Quality = graphics->Quality();
    graphics->Device()->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pipelineState));

    vertexShader->Release();
    pixelShader->Release();
}


// ------------------------------------------------------------------------------
//                                  WinMain                                      
// ------------------------------------------------------------------------------

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    try
    {
        // cria motor e configura a janela
        Engine* engine = new Engine();
        engine->window->Mode(WINDOWED);
        engine->window->Size(1024, 720);
        engine->window->Color(25, 25, 25);
        engine->window->Title("Multi");
        engine->window->Icon(IDI_ICON);
        engine->window->Cursor(IDC_CURSOR);
        engine->window->LostFocus(Engine::Pause);
        engine->window->InFocus(Engine::Resume);

        // cria e executa a aplicação
        engine->Start(new Multi());

        // finaliza execução
        delete engine;
    }
    catch (Error & e)
    {
        // exibe mensagem em caso de erro
        MessageBox(nullptr, e.ToString().data(), "Multi", MB_OK);
    }

    return 0;
}

// ----------------------------------------------------------------------------

