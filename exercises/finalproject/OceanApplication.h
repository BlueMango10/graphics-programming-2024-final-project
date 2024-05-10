#pragma once

#include <ituGL/application/Application.h>

#include <ituGL/asset/ShaderLoader.h>
#include <ituGL/geometry/Mesh.h>
#include <ituGL/camera/Camera.h>
#include <ituGL/shader/Material.h>
#include <ituGL/utils/DearImGui.h>
#include <glm/mat4x4.hpp>
#include <vector>
#include <chrono>

class Texture2DObject;

class OceanApplication : public Application
{
public:
    OceanApplication();

protected:
    void Initialize() override;
    void Update() override;
    void Render() override;
    void Cleanup() override;

private:
    void InitializeTextures();
    void InitializeMaterials();
    void InitializeMeshes();
    void InitializeCamera();

    void UpdateCamera();
    void UpdateUniforms();

    void RenderGUI();

    void DrawObject(const Mesh& mesh, Material& material, const glm::mat4& worldMatrix);

    std::shared_ptr<Texture2DObject> CreateDefaultTexture();
    std::shared_ptr<Texture2DObject> CreateHeightMap(unsigned int width, unsigned int height, glm::ivec2 coords);
    std::shared_ptr<Texture2DObject> LoadTexture(const char* path, GLenum wrapMode = GL_REPEAT);

    void CreateTerrainMesh(Mesh& mesh, unsigned int gridX, unsigned int gridY);

private:
    unsigned int m_gridX, m_gridY;
    std::chrono::steady_clock::time_point m_startTime;

    // Camera
    Camera m_camera;
    glm::vec3 m_cameraPosition;
    float m_cameraTranslationSpeed;
    float m_cameraRotationSpeed;
    bool m_cameraEnabled;
    bool m_cameraEnablePressed;
    glm::vec2 m_mousePosition;

    // Shader loaders
    ShaderLoader m_vertexShaderLoader;
    ShaderLoader m_fragmentShaderLoader;

    // Meshes
    Mesh m_terrainPatch;

    // Materials
    std::shared_ptr<Material> m_defaultMaterial;
    //std::shared_ptr<Material> m_terrainMaterial00;
    //std::shared_ptr<Material> m_terrainMaterial10;
    //std::shared_ptr<Material> m_terrainMaterial01;
    //std::shared_ptr<Material> m_terrainMaterial11;
    std::shared_ptr<Material> m_terrainMaterial;
    std::shared_ptr<Material> m_oceanMaterial;

    // Textures
    std::shared_ptr<Texture2DObject> m_defaultTexture;
    //std::shared_ptr<Texture2DObject> m_heightmapTexture00;
    //std::shared_ptr<Texture2DObject> m_heightmapTexture10;
    //std::shared_ptr<Texture2DObject> m_heightmapTexture01;
    //std::shared_ptr<Texture2DObject> m_heightmapTexture11;
    std::shared_ptr<Texture2DObject> m_terrainTexture;
    //std::shared_ptr<Texture2DObject> m_grassTexture;
    //std::shared_ptr<Texture2DObject> m_rockTexture;
    //std::shared_ptr<Texture2DObject> m_snowTexture;
    std::shared_ptr<Texture2DObject> m_oceanTexture;
    std::shared_ptr<Texture2DObject> m_heightmapTexture;


    // GUI and misc adjustable parameters
    DearImGui m_imGui;

    // Terrain
    // vertex
    glm::vec4 m_terrainBounds;
    float m_terrainHeightScale;
    float m_terrainHeightOffset;
    float m_terrainSampleOffset;
    // fragment
    glm::vec4 m_terrainColor;
    float m_terrainSpecularReflection;
    float m_terrainSpecularExponent;

    // Water
    // vertex
    float m_oceanWaveFrequency;
    float m_oceanWaveSpeed;
    float m_oceanWaveWidth;
    float m_oceanWaveHeight;
    float m_oceanCoastOffset;
    // fragment
    glm::vec4 m_oceanColor;
    float m_oceanSpecularReflection;
    float m_oceanSpecularExponent;
    
    // Light
    glm::vec3 m_lightAmbientColor;
    glm::vec3 m_lightColor;
    glm::vec3 m_lightPosition;
    float m_lightIntensity;
};
