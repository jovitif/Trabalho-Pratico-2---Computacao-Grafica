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

class Single : public App
{
private:
    ID3D12RootSignature* rootSignature = nullptr;
    ID3D12PipelineState* pipelineState = nullptr;
    vector<Object> scene;
    Mesh* mesh = nullptr;

    Timer timer;
    bool spinning = false; // O objeto não gira por padrão

    XMFLOAT4X4 Identity = {};
    XMFLOAT4X4 View = {};
    XMFLOAT4X4 Proj = {};

    float theta = 0;
    float phi = 0;
    float radius = 0;
    float lastMousePosX = 0;
    float lastMousePosY = 0;
    bool topView = false;
    float offset = 3.0f;
    Object obj;
    bool boxCriada = false;
    uint selecionado = -1; // mostrar o item selecionado 
    //contadores
    uint indexCount = 0;
    uint vertexCount = 0;

    vector<Vertex> vertices;

    uint totalVertexCount = 0;
    uint totalIndexCount = 0;
    uint vbSize = 0;

    uint ibSize = 0;

    uint k = 0;

    vector<uint> indices;

    int boxCount = 0;

    uint cbIndex = 1; // inicialmente um objeto em cena

public:
    void Init();
    void Update();
    void Draw();
    void Finalize();
    ObjData LoadOBJ(const std::string& filename);
    void BuildRootSignature();
    void BuildPipelineState();
};

