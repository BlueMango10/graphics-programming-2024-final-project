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
#include <ituGL/texture/TextureCubemapObject.h>

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
    // Update all uniform values that should update every frame (i.e. configurable in debug UI)
    void UpdateUniforms();
    // Update configurable values and change terrain
    void ApplyPreset(int presetId);

    void RenderGUI();

    void DrawObject(const Mesh& mesh, Material& material, const glm::mat4& worldMatrix);
    void DrawSkybox();

    std::shared_ptr<Texture2DObject> CreateDefaultTexture();
    std::shared_ptr<Texture2DObject> CreateHeightMap(unsigned int width, unsigned int height, glm::ivec2 coords);
    std::shared_ptr<Texture2DObject> Load2DTexture(const char* path, TextureObject::Format format, TextureObject::InternalFormat internalFormat, GLenum wrapMode, GLenum filter);

    void CreateTerrainMesh(Mesh& mesh, unsigned int gridX, unsigned int gridY);
    void CreateFullscreenMesh(Mesh& mesh);

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
    Mesh m_fullscreenMesh;

    // Materials
    std::shared_ptr<Material> m_defaultMaterial;
    std::shared_ptr<Material> m_terrainMaterial;
    std::shared_ptr<Material> m_oceanMaterial;
    std::shared_ptr<Material> m_skyboxMaterial;

    // Textures
    std::shared_ptr<Texture2DObject> m_defaultTexture;
    std::shared_ptr<Texture2DObject> m_terrainTexture;
    std::shared_ptr<Texture2DObject> m_oceanTexture;
    std::shared_ptr<Texture2DObject> m_foamTexture;
    std::shared_ptr<Texture2DObject> m_heightmapTexture[3];
    std::shared_ptr<TextureCubemapObject> m_skyboxTexture;


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
    glm::vec4 m_oceanWaveFrequency;
    glm::vec4 m_oceanWaveSpeed;
    glm::vec4 m_oceanWaveWidth;
    glm::vec4 m_oceanWaveHeight;
    glm::vec4 m_oceanWaveDirection;
    float m_oceanCoastOffset;
    float m_oceanCoastExponent;
    float m_oceanWaveScale;
    // fragment
    float m_oceanDetailAnimSpeed;
    float m_oceanDetailScale;
    float m_oceanFresnelBias;
    float m_oceanFresnelScale;
    float m_oceanFresnelPower;
    glm::vec4 m_oceanColor;
    
    // Light
    glm::vec3 m_lightAmbientColor;
    glm::vec3 m_lightColor;
    glm::vec3 m_lightPosition;
    float m_lightIntensity;
};
