/**********************************************************************************
// Single (Código Fonte)
//
// Criação:     27 Abr 2016
// Atualização: 15 Set 2023
// Compilador:  Visual C++ 2022
//
// Descrição:   Constrói cena com um único buffer de vértices e índices
//
**********************************************************************************/

#include "DXUT.h"
#include <windows.h>
#include <iostream>
using namespace std;

// ------------------------------------------------------------------------------

struct ObjectConstants
{
    XMFLOAT4X4 WorldViewProj =
    { 1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f };
};

// ------------------------------------------------------------------------------

class Single : public App
{
private:
    ID3D12RootSignature* rootSignature = nullptr;
    ID3D12PipelineState* pipelineState = nullptr;
    vector<Object> scene;
    Mesh* mesh = nullptr;

    Timer timer;
    bool spinning = true;

    XMFLOAT4X4 Identity = {};
    XMFLOAT4X4 View = {};
    XMFLOAT4X4 Proj = {};

    float theta = 0;
    float phi = 0;
    float radius = 0;
    float lastMousePosX = 0;
    float lastMousePosY = 0;

public:
    void Init();
    void Update();
    void Draw();
    void Finalize();

    void BuildRootSignature();
    void BuildPipelineState();
};

// ------------------------------------------------------------------------------

void Single::Init()
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

    Box box(2.0f, 2.0f, 2.0f);
    Cylinder cylinder(1.0f, 0.5f, 3.0f, 20, 10);
    Sphere sphere(1.0f, 20, 20);
    Grid grid(3.0f, 3.0f, 20, 20);

    // ---------------------------
    // União de Vértices e Índices
    // ---------------------------

    // quantidade total de vértices
    uint totalVertexCount = (
        box.VertexCount() +
        cylinder.VertexCount() +
        sphere.VertexCount() +
        grid.VertexCount()
        );

    // tamanho em bytes dos vértices
    const uint vbSize = totalVertexCount * sizeof(Vertex);

    // quantidade total de índices
    uint totalIndexCount = (
        box.IndexCount() +
        cylinder.IndexCount() +
        sphere.IndexCount() +
        grid.IndexCount()
        );

    // tamanho em bytes dos índices
    const uint ibSize = totalIndexCount * sizeof(uint);

    // junta todos os vértices em um único vetor
    vector<Vertex> vertices(totalVertexCount);

    uint k = 0;
    for (uint i = 0; i < box.VertexCount(); ++i, ++k)
    {
        vertices[k].pos = box.vertices[i].pos;
        vertices[k].color = XMFLOAT4(DirectX::Colors::Orange);
    }

    for (uint i = 0; i < cylinder.VertexCount(); ++i, ++k)
    {
        vertices[k].pos = cylinder.vertices[i].pos;
        vertices[k].color = XMFLOAT4(DirectX::Colors::Yellow);
    }

    for (uint i = 0; i < sphere.VertexCount(); ++i, ++k)
    {
        vertices[k].pos = sphere.vertices[i].pos;
        vertices[k].color = XMFLOAT4(DirectX::Colors::Crimson);
    }

    for (uint i = 0; i < grid.VertexCount(); ++i, ++k)
    {
        vertices[k].pos = grid.vertices[i].pos;
        vertices[k].color = XMFLOAT4(DirectX::Colors::DimGray);
    }

    // junta todos os índices em um único vetor
    vector<uint> indices;

    indices.insert(indices.end(), begin(box.indices), end(box.indices));
    indices.insert(indices.end(), begin(cylinder.indices), end(cylinder.indices));
    indices.insert(indices.end(), begin(sphere.indices), end(sphere.indices));
    indices.insert(indices.end(), begin(grid.indices), end(grid.indices));

    // ------------------------
    // Definição das Sub-Malhas
    // ------------------------

    // define os parâmetros das sub-malhas
    SubMesh boxSubMesh;
    boxSubMesh.indexCount = uint(box.IndexCount());
    boxSubMesh.startIndex = 0;
    boxSubMesh.baseVertex = 0;

    SubMesh cylinderSubMesh;
    cylinderSubMesh.indexCount = uint(cylinder.IndexCount());
    cylinderSubMesh.startIndex = uint(box.IndexCount());
    cylinderSubMesh.baseVertex = uint(box.VertexCount());

    SubMesh sphereSubMesh;
    sphereSubMesh.indexCount = uint(sphere.IndexCount());
    sphereSubMesh.startIndex = uint(box.IndexCount() + cylinder.IndexCount());
    sphereSubMesh.baseVertex = uint(box.VertexCount() + cylinder.VertexCount());

    SubMesh gridSubMesh;
    gridSubMesh.indexCount = uint(grid.IndexCount());
    gridSubMesh.startIndex = uint(box.IndexCount() + cylinder.IndexCount() + sphere.IndexCount());
    gridSubMesh.baseVertex = uint(box.VertexCount() + cylinder.VertexCount() + sphere.VertexCount());

    // --------------------
    // Definição de Objetos
    // --------------------

    Object obj;

    // box
    XMStoreFloat4x4(&obj.world,
        XMMatrixScaling(0.4f, 0.4f, 0.4f) *
        XMMatrixTranslation(-1.0f, 0.41f, 1.0f));
    obj.cbIndex = 0;
    obj.submesh = boxSubMesh;
    scene.push_back(obj);

    // cylinder
    XMStoreFloat4x4(&obj.world,
        XMMatrixScaling(0.5f, 0.5f, 0.5f) *
        XMMatrixTranslation(1.0f, 0.75f, -1.0f));
    obj.cbIndex = 1;
    obj.submesh = cylinderSubMesh;
    scene.push_back(obj);

    // sphere
    XMStoreFloat4x4(&obj.world,
        XMMatrixScaling(0.5f, 0.5f, 0.5f) *
        XMMatrixTranslation(0.0f, 0.5f, 0.0f));
    obj.cbIndex = 2;
    obj.submesh = sphereSubMesh;
    scene.push_back(obj);

    // grid
    obj.world = Identity;
    obj.cbIndex = 3;
    obj.submesh = gridSubMesh;
    scene.push_back(obj);

    // ---------------------------------------------------------------
    // Alocação e Cópia de Vertex, Index e Constant Buffers para a GPU
    // ---------------------------------------------------------------

    // cria malha 3D
    mesh = new Mesh();
    mesh->VertexBuffer(vertices.data(), vbSize, sizeof(Vertex));
    mesh->IndexBuffer(indices.data(), ibSize, DXGI_FORMAT_R32_UINT);
    mesh->ConstantBuffer(sizeof(ObjectConstants), uint(scene.size()));
 
    // ---------------------------------------

    BuildRootSignature();
    BuildPipelineState();    

    // ---------------------------------------
    graphics->SubmitCommands();

    timer.Start();
}

