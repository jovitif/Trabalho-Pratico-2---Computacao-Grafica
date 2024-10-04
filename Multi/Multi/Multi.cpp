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
#include <windows.h>
#include <iostream>
#include <string> 
using namespace std;

#include <fstream>
#include <sstream>
#include <vector>
#include <DirectXMath.h>
// ------------------------------------------------------------------------------

struct ObjData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

struct ObjectConstants
{
    XMFLOAT4X4 WorldViewProj =
    { 1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f };
};

// ------------------------------------------------------------------------------

class Multi : public App
{
private:
    ID3D12RootSignature* rootSignature = nullptr;
    ID3D12PipelineState* pipelineState = nullptr;
    vector<Object> scene;
    vector<Geometry> vertices;

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
    int selecionado = -1; // mostrar o item selecionado 


public:
    void Init();
    void Update();
    void Draw();
    void Finalize();
    Geometry LoadOBJ(const std::string& filename);
    void BuildRootSignature();
    void BuildPipelineState();
};

Geometry Multi::LoadOBJ(const std::string& filename) {
    Geometry objData;
    ifstream file(filename);
    string line;

    vector<XMFLOAT3> positions;
    vector<XMFLOAT3> normals;

    while (getline(file, line)) {
       istringstream iss(line);
       string prefix;
       iss >> prefix;

        if (prefix == "v") {
            // Vértices (posições)
            XMFLOAT3 position;
            iss >> position.x >> position.y >> position.z;
            positions.push_back(position);
        }
        else if (prefix == "vn") {
            // Normais
            XMFLOAT3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (prefix == "f") {
            // Faces (índices de vértices)
            uint32_t v1, v2, v3;
            uint32_t n1, n2, n3;
            char slash;
            std::string faceStr;
            getline(iss, faceStr);
            std::istringstream faceStream(faceStr);

            if (faceStr.find("//") != std::string::npos) {
                faceStream >> v1 >> slash >> slash >> n1
                    >> v2 >> slash >> slash >> n2
                    >> v3 >> slash >> slash >> n3;
                objData.indices.push_back(v1 - 1);
                objData.indices.push_back(v2 - 1);
                objData.indices.push_back(v3 - 1);
            }
            else {
                uint32_t vt1, vt2, vt3;
                faceStream >> v1 >> slash >> vt1 >> slash >> n1
                    >> v2 >> slash >> vt2 >> slash >> n2
                    >> v3 >> slash >> vt3 >> slash >> n3;
                objData.indices.push_back(v1 - 1);
                objData.indices.push_back(v2 - 1);
                objData.indices.push_back(v3 - 1);
            }
        }
    }

    for (auto& pos : positions) {
        Vertex vertex;
        vertex.pos = pos;
        vertex.color = XMFLOAT4(DirectX::Colors::DimGray);
        objData.vertices.push_back(vertex);
    }
    vertices.push_back(objData);
    return objData;
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

    Grid grid(6.0f, 6.0f, 30, 30);

    for (auto& v : grid.vertices) v.color = XMFLOAT4(DirectX::Colors::DimGray);

    vertices.push_back(grid);
    // ---------------------------------------------------------------
    // Alocação e Cópia de Vertex, Index e Constant Buffers para a GPU
    // ---------------------------------------------------------------


    // grid
    Object gridObj;
    gridObj.mesh = new Mesh();
    gridObj.world = Identity;
    gridObj.mesh->VertexBuffer(grid.VertexData(), grid.VertexCount() * sizeof(Vertex), sizeof(Vertex));
    gridObj.mesh->IndexBuffer(grid.IndexData(), grid.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
    gridObj.mesh->ConstantBuffer(sizeof(ObjectConstants));
    gridObj.submesh.indexCount = grid.IndexCount();
    scene.push_back(gridObj);
 
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
        Box newBox(2.0f, 2.0f, 2.0f);
        graphics->ResetCommands();
        //Colocando cor nos vertices
        for (auto& v : newBox.vertices) {
            v.color = XMFLOAT4(DirectX::Colors::DimGray);
        }
        vertices.push_back(newBox);
        Object obj; //Objeto
        XMStoreFloat4x4(&obj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.0f, 0.0f));

        obj.mesh = new Mesh();
        obj.mesh->VertexBuffer(newBox.VertexData(), newBox.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        obj.mesh->IndexBuffer(newBox.IndexData(), newBox.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        obj.mesh->ConstantBuffer(sizeof(ObjectConstants));
        obj.submesh.indexCount = newBox.IndexCount();
        scene.push_back(obj);

        graphics->SubmitCommands();
    }
    //Tecla C para adicionar Cylinder
    if (input->KeyPress('C') ) {
        Cylinder newCylinder(1.0f, 0.5f, 3.0f, 20, 10); //Cria novo Cylinder
        graphics->ResetCommands();
        //Colocando cor nos vertices
        for (auto& v : newCylinder.vertices) {
            v.color = XMFLOAT4(DirectX::Colors::DimGray);
        }
        vertices.push_back(newCylinder);
        Object obj; //Objeto
        XMStoreFloat4x4(&obj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.0f, 0.0f));

        obj.mesh = new Mesh();
        obj.mesh->VertexBuffer(newCylinder.VertexData(), newCylinder.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        obj.mesh->IndexBuffer(newCylinder.IndexData(), newCylinder.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        obj.mesh->ConstantBuffer(sizeof(ObjectConstants));
        obj.submesh.indexCount = newCylinder.IndexCount();
        scene.push_back(obj);

        graphics->SubmitCommands();
    }

    //Tecla S para adicionar Sphere
    if (input->KeyPress('S')) {
        Sphere newSphere(1.0f, 20, 20); //Cria nova Sphere
        graphics->ResetCommands();
        //Colocando cor nos vertices
        for (auto& v : newSphere.vertices) {
            v.color = XMFLOAT4(DirectX::Colors::DimGray);
        }
        vertices.push_back(newSphere);
        Object obj; //Objeto
        XMStoreFloat4x4(&obj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.0f, 0.0f));

        obj.mesh = new Mesh();
        obj.mesh->VertexBuffer(newSphere.VertexData(), newSphere.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        obj.mesh->IndexBuffer(newSphere.IndexData(), newSphere.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        obj.mesh->ConstantBuffer(sizeof(ObjectConstants));
        obj.submesh.indexCount = newSphere.IndexCount();
        scene.push_back(obj);

        graphics->SubmitCommands();
    }
    //Tecla G para adicionar GeoSphere
    if (input->KeyPress('G')) {
        GeoSphere newGeoSphere(1.0f, 20); //Cria nova GeoSphere
        graphics->ResetCommands();
        //Colocando cor nos vertices
        for (auto& v : newGeoSphere.vertices) {
            v.color = XMFLOAT4(DirectX::Colors::DimGray);
        }
        vertices.push_back(newGeoSphere);
        Object obj; //Objeto
        XMStoreFloat4x4(&obj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.0f, 0.0f));

        obj.mesh = new Mesh();
        obj.mesh->VertexBuffer(newGeoSphere.VertexData(), newGeoSphere.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        obj.mesh->IndexBuffer(newGeoSphere.IndexData(), newGeoSphere.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        obj.mesh->ConstantBuffer(sizeof(ObjectConstants));
        obj.submesh.indexCount = newGeoSphere.IndexCount();
        scene.push_back(obj);

        graphics->SubmitCommands();
    }
    //Tecla P para adicionar Plane(Grid)
    if (input->KeyPress('P')) {
        Grid newGrid(3.0f, 3.0f, 20, 20); //Cria novo Grid
        graphics->ResetCommands();
        //Colocando cor nos vertices
        for (auto& v : newGrid.vertices) {
            v.color = XMFLOAT4(DirectX::Colors::DimGray);
        }
        vertices.push_back(newGrid);
        Object obj; //Objeto
        XMStoreFloat4x4(&obj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.0f, 0.0f));

        obj.mesh = new Mesh();
        obj.mesh->VertexBuffer(newGrid.VertexData(), newGrid.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        obj.mesh->IndexBuffer(newGrid.IndexData(), newGrid.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        obj.mesh->ConstantBuffer(sizeof(ObjectConstants));
        obj.submesh.indexCount = newGrid.IndexCount();
        scene.push_back(obj);

        graphics->SubmitCommands();
    }
    //Tecla Q para adicionar Quad
    if (input->KeyPress('Q') ) {
        Quad newQuad(2.0f, 2.0f); //Cria novo Quad
        graphics->ResetCommands();
        //Colocando cor nos vertices
        for (auto& v : newQuad.vertices) {
            v.color = XMFLOAT4(DirectX::Colors::DimGray);
        }
        vertices.push_back(newQuad);
        Object obj; //Objeto
        XMStoreFloat4x4(&obj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.0f, 0.0f));

        obj.mesh = new Mesh();
        obj.mesh->VertexBuffer(newQuad.VertexData(), newQuad.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        obj.mesh->IndexBuffer(newQuad.IndexData(), newQuad.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        obj.mesh->ConstantBuffer(sizeof(ObjectConstants));
        obj.submesh.indexCount = newQuad.IndexCount();
        scene.push_back(obj);

        graphics->SubmitCommands();
    }
    //Ball, Capsule, House, Monkey e Thorus
    else if (input->KeyPress('1')) {
        OutputDebugString("Ball\n");

        // Carregar o arquivo .obj
        Geometry ballData = LoadOBJ("ball.obj");

        graphics->ResetCommands();

        Object obj; //Objeto
        XMStoreFloat4x4(&obj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.0f, 0.0f));

        obj.mesh = new Mesh();
        obj.mesh->VertexBuffer(ballData.VertexData(), ballData.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        obj.mesh->IndexBuffer(ballData.IndexData(), ballData.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        obj.mesh->ConstantBuffer(sizeof(ObjectConstants));
        obj.submesh.indexCount = ballData.IndexCount();
        scene.push_back(obj);

        graphics->SubmitCommands();
    }
    else if (input->KeyPress('2')) {
        OutputDebugString("Capsule\n");

        // Carregar o arquivo .obj
        Geometry ballData = LoadOBJ("capsule.obj");

        graphics->ResetCommands();

        Object obj; //Objeto
        XMStoreFloat4x4(&obj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.0f, 0.0f));

        obj.mesh = new Mesh();
        obj.mesh->VertexBuffer(ballData.VertexData(), ballData.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        obj.mesh->IndexBuffer(ballData.IndexData(), ballData.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        obj.mesh->ConstantBuffer(sizeof(ObjectConstants));
        obj.submesh.indexCount = ballData.IndexCount();
        scene.push_back(obj);

        graphics->SubmitCommands();
    }
    else if (input->KeyPress('3')) {
        OutputDebugString("House\n");

        // Carregar o arquivo .obj
        Geometry ballData = LoadOBJ("house.obj");

        graphics->ResetCommands();

        Object obj; //Objeto
        XMStoreFloat4x4(&obj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.0f, 0.0f));

        obj.mesh = new Mesh();
        obj.mesh->VertexBuffer(ballData.VertexData(), ballData.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        obj.mesh->IndexBuffer(ballData.IndexData(), ballData.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        obj.mesh->ConstantBuffer(sizeof(ObjectConstants));
        obj.submesh.indexCount = ballData.IndexCount();
        scene.push_back(obj);

        graphics->SubmitCommands();
    }
    else if (input->KeyPress('4')) {
        OutputDebugString("Monkey\n");

        // Carregar o arquivo .obj
        Geometry ballData = LoadOBJ("monkey.obj");

        graphics->ResetCommands();

        Object obj; //Objeto
        XMStoreFloat4x4(&obj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.0f, 0.0f));

        obj.mesh = new Mesh();
        obj.mesh->VertexBuffer(ballData.VertexData(), ballData.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        obj.mesh->IndexBuffer(ballData.IndexData(), ballData.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        obj.mesh->ConstantBuffer(sizeof(ObjectConstants));
        obj.submesh.indexCount = ballData.IndexCount();
        scene.push_back(obj);

        graphics->SubmitCommands();
    }
    else if (input->KeyPress('5')) {
        OutputDebugString("Thorus\n");

        // Carregar o arquivo .obj
        Geometry ballData = LoadOBJ("thorus.obj");

        graphics->ResetCommands();

        Object obj; //Objeto
        XMStoreFloat4x4(&obj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.0f, 0.0f));

        obj.mesh = new Mesh();
        obj.mesh->VertexBuffer(ballData.VertexData(), ballData.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        obj.mesh->IndexBuffer(ballData.IndexData(), ballData.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        obj.mesh->ConstantBuffer(sizeof(ObjectConstants));
        obj.submesh.indexCount = ballData.IndexCount();
        scene.push_back(obj);

        graphics->SubmitCommands();
        }
    //Tab para selecionar figura
    if (input->KeyPress(VK_TAB)) {
        graphics->ResetCommands();

        // Reverte a cor do objeto atual antes de selecionar o pr�ximo
        if (!scene.empty() && selecionado >= 0) {
            for (auto& v : vertices[selecionado].vertices) {
                v.color = XMFLOAT4(DirectX::Colors::DimGray); // ou a cor padr�o que voc� deseja
            }

            // Atualiza o buffer do objeto anterior
            scene[selecionado].mesh->VertexBuffer(vertices[selecionado].VertexData(), vertices[selecionado].VertexCount() * sizeof(Vertex), sizeof(Vertex));
            scene[selecionado].mesh->IndexBuffer(vertices[selecionado].IndexData(), vertices[selecionado].IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
            scene[selecionado].mesh->ConstantBuffer(sizeof(ObjectConstants));
            scene[selecionado].submesh.indexCount = vertices[selecionado].IndexCount();
        }

        // Avan�a para o pr�ximo objeto
        selecionado++;
        if (selecionado >= scene.size()) {
            selecionado = 0; // Volta ao primeiro objeto se ultrapassar o tamanho do vetor
        }

        OutputDebugString(std::to_string(selecionado).c_str());

        // Altera a cor do novo objeto selecionado
        if (!scene.empty()) {
            for (auto& v : vertices[selecionado].vertices) {
                v.color = XMFLOAT4(DirectX::Colors::DarkRed);
            }

            // Atualiza o buffer do objeto novo
            scene[selecionado].mesh->VertexBuffer(vertices[selecionado].VertexData(), vertices[selecionado].VertexCount() * sizeof(Vertex), sizeof(Vertex));
            scene[selecionado].mesh->IndexBuffer(vertices[selecionado].IndexData(), vertices[selecionado].IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
            scene[selecionado].mesh->ConstantBuffer(sizeof(ObjectConstants));
            scene[selecionado].submesh.indexCount = vertices[selecionado].IndexCount();
        }

        graphics->SubmitCommands();
    }

    if (input->KeyPress(VK_SHIFT)) {
        OutputDebugString("Remover select");
        graphics->ResetCommands();

        if (!scene.empty() && selecionado >= 0) {
            // Reverte a cor do objeto selecionado para a cor padrão
            for (auto& v : vertices[selecionado].vertices) {
                v.color = XMFLOAT4(DirectX::Colors::DimGray); // ou a cor padrão que você deseja
            }

            // Atualiza o buffer do objeto
            scene[selecionado].mesh->VertexBuffer(vertices[selecionado].VertexData(), vertices[selecionado].VertexCount() * sizeof(Vertex), sizeof(Vertex));
            scene[selecionado].mesh->IndexBuffer(vertices[selecionado].IndexData(), vertices[selecionado].IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
            scene[selecionado].mesh->ConstantBuffer(sizeof(ObjectConstants));
            scene[selecionado].submesh.indexCount = vertices[selecionado].IndexCount();
        }

        selecionado = -1; // Reseta o índice de seleção
        graphics->SubmitCommands();
    
    }


    if (input->KeyPress(VK_DELETE)) {

        if (selecionado >= 0) {
            graphics->ResetCommands();

            scene.erase(scene.begin() + selecionado);
            vertices.erase(vertices.begin() + selecionado);
            selecionado = -1;
            graphics->SubmitCommands();
        }

    }    // ativa ou desativa o giro do objeto
    if (input->KeyPress('S'))
    {
        spinning = !spinning;

        if (spinning)
            timer.Start();
        else
            timer.Stop();
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

    if (input->KeyDown(VK_CONTROL) && ((input->KeyDown('E') || input->KeyDown('e')) && input->KeyDown(VK_OEM_PLUS))) {
        OutputDebugString("Escala aumentar");

        if (selecionado > -1 && selecionado < scene.size()) {
            graphics->ResetCommands();

            // Aplicando a escala diretamente
            float scaleX = 1.1f;
            float scaleY = 1.1f;
            float scaleZ = 1.1f;

            // Mudando a escala
            XMMATRIX newWorld = XMMatrixScaling(scaleX, scaleY, scaleZ) * XMLoadFloat4x4(&scene[selecionado].world);
            XMStoreFloat4x4(&scene[selecionado].world, newWorld);

            // Atualizando os constantes do objeto
            ObjectConstants constants;
            XMMATRIX wvp = newWorld * XMLoadFloat4x4(&View) * XMLoadFloat4x4(&Proj);
            XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(wvp));
            scene[selecionado].mesh->CopyConstants(&constants);

            graphics->SubmitCommands();
        }
    }

    // Diminuir escala do objeto selecionado com a combinação de teclas
    if (input->KeyDown(VK_CONTROL) && ((input->KeyDown('E') || input->KeyDown('e')) && input->KeyDown(VK_OEM_MINUS))) {
        OutputDebugString("Diminuir a imagem");

        if (selecionado > -1 && selecionado < scene.size()) {
            graphics->ResetCommands();

            // Aplicando a redução de escala diretamente
            float scaleX = 0.9f;
            float scaleY = 0.9f;
            float scaleZ = 0.9f;

            // Mudando a escala
            XMMATRIX newWorld = XMMatrixScaling(scaleX, scaleY, scaleZ) * XMLoadFloat4x4(&scene[selecionado].world);
            XMStoreFloat4x4(&scene[selecionado].world, newWorld);

            // Atualizando os constantes do objeto
            ObjectConstants constants;
            XMMATRIX wvp = newWorld * XMLoadFloat4x4(&View) * XMLoadFloat4x4(&Proj);
            XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(wvp));
            scene[selecionado].mesh->CopyConstants(&constants);

            graphics->SubmitCommands();
        }
    }




    // Rodar no eixo X
    if (input->KeyDown(VK_CONTROL) && (input->KeyDown('X') || input->KeyDown('x')) && (input->KeyPress('R') || input->KeyPress('r'))) {
        OutputDebugString("Rodando no eixo X");

        if (selecionado > -1 && selecionado < scene.size()) {
            graphics->ResetCommands();

            // Convertendo para radianos
            float rotX = XMConvertToRadians(-0.1f);
            float rotY = 0.0f;
            float rotZ = 0.0f;

            // Aplica a rotação
            XMMATRIX w = XMLoadFloat4x4(&scene[selecionado].world);
            w = XMMatrixRotationX(rotX) * XMMatrixRotationY(rotY) * XMMatrixRotationZ(rotZ) * w;

            XMStoreFloat4x4(&scene[selecionado].world, w);

            // Atualizando o buffer de constantes
            XMMATRIX wvp = w * XMLoadFloat4x4(&View) * XMLoadFloat4x4(&Proj);
            ObjectConstants constants;
            XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(wvp));
            scene[selecionado].mesh->CopyConstants(&constants);

            graphics->SubmitCommands();
        }
    }


    // Rodar no eixo Y
    if (input->KeyDown(VK_CONTROL) && (input->KeyDown('Y') || input->KeyDown('y')) && (input->KeyPress('R') || input->KeyPress('r'))) {
        OutputDebugString("Rodando no eixo Y");

        if (selecionado > -1 && selecionado < scene.size()) {
            graphics->ResetCommands();

            // Convertendo para radianos
            float rotX = 0.0f;
            float rotY = XMConvertToRadians(-0.1f);
            float rotZ = 0.0f;

            // Aplica a rotação
            XMMATRIX w = XMLoadFloat4x4(&scene[selecionado].world);
            w = XMMatrixRotationX(rotX) * XMMatrixRotationY(rotY) * XMMatrixRotationZ(rotZ) * w;

            XMStoreFloat4x4(&scene[selecionado].world, w);

            // Atualizando o buffer de constantes
            XMMATRIX wvp = w * XMLoadFloat4x4(&View) * XMLoadFloat4x4(&Proj);
            ObjectConstants constants;
            XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(wvp));
            scene[selecionado].mesh->CopyConstants(&constants);

            graphics->SubmitCommands();
        }
    }


    // Rodar no eixo Z
    if (input->KeyDown(VK_CONTROL) && (input->KeyDown('Z') || input->KeyDown('z')) && (input->KeyPress('R') || input->KeyPress('r'))) {
        OutputDebugString("Rodando no eixo Z");

        if (selecionado > -1 && selecionado < scene.size()) {
            graphics->ResetCommands();

            // Convertendo para radianos
            float rotX = 0.0f;
            float rotY = 0.0f;
            float rotZ = XMConvertToRadians(-0.1f);

            // Aplica a rotação
            XMMATRIX w = XMLoadFloat4x4(&scene[selecionado].world);
            w = XMMatrixRotationX(rotX) * XMMatrixRotationY(rotY) * XMMatrixRotationZ(rotZ) * w;

            XMStoreFloat4x4(&scene[selecionado].world, w);

            // Atualizando o buffer de constantes
            XMMATRIX wvp = w * XMLoadFloat4x4(&View) * XMLoadFloat4x4(&Proj);
            ObjectConstants constants;
            XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(wvp));
            scene[selecionado].mesh->CopyConstants(&constants);

            graphics->SubmitCommands();
        }
    }

    // Translação para a direita (eixo X positivo)
    if (input->KeyDown('T') && input->KeyDown(VK_RIGHT)) {
        OutputDebugString("Translação no eixo direita");

        if (selecionado > -1 && selecionado < scene.size()) {
            graphics->ResetCommands();

            // Translação no eixo X (direita)
            XMMATRIX translation = XMMatrixTranslation(0.1f, 0.0f, 0.0f);
            XMMATRIX newWorld = translation * XMLoadFloat4x4(&scene[selecionado].world);
            XMStoreFloat4x4(&scene[selecionado].world, newWorld);

            // Atualizando o buffer de constantes
            ObjectConstants constants;
            XMMATRIX wvp = newWorld * XMLoadFloat4x4(&View) * XMLoadFloat4x4(&Proj);
            XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(wvp));
            scene[selecionado].mesh->CopyConstants(&constants);

            graphics->SubmitCommands();
        }
    }

    // Translação para a esquerda (eixo X negativo)
    if (input->KeyDown('T') && input->KeyDown(VK_LEFT)) {
        OutputDebugString("Tranlação no eixo da esquerda");

        if (selecionado > -1 && selecionado < scene.size()) {
            graphics->ResetCommands();

            // Translação no eixo X (esquerda)
            XMMATRIX translation = XMMatrixTranslation(-0.1f, 0.0f, 0.0f);
            XMMATRIX newWorld = translation * XMLoadFloat4x4(&scene[selecionado].world);
            XMStoreFloat4x4(&scene[selecionado].world, newWorld);

            // Atualizando o buffer de constantes
            ObjectConstants constants;
            XMMATRIX wvp = newWorld * XMLoadFloat4x4(&View) * XMLoadFloat4x4(&Proj);
            XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(wvp));
            scene[selecionado].mesh->CopyConstants(&constants);

            graphics->SubmitCommands();
        }
    }

    // Translação para cima (eixo Y positivo)
    if (input->KeyDown('T') && input->KeyDown(VK_UP)) {
        OutputDebugString("Translação para cima");

        if (selecionado > -1 && selecionado < scene.size()) {
            graphics->ResetCommands();

            // Translação no eixo Y (cima)
            XMMATRIX translation = XMMatrixTranslation(0.0f, 0.1f, 0.0f);
            XMMATRIX newWorld = translation * XMLoadFloat4x4(&scene[selecionado].world);
            XMStoreFloat4x4(&scene[selecionado].world, newWorld);

            // Atualizando o buffer de constantes
            ObjectConstants constants;
            XMMATRIX wvp = newWorld * XMLoadFloat4x4(&View) * XMLoadFloat4x4(&Proj);
            XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(wvp));
            scene[selecionado].mesh->CopyConstants(&constants);

            graphics->SubmitCommands();
        }
    }

    // Translação para baixo (eixo Y negativo)
    if (input->KeyDown('T') && input->KeyDown(VK_DOWN)) {
        OutputDebugString("Translação para Baixo");

        if (selecionado > -1 && selecionado < scene.size()) {
            graphics->ResetCommands();

            // Translação no eixo Y (baixo)
            XMMATRIX translation = XMMatrixTranslation(0.0f, -0.1f, 0.0f);
            XMMATRIX newWorld = translation * XMLoadFloat4x4(&scene[selecionado].world);
            XMStoreFloat4x4(&scene[selecionado].world, newWorld);

            // Atualizando o buffer de constantes
            ObjectConstants constants;
            XMMATRIX wvp = newWorld * XMLoadFloat4x4(&View) * XMLoadFloat4x4(&Proj);
            XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(wvp));
            scene[selecionado].mesh->CopyConstants(&constants);

            graphics->SubmitCommands();
        }
    }

    // Translação para frente (eixo Z positivo)
    if (input->KeyDown('T') && input->KeyDown('W')) {
        OutputDebugString("Entrei - Frente");

        if (selecionado > -1 && selecionado < scene.size()) {
            graphics->ResetCommands();

            // Translação no eixo Z (frente)
            XMMATRIX translation = XMMatrixTranslation(0.0f, 0.0f, 0.1f);
            XMMATRIX newWorld = translation * XMLoadFloat4x4(&scene[selecionado].world);
            XMStoreFloat4x4(&scene[selecionado].world, newWorld);

            // Atualizando o buffer de constantes
            ObjectConstants constants;
            XMMATRIX wvp = newWorld * XMLoadFloat4x4(&View) * XMLoadFloat4x4(&Proj);
            XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(wvp));
            scene[selecionado].mesh->CopyConstants(&constants);

            graphics->SubmitCommands();
        }
    }

    // Translação para trás (eixo Z negativo)
    if (input->KeyDown('T') && input->KeyDown('S')) {
        OutputDebugString("Translação para trás");

        if (selecionado > -1 && selecionado < scene.size()) {
            graphics->ResetCommands();

            // Translação no eixo Z (trás)
            XMMATRIX translation = XMMatrixTranslation(0.0f, 0.0f, -0.1f);
            XMMATRIX newWorld = translation * XMLoadFloat4x4(&scene[selecionado].world);
            XMStoreFloat4x4(&scene[selecionado].world, newWorld);

            // Atualizando o buffer de constantes
            ObjectConstants constants;
            XMMATRIX wvp = newWorld * XMLoadFloat4x4(&View) * XMLoadFloat4x4(&Proj);
            XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(wvp));
            scene[selecionado].mesh->CopyConstants(&constants);

            graphics->SubmitCommands();
        }
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
    for (auto & obj : scene)
    {
        // carrega matriz de mundo em uma XMMATRIX
        XMMATRIX world = XMLoadFloat4x4(&obj.world);      

        // constrói matriz combinada (world x view x proj)
        XMMATRIX WorldViewProj = world * view * proj;        

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