ObjData Single::LoadOBJ(const std::string& filename) {
    ObjData objData;
    std::ifstream file(filename);
    std::string line;

    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT3> normals;
    std::vector<uint32_t> faces;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            // Vértices (posições)
            XMFLOAT3 position;
            iss >> position.x >> position.y >> position.z;
            positions.push_back(position);
        }
        else if (prefix == "vt") {
            // Normais
            XMFLOAT3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (prefix == "f") {
            // Faces (índices de vértices, coordenadas de textura e normais)
            uint32_t v1, v2, v3;
            uint32_t vt1, vt2, vt3; // Coordenadas de textura
            uint32_t n1, n2, n3;    // Normais
            char slash;

            // Verifica se o formato é v/vt/vn
            std::string faceStr;
            getline(iss, faceStr);
            std::istringstream faceStream(faceStr);

            if (faceStr.find("//") != std::string::npos) {

                // Leitura do formato v//vn (sem coordenadas de textura)
                faceStream >> v1 >> slash >> slash >> n1
                    >> v2 >> slash >> slash >> n2
                    >> v3 >> slash >> slash >> n3;

                // Armazena os índices dos vértices (convertendo de 1-based para 0-based)
                objData.indices.push_back(v1 - 1);
                objData.indices.push_back(v2 - 1);
                objData.indices.push_back(v3 - 1);

               
            }
            else {

                // Leitura do formato v/vt/vn
                faceStream >> v1 >> slash >> vt1 >> slash >> n1
                    >> v2 >> slash >> vt2 >> slash >> n2
                    >> v3 >> slash >> vt3 >> slash >> n3;

                // Armazena os índices dos vértices (convertendo de 1-based para 0-based)
                objData.indices.push_back(v1 - 1);
                objData.indices.push_back(v2 - 1);
                objData.indices.push_back(v3 - 1);




            }
        }

    }
    for (auto& pos : positions) {
        Vertex vertex;
        vertex.pos = pos;
        vertex.color = XMFLOAT4(DirectX::Colors::DimGray); // Definir cor padrão
        objData.vertices.push_back(vertex);
    }

    return objData;
}

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

    Grid grid(6.0f, 6.0f, 30, 30);

    totalVertexCount = (grid.VertexCount()); // quantidade de vertices

    vbSize = totalVertexCount * sizeof(Vertex); // tamanho em bytes dos vértices

    totalIndexCount = (grid.IndexCount());     // quantidade total de índices

    ibSize = totalIndexCount * sizeof(uint);     // tamanho em bytes dos índices

    vertices.resize(totalVertexCount);    // junta todos os vértices em um único vetor

    for (uint i = 0; i < grid.VertexCount(); ++i, ++k)
    {
        vertices[k].pos = grid.vertices[i].pos;
        vertices[k].color = XMFLOAT4(DirectX::Colors::DimGray);
    }

    // junta todos os índices em um único vetor
 
    indices.insert(indices.end(), begin(grid.indices), end(grid.indices));

  
    SubMesh gridSubMesh;
    gridSubMesh.indexCount = uint(grid.IndexCount());
    gridSubMesh.startIndex = 0;
    gridSubMesh.baseVertex = 0;
  
    obj.world = Identity;
    obj.cbIndex = 0;
    obj.submesh = gridSubMesh;
    scene.push_back(obj);


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
       
    }
    // b: box, c:cylinder, s:sphere, g: geosphere, p: plane (grid), q:quad
    if (input->KeyPress('B')) {
        OutputDebugString("Box criada\n");
        graphics->ResetCommands();

        Box newBox(2.0f, 2.0f, 2.0f); //Cria BOX

        uint newTotalVertexCount = totalVertexCount + newBox.VertexCount(); //Calcula quantidade de novos vertices

        const uint vbSize = newTotalVertexCount * sizeof(Vertex); //Calcula vbSize nova

        uint newTotalIndexCount = totalIndexCount + newBox.IndexCount(); //Calcula quantidade novos indices

        const uint ibSize = newTotalIndexCount * sizeof(uint); //Calcula ibsize nova

        std::vector<Vertex> aux(newTotalVertexCount); // Cria um std::vector com newTotalVertexCount elementos.
        int k = 0;
        for (uint i{}; i < totalVertexCount; i++, k++) { //Copia dados do vetor antigo
            aux[i].pos = vertices[i].pos;
            aux[i].color = vertices[i].color;
        }

        for (uint i{}; i < newBox.VertexCount(); i++, k++) {
            aux[k].pos = newBox.vertices[i].pos;
            aux[k].color = XMFLOAT4(DirectX::Colors::DimGray);
        }

        indices.insert(indices.end(), begin(newBox.indices), end(newBox.indices)); //Insere em vector de indices

        SubMesh newBoxSubMesh; //Definindo subMalha
        newBoxSubMesh.indexCount = uint(newBox.IndexCount());
        newBoxSubMesh.startIndex = totalIndexCount;
        newBoxSubMesh.baseVertex = totalVertexCount;

        //Definição do Objeto
        Object obj;

        XMStoreFloat4x4(&obj.world,
            XMMatrixScaling(0.4f, 0.4f, 0.4f) *
            XMMatrixTranslation(0.0f, 0.0f, 0.0f));
        obj.cbIndex = cbIndex; //Recebe numero de objetos *COMEÇA COM UM POR JA INICIAR COM O GRID*
        obj.submesh = newBoxSubMesh; //Recebe submalha da nova BOX
        scene.push_back(obj); //Coloca no vetor

        cbIndex++; //Atualiza quantidade de objetos na cena


        totalVertexCount = newTotalVertexCount;
        totalIndexCount = newTotalIndexCount;

        //OutputDebugString(std::to_string(totalVertexCount).c_str());

        vertices = aux;

        //mesh = new Mesh();
        mesh->VertexBuffer(vertices.data(), vbSize, sizeof(Vertex));
        mesh->IndexBuffer(indices.data(), ibSize, DXGI_FORMAT_R32_UINT);
        mesh->ConstantBuffer(sizeof(ObjectConstants), uint(scene.size()));

        // ---------------------------------------

        graphics->SubmitCommands();
    }

    else if (input->KeyPress('C')) {
        OutputDebugString("Cylinder\n");
        graphics->ResetCommands();

        Cylinder newCylinder(1.0f, 0.5f, 3.0f, 20, 10); //Cria novo Cylinder

        uint newTotalVertexCount = totalVertexCount + newCylinder.VertexCount(); //Calcula quantidade de novos vertices

        const uint vbSize = newTotalVertexCount * sizeof(Vertex); //Calcula vbSize nova

        uint newTotalIndexCount = totalIndexCount + newCylinder.IndexCount(); //Calcula quantidade novos indices

        const uint ibSize = newTotalIndexCount * sizeof(uint); //Calcula ibsize nova

        std::vector<Vertex> aux(newTotalVertexCount); // Cria um std::vector com newTotalVertexCount elementos.
        int k = 0;

        for (uint i{}; i < totalVertexCount; i++, k++) {
            aux[i] = vertices[i];
        }

        for (uint i{}; i < newCylinder.VertexCount(); i++, k++) {
            aux[k].pos = newCylinder.vertices[i].pos;
            aux[k].color = XMFLOAT4(DirectX::Colors::DimGray);
        }

        indices.insert(indices.end(), begin(newCylinder.indices), end(newCylinder.indices));

        SubMesh newCylinderSubMesh;
        newCylinderSubMesh.indexCount = uint(newCylinder.IndexCount());
        newCylinderSubMesh.startIndex = totalIndexCount;
        newCylinderSubMesh.baseVertex = totalVertexCount;

        Object obj;
        // cylinder
        XMStoreFloat4x4(&obj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.0f, 0.0f));
        obj.cbIndex = cbIndex;
        obj.submesh = newCylinderSubMesh;
        scene.push_back(obj);

        cbIndex++;


        totalVertexCount = newTotalVertexCount;
        totalIndexCount = newTotalIndexCount;

        //OutputDebugString(std::to_string(totalVertexCount).c_str());

        vertices = aux;

        //mesh = new Mesh();
        mesh->VertexBuffer(vertices.data(), vbSize, sizeof(Vertex));
        mesh->IndexBuffer(indices.data(), ibSize, DXGI_FORMAT_R32_UINT);
        mesh->ConstantBuffer(sizeof(ObjectConstants), uint(scene.size()));

        graphics->SubmitCommands();
    }
    else if (input->KeyPress('S')) {
        OutputDebugString("Sphere\n");
        graphics->ResetCommands();

        Sphere newSphere(1.0f, 20, 20); //Cria nova Sphere

        uint newTotalVertexCount = totalVertexCount + newSphere.VertexCount(); //Calcula quantidade de novos vertices

        const uint vbSize = newTotalVertexCount * sizeof(Vertex); //Calcula vbSize nova

        uint newTotalIndexCount = totalIndexCount + newSphere.IndexCount(); //Calcula quantidade novos indices

        const uint ibSize = newTotalIndexCount * sizeof(uint); //Calcula ibsize nova

        std::vector<Vertex> aux(newTotalVertexCount); // Cria um std::vector com newTotalVertexCount elementos.
        int k = 0;

        for (uint i{}; i < totalVertexCount; i++, k++) {
            aux[i] = vertices[i];
        }

        for (uint i{}; i < newSphere.VertexCount(); i++, k++) {
            aux[k].pos = newSphere.vertices[i].pos;
            aux[k].color = XMFLOAT4(DirectX::Colors::DimGray);
        }

        indices.insert(indices.end(), begin(newSphere.indices), end(newSphere.indices));

        SubMesh newSphereSubMesh;
        newSphereSubMesh.indexCount = uint(newSphere.IndexCount());
        newSphereSubMesh.startIndex = totalIndexCount;
        newSphereSubMesh.baseVertex = totalVertexCount;

        Object obj;
        // Sphere
        XMStoreFloat4x4(&obj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.0f, 0.0f));
        obj.cbIndex = cbIndex;
        obj.submesh = newSphereSubMesh;
        scene.push_back(obj);

        cbIndex++;


        totalVertexCount = newTotalVertexCount;
        totalIndexCount = newTotalIndexCount;

        //OutputDebugString(std::to_string(totalVertexCount).c_str());

        vertices = aux;

        //mesh = new Mesh();
        mesh->VertexBuffer(vertices.data(), vbSize, sizeof(Vertex));
        mesh->IndexBuffer(indices.data(), ibSize, DXGI_FORMAT_R32_UINT);
        mesh->ConstantBuffer(sizeof(ObjectConstants), uint(scene.size()));

        graphics->SubmitCommands();
    }
    else if (input->KeyPress('G')) {
        OutputDebugString("Geosphere\n");
        graphics->ResetCommands();

        GeoSphere newGeoSphere(1.0f, 20); //Cria nova GeoSphere

        uint newTotalVertexCount = totalVertexCount + newGeoSphere.VertexCount(); //Calcula quantidade de novos vertices

        const uint vbSize = newTotalVertexCount * sizeof(Vertex); //Calcula vbSize nova

        uint newTotalIndexCount = totalIndexCount + newGeoSphere.IndexCount(); //Calcula quantidade novos indices

        const uint ibSize = newTotalIndexCount * sizeof(uint); //Calcula ibsize nova

        std::vector<Vertex> aux(newTotalVertexCount); // Cria um std::vector com newTotalVertexCount elementos.
        int k = 0;

        for (uint i{}; i < totalVertexCount; i++, k++) {
            aux[i] = vertices[i];
        }

        for (uint i{}; i < newGeoSphere.VertexCount(); i++, k++) {
            aux[k].pos = newGeoSphere.vertices[i].pos;
            aux[k].color = XMFLOAT4(DirectX::Colors::DimGray);
        }

        indices.insert(indices.end(), begin(newGeoSphere.indices), end(newGeoSphere.indices));

        SubMesh newGeoSphereSubMesh;
        newGeoSphereSubMesh.indexCount = uint(newGeoSphere.IndexCount());
        newGeoSphereSubMesh.startIndex = totalIndexCount;
        newGeoSphereSubMesh.baseVertex = totalVertexCount;

        Object obj;
        // GeoSphere
        XMStoreFloat4x4(&obj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.0f, 0.0f));
        obj.cbIndex = cbIndex;
        obj.submesh = newGeoSphereSubMesh;
        scene.push_back(obj);

        cbIndex++;


        totalVertexCount = newTotalVertexCount;
        totalIndexCount = newTotalIndexCount;

        //OutputDebugString(std::to_string(totalVertexCount).c_str());

        vertices = aux;

        //mesh = new Mesh();
        mesh->VertexBuffer(vertices.data(), vbSize, sizeof(Vertex));
        mesh->IndexBuffer(indices.data(), ibSize, DXGI_FORMAT_R32_UINT);
        mesh->ConstantBuffer(sizeof(ObjectConstants), uint(scene.size()));

        graphics->SubmitCommands();
    }
    else if (input->KeyPress('P')) {
        OutputDebugString("Plane (grid)\n");
        graphics->ResetCommands();

        Grid newGrid(3.0f, 3.0f, 20, 20); //Cria novo Grid

        uint newTotalVertexCount = totalVertexCount + newGrid.VertexCount(); //Calcula quantidade de novos vertices

        const uint vbSize = newTotalVertexCount * sizeof(Vertex); //Calcula vbSize nova

        uint newTotalIndexCount = totalIndexCount + newGrid.IndexCount(); //Calcula quantidade novos indices

        const uint ibSize = newTotalIndexCount * sizeof(uint); //Calcula ibsize nova

        std::vector<Vertex> aux(newTotalVertexCount); // Cria um std::vector com newTotalVertexCount elementos.
        int k = 0;

        for (uint i{}; i < totalVertexCount; i++, k++) {
            aux[i] = vertices[i];
        }

        for (uint i{}; i < newGrid.VertexCount(); i++, k++) {
            aux[k].pos = newGrid.vertices[i].pos;
            aux[k].color = XMFLOAT4(DirectX::Colors::DimGray);
        }

        indices.insert(indices.end(), begin(newGrid.indices), end(newGrid.indices));

        SubMesh newGridSubMesh;
        newGridSubMesh.indexCount = uint(newGrid.IndexCount());
        newGridSubMesh.startIndex = totalIndexCount;
        newGridSubMesh.baseVertex = totalVertexCount;

        Object obj;
        // Grid
        XMStoreFloat4x4(&obj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.5f, 0.0f));
        obj.cbIndex = cbIndex;
        obj.submesh = newGridSubMesh;
        scene.push_back(obj);

        cbIndex++;

        totalVertexCount = newTotalVertexCount;
        totalIndexCount = newTotalIndexCount;

        //OutputDebugString(std::to_string(totalVertexCount).c_str());

        vertices = aux;

        //mesh = new Mesh();
        mesh->VertexBuffer(vertices.data(), vbSize, sizeof(Vertex));
        mesh->IndexBuffer(indices.data(), ibSize, DXGI_FORMAT_R32_UINT);
        mesh->ConstantBuffer(sizeof(ObjectConstants), uint(scene.size()));

        graphics->SubmitCommands();
    }
    else if (input->KeyPress('Q')) {
        OutputDebugString("Quad\n");
        graphics->ResetCommands();

        Quad newQuad(2.0f, 2.0f); //Cria novo Quad

        uint newTotalVertexCount = totalVertexCount + newQuad.VertexCount(); //Calcula quantidade de novos vertices

        const uint vbSize = newTotalVertexCount * sizeof(Vertex); //Calcula vbSize nova

        uint newTotalIndexCount = totalIndexCount + newQuad.IndexCount(); //Calcula quantidade novos indices

        const uint ibSize = newTotalIndexCount * sizeof(uint); //Calcula ibsize nova

        std::vector<Vertex> aux(newTotalVertexCount); // Cria um std::vector com newTotalVertexCount elementos.
        int k = 0;

        for (uint i{}; i < totalVertexCount; i++, k++) {
            aux[i] = vertices[i];
        }

        for (uint i{}; i < newQuad.VertexCount(); i++, k++) {
            aux[k].pos = newQuad.vertices[i].pos;
            aux[k].color = XMFLOAT4(DirectX::Colors::DimGray);
        }

        indices.insert(indices.end(), begin(newQuad.indices), end(newQuad.indices));

        SubMesh newQuadSubMesh;
        newQuadSubMesh.indexCount = uint(newQuad.IndexCount());
        newQuadSubMesh.startIndex = totalIndexCount;
        newQuadSubMesh.baseVertex = totalVertexCount;

        Object obj;
        // Quad
        XMStoreFloat4x4(&obj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.0f, 0.0f));
        obj.cbIndex = cbIndex;
        obj.submesh = newQuadSubMesh;
        scene.push_back(obj);

        cbIndex++;


        totalVertexCount = newTotalVertexCount;
        totalIndexCount = newTotalIndexCount;

        //OutputDebugString(std::to_string(totalVertexCount).c_str());

        vertices = aux;

        //mesh = new Mesh();
        mesh->VertexBuffer(vertices.data(), vbSize, sizeof(Vertex));
        mesh->IndexBuffer(indices.data(), ibSize, DXGI_FORMAT_R32_UINT);
        mesh->ConstantBuffer(sizeof(ObjectConstants), uint(scene.size()));

        graphics->SubmitCommands();
    }
    //Ball, Capsule, House, Monkey e Thorus
    else if (input->KeyPress('1')) {
        OutputDebugString("Ball\n");
        graphics->ResetCommands();

        // Carregar o arquivo .obj
        ObjData ballData = LoadOBJ("ball.obj");

        // Atualizar a quantidade de vértices e índices
        uint newTotalVertexCount = totalVertexCount + uint(ballData.vertices.size());
        const uint vbSize = newTotalVertexCount * sizeof(Vertex);

        uint newTotalIndexCount = totalIndexCount + uint(ballData.indices.size());
        const uint ibSize = newTotalIndexCount * sizeof(uint);

        // Expandir o vetor de vértices e índices
        std::vector<Vertex> aux(newTotalVertexCount);
        int k = 0;

        for (uint i{}; i < totalVertexCount; i++, k++) {
            aux[i] = vertices[i]; // Copia os vértices já existentes
        }

        for (uint i{}; i < ballData.vertices.size(); i++, k++) {
            aux[k] = ballData.vertices[i]; // Adiciona os novos vértices
        }

        indices.insert(indices.end(), begin(ballData.indices), end(ballData.indices)); // Adiciona os novos índices

        // Definir submesh para o novo objeto
        SubMesh newBallSubMesh;
        newBallSubMesh.indexCount = uint(ballData.indices.size());
        newBallSubMesh.startIndex = totalIndexCount;
        newBallSubMesh.baseVertex = totalVertexCount;

        Object obj;
        XMStoreFloat4x4(&obj.world, XMMatrixScaling(0.5f, 0.5f, 0.5f)); // Escala do objeto
        obj.cbIndex = cbIndex;
        obj.submesh = newBallSubMesh;
        scene.push_back(obj); // Adiciona à cena

        cbIndex++;

        totalVertexCount = newTotalVertexCount;
        totalIndexCount = newTotalIndexCount;

        vertices = aux;

        // Atualiza buffers de vértices e índices
        mesh->VertexBuffer(vertices.data(), vbSize, sizeof(Vertex));
        mesh->IndexBuffer(indices.data(), ibSize, DXGI_FORMAT_R32_UINT);
        mesh->ConstantBuffer(sizeof(ObjectConstants), uint(scene.size()));

        graphics->SubmitCommands();
        }

    else if (input->KeyPress('2')) {
        OutputDebugString("Capsule\n");
        graphics->ResetCommands();

        // Carregar o arquivo .obj
        ObjData capsuleData = LoadOBJ("capsule.obj");

        // Atualizar a quantidade de vértices e índices
        uint newTotalVertexCount = totalVertexCount + uint(capsuleData.vertices.size());
        const uint vbSize = newTotalVertexCount * sizeof(Vertex);

        uint newTotalIndexCount = totalIndexCount + uint(capsuleData.indices.size());
        const uint ibSize = newTotalIndexCount * sizeof(uint);

        // Expandir o vetor de vértices e índices
        std::vector<Vertex> aux(newTotalVertexCount);
        int k = 0;

        for (uint i{}; i < totalVertexCount; i++, k++) {
            aux[i] = vertices[i]; // Copia os vértices já existentes
        }

        for (uint i{}; i < capsuleData.vertices.size(); i++, k++) {
            aux[k] = capsuleData.vertices[i]; // Adiciona os novos vértices
        }

        indices.insert(indices.end(), begin(capsuleData.indices), end(capsuleData.indices)); // Adiciona os novos índices

        // Definir submesh para o novo objeto
        SubMesh newCapsuleSubMesh;
        newCapsuleSubMesh.indexCount = uint(capsuleData.indices.size());
        newCapsuleSubMesh.startIndex = totalIndexCount;
        newCapsuleSubMesh.baseVertex = totalVertexCount;

        Object obj;
        XMStoreFloat4x4(&obj.world, XMMatrixScaling(0.5f, 0.5f, 0.5f)); // Escala do objeto
        obj.cbIndex = cbIndex;
        obj.submesh = newCapsuleSubMesh;
        scene.push_back(obj); // Adiciona à cena

        cbIndex++;

        totalVertexCount = newTotalVertexCount;
        totalIndexCount = newTotalIndexCount;

        vertices = aux;

        // Atualiza buffers de vértices e índices
        mesh->VertexBuffer(vertices.data(), vbSize, sizeof(Vertex));
        mesh->IndexBuffer(indices.data(), ibSize, DXGI_FORMAT_R32_UINT);
        mesh->ConstantBuffer(sizeof(ObjectConstants), uint(scene.size()));

        graphics->SubmitCommands();
    }
    else if (input->KeyPress('3')) {
        OutputDebugString("House\n");
        graphics->ResetCommands();

        // Carregar o arquivo .obj
        ObjData houseData = LoadOBJ("house.obj");

        // Atualizar a quantidade de vértices e índices
        uint newTotalVertexCount = totalVertexCount + uint(houseData.vertices.size());
        const uint vbSize = newTotalVertexCount * sizeof(Vertex);

        uint newTotalIndexCount = totalIndexCount + uint(houseData.indices.size());
        const uint ibSize = newTotalIndexCount * sizeof(uint);

        // Expandir o vetor de vértices e índices
        std::vector<Vertex> aux(newTotalVertexCount);
        int k = 0;

        for (uint i{}; i < totalVertexCount; i++, k++) {
            aux[i] = vertices[i]; // Copia os vértices já existentes
        }

        for (uint i{}; i < houseData.vertices.size(); i++, k++) {
            aux[k] = houseData.vertices[i]; // Adiciona os novos vértices
        }

        indices.insert(indices.end(), begin(houseData.indices), end(houseData.indices)); // Adiciona os novos índices

        // Definir submesh para o novo objeto
        SubMesh newHouseSubMesh;
        newHouseSubMesh.indexCount = uint(houseData.indices.size());
        newHouseSubMesh.startIndex = totalIndexCount;
        newHouseSubMesh.baseVertex = totalVertexCount;

        Object obj;
        XMStoreFloat4x4(&obj.world, XMMatrixScaling(0.5f, 0.5f, 0.5f)); // Escala do objeto
        obj.cbIndex = cbIndex;
        obj.submesh = newHouseSubMesh;
        scene.push_back(obj); // Adiciona à cena

        cbIndex++;

        totalVertexCount = newTotalVertexCount;
        totalIndexCount = newTotalIndexCount;

        vertices = aux;

        // Atualiza buffers de vértices e índices
        mesh->VertexBuffer(vertices.data(), vbSize, sizeof(Vertex));
        mesh->IndexBuffer(indices.data(), ibSize, DXGI_FORMAT_R32_UINT);
        mesh->ConstantBuffer(sizeof(ObjectConstants), uint(scene.size()));

        graphics->SubmitCommands();
    }
    else if (input->KeyPress('4')) {
        OutputDebugString("Monkey\n");
        graphics->ResetCommands();

        // Carregar o arquivo .obj
        ObjData houseData = LoadOBJ("monkey.obj");

        // Atualizar a quantidade de vértices e índices
        uint newTotalVertexCount = totalVertexCount + uint(houseData.vertices.size());
        const uint vbSize = newTotalVertexCount * sizeof(Vertex);

        uint newTotalIndexCount = totalIndexCount + uint(houseData.indices.size());
        const uint ibSize = newTotalIndexCount * sizeof(uint);

        // Expandir o vetor de vértices e índices
        std::vector<Vertex> aux(newTotalVertexCount);
        int k = 0;

        for (uint i{}; i < totalVertexCount; i++, k++) {
            aux[i] = vertices[i]; // Copia os vértices já existentes
        }

        for (uint i{}; i < houseData.vertices.size(); i++, k++) {
            aux[k] = houseData.vertices[i]; // Adiciona os novos vértices
        }

        indices.insert(indices.end(), begin(houseData.indices), end(houseData.indices)); // Adiciona os novos índices

        // Definir submesh para o novo objeto
        SubMesh newHouseSubMesh;
        newHouseSubMesh.indexCount = uint(houseData.indices.size());
        newHouseSubMesh.startIndex = totalIndexCount;
        newHouseSubMesh.baseVertex = totalVertexCount;

        Object obj;
        XMStoreFloat4x4(&obj.world, XMMatrixScaling(0.5f, 0.5f, 0.5f)); // Escala do objeto
        obj.cbIndex = cbIndex;
        obj.submesh = newHouseSubMesh;
        scene.push_back(obj); // Adiciona à cena

        cbIndex++;

        totalVertexCount = newTotalVertexCount;
        totalIndexCount = newTotalIndexCount;

        vertices = aux;

        // Atualiza buffers de vértices e índices
        mesh->VertexBuffer(vertices.data(), vbSize, sizeof(Vertex));
        mesh->IndexBuffer(indices.data(), ibSize, DXGI_FORMAT_R32_UINT);
        mesh->ConstantBuffer(sizeof(ObjectConstants), uint(scene.size()));

        graphics->SubmitCommands();
    }
    else if (input->KeyPress('5')) {
        OutputDebugString("Thorus\n");
        graphics->ResetCommands();

        // Carregar o arquivo .obj
        ObjData houseData = LoadOBJ("thorus.obj");

        // Atualizar a quantidade de vértices e índices
        uint newTotalVertexCount = totalVertexCount + uint(houseData.vertices.size());
        const uint vbSize = newTotalVertexCount * sizeof(Vertex);

        uint newTotalIndexCount = totalIndexCount + uint(houseData.indices.size());
        const uint ibSize = newTotalIndexCount * sizeof(uint);

        // Expandir o vetor de vértices e índices
        std::vector<Vertex> aux(newTotalVertexCount);
        int k = 0;

        for (uint i{}; i < totalVertexCount; i++, k++) {
            aux[i] = vertices[i]; // Copia os vértices já existentes
        }

        for (uint i{}; i < houseData.vertices.size(); i++, k++) {
            aux[k] = houseData.vertices[i]; // Adiciona os novos vértices
        }

        indices.insert(indices.end(), begin(houseData.indices), end(houseData.indices)); // Adiciona os novos índices

        // Definir submesh para o novo objeto
        SubMesh newHouseSubMesh;
        newHouseSubMesh.indexCount = uint(houseData.indices.size());
        newHouseSubMesh.startIndex = totalIndexCount;
        newHouseSubMesh.baseVertex = totalVertexCount;

        Object obj;
        XMStoreFloat4x4(&obj.world, XMMatrixScaling(0.5f, 0.5f, 0.5f)); // Escala do objeto
        obj.cbIndex = cbIndex;
        obj.submesh = newHouseSubMesh;
        scene.push_back(obj); // Adiciona à cena

        cbIndex++;

        totalVertexCount = newTotalVertexCount;
        totalIndexCount = newTotalIndexCount;

        vertices = aux;

        // Atualiza buffers de vértices e índices
        mesh->VertexBuffer(vertices.data(), vbSize, sizeof(Vertex));
        mesh->IndexBuffer(indices.data(), ibSize, DXGI_FORMAT_R32_UINT);
        mesh->ConstantBuffer(sizeof(ObjectConstants), uint(scene.size()));

        graphics->SubmitCommands();
    }
   
    else if (input->KeyPress(VK_TAB)) {
        OutputDebugString("Tab pressionado\n");

        // Alterna o índice do objeto selecionado
        selecionado = (selecionado + 1) % scene.size();
        OutputDebugString(("Objeto selecionado: " + std::to_string(selecionado) + "\n").c_str());

        graphics->ResetCommands();

        bool verticesAlterados = false;

        // Itera sobre os objetos na cena
        for (auto& obj : scene) {
            // Calcula o número de vértices do submesh atual
            uint numVertices = obj.submesh.indexCount;
            OutputDebugString(("Num vertices: " + std::to_string(numVertices) + "\n").c_str());

            // Itera sobre todos os vértices do submesh do objeto selecionado
            if (obj.cbIndex == selecionado) {
                for (uint i = 0; i < numVertices; ++i) {
                    // Verifica os limites de acesso aos vértices
                    if (obj.submesh.baseVertex + i < vertices.size()) {
                        vertices[obj.submesh.baseVertex + i].color = XMFLOAT4(DirectX::Colors::Red);
                        verticesAlterados = true;
                    }
                }
            }
            else {
                for (uint i = 0; i < numVertices; ++i) {
                    // Verifica os limites de acesso aos vértices
                    if (obj.submesh.baseVertex + i < vertices.size()) {
                        vertices[obj.submesh.baseVertex + i].color = XMFLOAT4(DirectX::Colors::DimGray);
                        verticesAlterados = true;
                    }
                }
            }
        }

        // Atualiza os buffers de vértices e índices apenas se houver mudanças
        if (verticesAlterados) {
            mesh->VertexBuffer(vertices.data(), totalVertexCount * sizeof(Vertex), sizeof(Vertex));
            mesh->IndexBuffer(indices.data(), totalIndexCount * sizeof(uint), DXGI_FORMAT_R32_UINT);
        }

        // Submete os comandos para a GPU
        graphics->SubmitCommands();
        }



    else if (input->KeyPress(VK_DELETE)) {
        OutputDebugString("Tecla DEL pressionada\n");

        if (!scene.empty()) {  // Verifica se há objetos na cena
            // Verifica se o objeto selecionado está dentro dos limites
            if (selecionado >= 0 && selecionado < scene.size()) {
                OutputDebugString(("Deletando objeto: " + std::to_string(selecionado) + "\n").c_str());

                // Obter o objeto a ser removido
                const Object& objToDelete = scene[selecionado];

                // Remover os vértices e índices associados ao objeto
                const uint vertexCountToRemove = objToDelete.submesh.indexCount;
                const uint startVertex = objToDelete.submesh.baseVertex;
                const uint indexCountToRemove = objToDelete.submesh.indexCount;
                const uint startIndex = objToDelete.submesh.startIndex;

                // Remover vértices
                vertices.erase(vertices.begin() + startVertex, vertices.begin() + startVertex + vertexCountToRemove);

                // Remover índices
                indices.erase(indices.begin() + startIndex, indices.begin() + startIndex + indexCountToRemove);

                // Atualizar os submeshes dos objetos restantes (ajustando os índices)
                for (uint i = selecionado + 1; i < scene.size(); ++i) {
                    scene[i].submesh.baseVertex -= vertexCountToRemove;
                    scene[i].submesh.startIndex -= indexCountToRemove;
                }

                // Remover o objeto da cena
                scene.erase(scene.begin() + selecionado);

                // Atualizar total de vértices e índices
                totalVertexCount -= vertexCountToRemove;
                totalIndexCount -= indexCountToRemove;

                // Caso não haja mais objetos, o índice selecionado é resetado
                if (scene.empty()) {
                    selecionado = -1;
                }
                else {
                    // Se houver objetos restantes, garantir que o índice de seleção seja válido
                    selecionado = selecionado % scene.size();
                }

                // Atualizar os buffers de vértices e índices na GPU
                const uint vbSize = totalVertexCount * sizeof(Vertex);
                const uint ibSize = totalIndexCount * sizeof(uint);

                graphics->ResetCommands();
                mesh->VertexBuffer(vertices.data(), vbSize, sizeof(Vertex));
                mesh->IndexBuffer(indices.data(), ibSize, DXGI_FORMAT_R32_UINT);
                mesh->ConstantBuffer(sizeof(ObjectConstants), uint(scene.size()));

                graphics->SubmitCommands();

                OutputDebugString("Objeto deletado e buffers atualizados\n");
            }
            else {
                OutputDebugString("Nenhum objeto selecionado para deletar\n");
            }
        }
        else {
            OutputDebugString("Cena vazia, nada para deletar\n");
        }
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
    }
    else {
        // Coloca a câmera acima dos objetos, olhando diretamente para baixo
        XMVECTOR pos = XMVectorSet(0.0f, 10.0f, 0.0f, 1.0f);  // Altura Y=10.0 (ajuste conforme necessário)
        XMVECTOR target = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f); // Centro da cena
        XMVECTOR up = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);    // Vetor "up" apontando para o eixo Z negativo
        XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
        XMStoreFloat4x4(&View, view);
    }
    // carrega matriz de projeção em uma XMMATRIX
    XMMATRIX proj = XMLoadFloat4x4(&Proj);
    if (input->KeyPress('R')) {
        spinning = !spinning; // Alterna o estado de rotação
    }
  
        for (auto& obj : scene) {
            // carrega matriz de mundo em uma XMMATRIX
            XMMATRIX world = XMLoadFloat4x4(&obj.world);

            // constrói matriz combinada (world x view x proj)
            XMMATRIX WorldViewProj = world * XMLoadFloat4x4(&View) * proj;

            // atualiza o buffer constante com a matriz combinada
            ObjectConstants constants;
            XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(WorldViewProj));
            mesh->CopyConstants(&constants, obj.cbIndex);

        }
    

  
    /*
    // ajusta o buffer constante de cada objeto
   */
   
    
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