// ------------------------------------------------------------------------------

void Single::Update()
{
    // sai com o pressionamento da tecla ESC
    if (input->KeyPress(VK_ESCAPE))
        window->Close();

    // ativa ou desativa o giro do objeto
    if (input->KeyPress('S'))
    {
        spinning = !spinning;

        if (spinning)
            timer.Start();
        else
            timer.Stop();
    }
    // b: box, c:cylinder, s:sphere, g: geosphere, p: plane (grid), q:quad
    if (input->KeyPress('B')) {
        OutputDebugString("Box\n");
    }
    else if (input->KeyPress('C')) {
        OutputDebugString("Cylinder\n");
    }
    else if (input->KeyPress('S')) {
        OutputDebugString("Sphere\n");
    }
    else if (input->KeyPress('G')) {
        OutputDebugString("Geosphere\n");
    }
    else if (input->KeyPress('P')) {
        OutputDebugString("Plane (grid)\n");
    }
    else if (input->KeyPress('Q')) {
        OutputDebugString("Quad\n");
    }
    //Ball, Capsule, House, Monkey e Thorus
    else if (input->KeyPress('1')) {
        OutputDebugString("Ball\n");
    }
    else if (input->KeyPress('2')) {
        OutputDebugString("Capsule\n");

    }
    else if (input->KeyPress('3')) {
        OutputDebugString("House\n");

    }
    else if (input->KeyPress('4')) {
        OutputDebugString("Monkey\n");
    }
    else if (input->KeyPress('5')) {
        OutputDebugString("Thorus\n");

    }

    float mousePosX = (float)input->MouseX();
    float mousePosY = (float)input->MouseY();

    if (input->KeyDown(VK_LBUTTON))
    {
        // cada pixel corresponde a 1/4 de grau
        float dx = XMConvertToRadians(0.25f * (mousePosX - lastMousePosX));
        float dy = XMConvertToRadians(0.25f * (mousePosY - lastMousePosY));

        // atualiza ângulos com base no deslocamento do mouse 
        // para orbitar a câmera ao redor da caixa
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

    // carrega matriz de projeção em uma XMMATRIX
    XMMATRIX proj = XMLoadFloat4x4(&Proj);

    // ajusta o buffer constante de cada objeto
    for (auto& obj : scene)
    {
        // modifica matriz de mundo da esfera
        if (obj.cbIndex == 2)
        {
            XMStoreFloat4x4(&obj.world,
                XMMatrixScaling(0.5f, 0.5f, 0.5f) *
                XMMatrixRotationY(float(timer.Elapsed())) *
                XMMatrixTranslation(0.0f, 0.5f, 0.0f));
        }

        // carrega matriz de mundo em uma XMMATRIX
        XMMATRIX world = XMLoadFloat4x4(&obj.world);      

        // constrói matriz combinada (world x view x proj)
        XMMATRIX WorldViewProj = world * view * proj;        

        // atualiza o buffer constante com a matriz combinada
        ObjectConstants constants;
        XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(WorldViewProj));
        mesh->CopyConstants(&constants, obj.cbIndex);
    }
}

// ------------------------------------------------------------------------------

void Single::Draw()
{
    // limpa o backbuffer
    graphics->Clear(pipelineState);

    // comandos de configuração do pipeline
    ID3D12DescriptorHeap* descriptorHeaps = mesh->ConstantBufferHeap() ;
    graphics->CommandList()->SetDescriptorHeaps(1, &descriptorHeaps);
    graphics->CommandList()->SetGraphicsRootSignature(rootSignature);
    graphics->CommandList()->IASetVertexBuffers(0, 1, mesh->VertexBufferView());
    graphics->CommandList()->IASetIndexBuffer(mesh->IndexBufferView());
    graphics->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // desenha objetos da cena
    for (auto& obj : scene)
    {
        // ajusta o buffer constante associado ao vertex shader
        graphics->CommandList()->SetGraphicsRootDescriptorTable(0, mesh->ConstantBufferHandle(obj.cbIndex));

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

void Single::Finalize()
{
    rootSignature->Release();
    pipelineState->Release();
    delete mesh;
}


// ------------------------------------------------------------------------------
//                                     D3D                                      
// ------------------------------------------------------------------------------

void Single::BuildRootSignature()
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

void Single::BuildPipelineState()
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
        engine->window->Title("Single");
        engine->window->Icon(IDI_ICON);
        engine->window->Cursor(IDC_CURSOR);
        engine->window->LostFocus(Engine::Pause);
        engine->window->InFocus(Engine::Resume);

        // cria e executa a aplicação
        engine->Start(new Single());

        // finaliza execução
        delete engine;
    }
    catch (Error & e)
    {
        // exibe mensagem em caso de erro
        MessageBox(nullptr, e.ToString().data(), "Single", MB_OK);
    }

    return 0;
}

// ----------------------------------------------------------------------------

