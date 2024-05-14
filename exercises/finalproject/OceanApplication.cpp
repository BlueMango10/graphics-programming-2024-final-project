#include "OceanApplication.h"

#include <ituGL/geometry/VertexFormat.h>
#include <ituGL/texture/Texture2DObject.h>

#include <glm/gtx/transform.hpp> // for matrix transformations

#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>

#define STP_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cmath>
#include <iostream>
#include <numbers> // for PI constant
#include <imgui.h>
#include <chrono>
#include <ituGL/asset/Texture2DLoader.h>
#include <ituGL/asset/TextureCubemapLoader.h>

OceanApplication::OceanApplication()
	: Application(1024, 1024, "Ocean demo")
	, m_gridX(128), m_gridY(128)
	, m_startTime(std::chrono::steady_clock::now())
	// Shader loaders
	, m_vertexShaderLoader(Shader::Type::VertexShader)
	, m_fragmentShaderLoader(Shader::Type::FragmentShader)
	// Camera
	, m_cameraPosition(10, 15, 20)
	, m_cameraTranslationSpeed(5.0f)
	, m_cameraRotationSpeed(0.5f)
	, m_cameraEnabled(false)
	, m_cameraEnablePressed(false)
	, m_mousePosition(GetMainWindow().GetMousePosition(true))
	// Adjustable values
	// Terrain
	, m_terrainBounds(glm::vec4(-10.0f, -10.0f, 10.0f, 10.0f))
	, m_terrainHeightScale(1.5f)
	, m_terrainHeightOffset(-0.7f)
	, m_terrainSampleOffset(0.2f)
	, m_terrainColor(glm::vec4(1.0f))
	, m_terrainSpecularExponent(10.0f)
	, m_terrainSpecularReflection(0.1f)
	// Ocean
	, m_oceanWaveFrequency(glm::vec4(0.38f, 0.49f, 2.38f, 1.71f)) // I have found these values to work well by experimentation
	, m_oceanWaveSpeed(glm::vec4(1.21f, 1.42f, 1.05f, 0.61f))
	, m_oceanWaveWidth(glm::vec4(0.41f, 0.92f, 0.19f, 0.07f))
	, m_oceanWaveHeight(glm::vec4(0.40f, 0.24f, 0.03f, 0.08f))
	, m_oceanWaveDirection(glm::vec4(2.52f, 3.89f, 3.54f, 2.68f))
	, m_oceanCoastOffset(0.0f)
	, m_oceanCoastExponent(1.0f)
	, m_oceanWaveScale(1.0f)
	, m_oceanFresnelBias(0.0f)
	, m_oceanFresnelScale(1.0f)
	, m_oceanFresnelPower(1.0f)
	, m_oceanDetailAnimSpeed(0.07f)
	, m_oceanDetailScale(2.0f)
	, m_oceanColorShallow(glm::vec4(0.0f, 0.4f, 0.2f, 1.0f))
	, m_oceanColor(glm::vec4(0.0f, 0.05f, 0.025f, 1.0f))
	, m_oceanMurkiness(2.5f)
	, m_oceanFakeRefraction(0.5f)
	// Light
	, m_lightAmbientColor(glm::vec3(0.10f, 0.10f, 0.12f))
	, m_lightColor(1.0f)
	, m_lightIntensity(1.0f)
	, m_lightPosition(glm::vec3(-2.5f, 4.0f, -5.0f))
{
}

void OceanApplication::Initialize()
{
	Application::Initialize();

	// Initialize DearImGUI
	m_imGui.Initialize(GetMainWindow());

	// Initialize scene content
	InitializeTextures();
	InitializeMaterials();
	InitializeMeshes();

	// Initialize camera
	InitializeCamera();

	// Enable depth test
	GetDevice().EnableFeature(GL_DEPTH_TEST);
	GetDevice().EnableFeature(GL_CULL_FACE);

	// Enable wireframe
	//GetDevice().SetWireframeEnabled(true);
}

void OceanApplication::Update()
{
	Application::Update();

	UpdateCamera();

	UpdateUniforms();
}

void OceanApplication::Render()
{
	Application::Render();
	
	// Before water pass
	m_fbBeforeWater->Bind();
	// clear color and depth
	GetDevice().Clear(true, Color(0.0f, 0.0f, 0.0f, 1.0f), true, 1.0f);
	// draw terrain and skybox
	DrawTerrain();
	DrawSkybox();

	// Main pass
	FramebufferObject::Unbind();
	// clear color and depth
	GetDevice().Clear(true, Color(0.0f, 0.0f, 0.0f, 1.0f), true, 1.0f);
	// draw terrain and skybox ( again :( )
	DrawTerrain();
	DrawSkybox();
	// draw ocean
	DrawOcean();

	// Render the debug user interface
	RenderGUI();
}

void OceanApplication::Cleanup()
{
	// Cleanup DearImGUI
	m_imGui.Cleanup();

	Application::Cleanup();
}

void OceanApplication::InitializeTextures()
{
	m_defaultTexture = CreateDefaultTexture();

	m_skyboxTexture[0] = TextureCubemapLoader::LoadTextureShared("textures/skybox0.png", TextureObject::FormatRGB, TextureObject::InternalFormatRGB);
	m_skyboxTexture[1] = TextureCubemapLoader::LoadTextureShared("textures/skybox1.png", TextureObject::FormatRGB, TextureObject::InternalFormatRGB);
	m_skyboxTexture[2] = TextureCubemapLoader::LoadTextureShared("textures/skybox2.png", TextureObject::FormatRGB, TextureObject::InternalFormatRGB);
	m_skyboxTexture[3] = TextureCubemapLoader::LoadTextureShared("textures/skybox3.png", TextureObject::FormatRGB, TextureObject::InternalFormatRGB);

	// Terrain
    m_terrainTexture = Load2DTexture("textures/dirt.png", TextureObject::FormatRGB, TextureObject::InternalFormatRGB, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR);

	m_heightmapTexture[0] = Load2DTexture("textures/heightmap0.png", TextureObject::FormatRGBA, TextureObject::InternalFormatRGBA, GL_CLAMP_TO_EDGE, GL_LINEAR); // heightmaps only really need R, but the texture files are RGBA, so we just have to roll with it
	m_heightmapTexture[1] = Load2DTexture("textures/heightmap1.png", TextureObject::FormatRGBA, TextureObject::InternalFormatRGBA, GL_CLAMP_TO_EDGE, GL_LINEAR); // no terrain (for debugging)
	m_heightmapTexture[2] = Load2DTexture("textures/heightmap2.png", TextureObject::FormatRGBA, TextureObject::InternalFormatRGBA, GL_CLAMP_TO_EDGE, GL_LINEAR);

	// Ocean
	m_oceanTexture = Load2DTexture("textures/water_n.png", TextureObject::FormatRGB, TextureObject::InternalFormatRGB, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR); // too much detail disappears when using mip maps
	m_foamTexture = Load2DTexture("textures/foam.png", TextureObject::FormatRGB, TextureObject::InternalFormatRGB, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR);

	// Frame buffers
	Window& window = GetMainWindow();
	int width, height;
	window.GetDimensions(width, height);

	m_fbBeforeWaterColor = std::make_shared<Texture2DObject>();
	m_fbBeforeWaterColor->Bind();
	m_fbBeforeWaterColor->SetImage(0, width, height, TextureObject::FormatRGB, TextureObject::InternalFormatRGB);
	m_fbBeforeWaterColor->SetParameter(TextureObject::ParameterEnum::MinFilter, GL_NEAREST);
	m_fbBeforeWaterColor->SetParameter(TextureObject::ParameterEnum::MagFilter, GL_NEAREST);
	m_fbBeforeWaterColor->GenerateMipmap();
	
	m_fbBeforeWaterDepth = std::make_shared<Texture2DObject>();
	m_fbBeforeWaterDepth->Bind();
	m_fbBeforeWaterDepth->SetImage(0, width, height, TextureObject::FormatDepth, TextureObject::InternalFormatDepth);
	m_fbBeforeWaterDepth->SetParameter(TextureObject::ParameterEnum::MinFilter, GL_NEAREST);
	m_fbBeforeWaterDepth->SetParameter(TextureObject::ParameterEnum::MagFilter, GL_NEAREST);
	m_fbBeforeWaterDepth->GenerateMipmap();

	Texture2DObject::Unbind();

	m_fbBeforeWater = std::make_shared<FramebufferObject>();
	m_fbBeforeWater->Bind();
	m_fbBeforeWater->SetTexture(FramebufferObject::Target::Both, FramebufferObject::Attachment::Color0, *m_fbBeforeWaterColor);
	m_fbBeforeWater->SetTexture(FramebufferObject::Target::Both, FramebufferObject::Attachment::Depth, *m_fbBeforeWaterDepth);
	m_fbBeforeWater->SetDrawBuffers(std::array<FramebufferObject::Attachment, 1>(
		{
			FramebufferObject::Attachment::Color0
		}));
	FramebufferObject::Unbind();
}

void OceanApplication::InitializeMaterials()
{
	// Default shader program
	Shader defaultVS = m_vertexShaderLoader.Load("shaders/default.vert");
	Shader defaultFS = m_fragmentShaderLoader.Load("shaders/default.frag");
	std::shared_ptr<ShaderProgram> defaultShaderProgram = std::make_shared<ShaderProgram>();
	defaultShaderProgram->Build(defaultVS, defaultFS);

	// Default material
	m_defaultMaterial = std::make_shared<Material>(defaultShaderProgram);
	m_defaultMaterial->SetUniformValue("Color", glm::vec4(1.0f));

	// Skybox shader (the shader used here comes from exercise 8)
	Shader skyboxVS = m_vertexShaderLoader.Load("shaders/skybox.vert");
	Shader skyboxFS = m_fragmentShaderLoader.Load("shaders/skybox.frag");
	std::shared_ptr<ShaderProgram> skyboxShaderProgram = std::make_shared<ShaderProgram>();
	skyboxShaderProgram->Build(skyboxVS, skyboxFS);

	// Skybox material
	m_skyboxMaterial = std::make_shared<Material>(skyboxShaderProgram);
	// (skybox is set in ApplySkybox)

	// Terrain shader program
	Shader terrainVS = m_vertexShaderLoader.Load("shaders/blinn-phong-terrain.vert");
	Shader terrainFS = m_fragmentShaderLoader.Load("shaders/blinn-phong-terrain.frag");
	std::shared_ptr<ShaderProgram> terrainShaderProgram = std::make_shared<ShaderProgram>();
	terrainShaderProgram->Build(terrainVS, terrainFS);

	// Terrain material
	m_terrainMaterial = std::make_shared<Material>(terrainShaderProgram);
	// (heightmap is set in ApplyPreset)
	m_terrainMaterial->SetUniformValue("ColorTexture", m_terrainTexture);
	m_terrainMaterial->SetUniformValue("AmbientReflection", 1.0f);
	m_terrainMaterial->SetUniformValue("DiffuseReflection", 1.0f);
	
	// Ocean shader
	Shader oceanVS = m_vertexShaderLoader.Load("shaders/ocean.vert");
	Shader oceanFS = m_fragmentShaderLoader.Load("shaders/ocean.frag");
	std::shared_ptr<ShaderProgram> oceanShaderProgram = std::make_shared<ShaderProgram>();
	oceanShaderProgram->Build(oceanVS, oceanFS);
	
	// Ocean material
	m_oceanMaterial = std::make_shared<Material>(oceanShaderProgram);
	// (heightmap is set in ApplyPreset)
	m_oceanMaterial->SetUniformValue("NormalMap", m_oceanTexture);
	m_oceanMaterial->SetUniformValue("FoamTexture", m_foamTexture);
	m_oceanMaterial->SetUniformValue("AmbientReflection", 1.0f);
	m_oceanMaterial->SetUniformValue("DiffuseReflection", 1.0f);
	// (skybox is set in ApplySkybox)

	// render buffer stuff for water
	m_oceanMaterial->SetUniformValue("SceneColor", m_fbBeforeWaterColor);
	m_oceanMaterial->SetUniformValue("SceneDepth", m_fbBeforeWaterDepth);

	Window& window = GetMainWindow();
	int width, height;
	window.GetDimensions(width, height);

	m_oceanMaterial->SetUniformValue("Resolution", glm::vec2(width, height));

	// Initial call to ApplyPreset, ApplySkybox and UpdateUniforms to initialize the uniform values
	ApplyPreset(0);
	ApplySkybox(0);
	UpdateUniforms();
}

void OceanApplication::InitializeMeshes()
{
	CreateTerrainMesh(m_terrainPatch, m_gridX, m_gridY);
	CreateFullscreenMesh(m_fullscreenMesh);
}

void OceanApplication::UpdateUniforms()
{
	// Terrain
	
	// vertex
	m_terrainMaterial->SetUniformValue("HeightmapBounds", m_terrainBounds);
	m_terrainMaterial->SetUniformValue("HeightScale", m_terrainHeightScale);
	m_terrainMaterial->SetUniformValue("HeightOffset", m_terrainHeightOffset);
	m_terrainMaterial->SetUniformValue("NormalSampleOffset", m_terrainSampleOffset);
	
	// fragment
	m_terrainMaterial->SetUniformValue("Color", m_terrainColor);
	m_terrainMaterial->SetUniformValue("SpecularExponent", m_terrainSpecularExponent);
	m_terrainMaterial->SetUniformValue("SpecularReflection", m_terrainSpecularReflection);
	
	m_terrainMaterial->SetUniformValue("AmbientColor", m_lightAmbientColor);
	m_terrainMaterial->SetUniformValue("LightColor", m_lightColor * m_lightIntensity);
	m_terrainMaterial->SetUniformValue("LightDirection", m_lightPosition);
	
	m_terrainMaterial->SetUniformValue("CameraPosition", m_cameraPosition);


	// Ocean
	auto currentTime = std::chrono::steady_clock::now();
	auto time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_startTime);
	m_oceanMaterial->SetUniformValue("Time", static_cast<float>(time.count()) / 1000);

	// vertex
	m_oceanMaterial->SetUniformValue("WaveFrequency", m_oceanWaveFrequency);
	m_oceanMaterial->SetUniformValue("WaveSpeed", m_oceanWaveSpeed);
	m_oceanMaterial->SetUniformValue("WaveDirectionX", glm::vec4(
		cos(m_oceanWaveDirection.x),
		cos(m_oceanWaveDirection.y),
		cos(m_oceanWaveDirection.z),
		cos(m_oceanWaveDirection.w)));
	m_oceanMaterial->SetUniformValue("WaveDirectionY", glm::vec4(
		sin(m_oceanWaveDirection.x),
		sin(m_oceanWaveDirection.y),
		sin(m_oceanWaveDirection.z),
		sin(m_oceanWaveDirection.w)));
	m_oceanMaterial->SetUniformValue("WaveHeight", m_oceanWaveHeight);
	m_oceanMaterial->SetUniformValue("WaveWidth", m_oceanWaveWidth);

	m_oceanMaterial->SetUniformValue("HeightmapBounds", m_terrainBounds);
	m_oceanMaterial->SetUniformValue("HeightScale", m_terrainHeightScale);
	m_oceanMaterial->SetUniformValue("HeightOffset", m_terrainHeightOffset);
	m_oceanMaterial->SetUniformValue("CoastOffset", m_oceanCoastOffset);
	m_oceanMaterial->SetUniformValue("CoastExponent", m_oceanCoastExponent);
	m_oceanMaterial->SetUniformValue("WaveScale", m_oceanWaveScale);

	m_oceanMaterial->SetUniformValue("DetailAnimSpeed", m_oceanDetailAnimSpeed);
	m_oceanMaterial->SetUniformValue("DetailScale", m_oceanDetailScale);

	m_oceanMaterial->SetUniformValue("NormalSampleOffset", m_terrainSampleOffset);
	
	// fragment
	m_oceanMaterial->SetUniformValue("ColorShallow", m_oceanColorShallow);
	m_oceanMaterial->SetUniformValue("Color", m_oceanColor);
	m_oceanMaterial->SetUniformValue("Murkiness", m_oceanMurkiness);
	m_oceanMaterial->SetUniformValue("FakeRefraction", m_oceanFakeRefraction);

	m_oceanMaterial->SetUniformValue("FresnelBias", m_oceanFresnelBias);
	m_oceanMaterial->SetUniformValue("FresnelScale", m_oceanFresnelScale);
	m_oceanMaterial->SetUniformValue("FresnelPower", m_oceanFresnelPower);
	
	m_oceanMaterial->SetUniformValue("AmbientColor", m_lightAmbientColor);
	m_oceanMaterial->SetUniformValue("LightColor", m_lightColor * m_lightIntensity);
	m_oceanMaterial->SetUniformValue("LightDirection", m_lightPosition);
	
	m_oceanMaterial->SetUniformValue("CameraPosition", m_cameraPosition);

	m_oceanMaterial->SetUniformValue("NearPlane", 0.1f);
	m_oceanMaterial->SetUniformValue("FarPlane", 1000.0f);
}

void OceanApplication::ApplyPreset(int presetId)
{
	m_terrainMaterial->SetUniformValue("Heightmap", m_heightmapTexture[presetId]);
	m_oceanMaterial->SetUniformValue("Heightmap", m_heightmapTexture[presetId]);
	switch (presetId)
	{
	case 0:
		m_terrainHeightScale = 1.5f;
		m_oceanCoastOffset = 0.2f;
		m_oceanCoastExponent = 1.5f;
		m_oceanWaveScale = 0.5f;
		break;
	case 1:
		m_terrainHeightScale = 1.5f;
		m_oceanCoastOffset = 0.05f;
		m_oceanCoastExponent = 1.0f;
		m_oceanWaveScale = 1.0f;
		break;
	case 2:
		m_terrainHeightScale = 2.0f;
		m_oceanCoastOffset = 0.05f;
		m_oceanCoastExponent = 1.0f;
		m_oceanWaveScale = 1.0f;
		break;
	}
}

void OceanApplication::ApplySkybox(int skyboxId)
{
	m_skyboxMaterial->SetUniformValue("SkyboxTexture", m_skyboxTexture[skyboxId]);
	m_oceanMaterial->SetUniformValue("SkyboxTexture", m_skyboxTexture[skyboxId]);
}

std::shared_ptr<Texture2DObject> OceanApplication::CreateDefaultTexture()
{
	std::shared_ptr<Texture2DObject> texture = std::make_shared<Texture2DObject>();

	int width = 4;
	int height = 4;
	std::vector<float> pixels;
	for (int j = 0; j < height; ++j)
	{
		for (int i = 0; i < width; ++i)
		{
			pixels.push_back(1.0f);
			pixels.push_back(0.0f);
			pixels.push_back(1.0f);
			pixels.push_back(1.0f);
		}
	}

	texture->Bind();
	texture->SetImage<float>(0, width, height, TextureObject::FormatRGBA, TextureObject::InternalFormatRGBA, pixels);
	texture->GenerateMipmap();

	return texture;
}

std::shared_ptr<Texture2DObject> OceanApplication::Load2DTexture(const char* path, TextureObject::Format format, TextureObject::InternalFormat internalFormat, GLenum wrapMode, GLenum filter)
{
	// I want to set some extra properties appart from what Texture2DLoader does which is why this function exists.
	std::shared_ptr<Texture2DObject> texture = Texture2DLoader::LoadTextureShared(path, format, internalFormat);
	
	texture->Bind();
	
	texture->SetParameter(TextureObject::ParameterEnum::WrapS, wrapMode);
	texture->SetParameter(TextureObject::ParameterEnum::WrapT, wrapMode);
	
	texture->SetParameter(TextureObject::ParameterEnum::MagFilter, filter);
	texture->SetParameter(TextureObject::ParameterEnum::MinFilter, filter);
	
	Texture2DObject::Unbind();

	return texture;
}

void OceanApplication::DrawObject(const Mesh& mesh, Material& material, const glm::mat4& worldMatrix)
{
	material.Use();

	ShaderProgram& shaderProgram = *material.GetShaderProgram();
	ShaderProgram::Location locationWorldMatrix = shaderProgram.GetUniformLocation("WorldMatrix");
	material.GetShaderProgram()->SetUniform(locationWorldMatrix, worldMatrix);
	ShaderProgram::Location locationViewProjMatrix = shaderProgram.GetUniformLocation("ViewProjMatrix");
	material.GetShaderProgram()->SetUniform(locationViewProjMatrix, m_camera.GetViewProjectionMatrix());

	mesh.DrawSubmesh(0);
}

void OceanApplication::DrawTerrain()
{
	DrawObject(m_terrainPatch, *m_terrainMaterial, glm::scale(glm::vec3(10.0f)));
	DrawObject(m_terrainPatch, *m_terrainMaterial, glm::translate(glm::vec3(-10.f, 0.0f, 0.0f)) * glm::scale(glm::vec3(10.0f)));
	DrawObject(m_terrainPatch, *m_terrainMaterial, glm::translate(glm::vec3(0.f, 0.0f, -10.0f)) * glm::scale(glm::vec3(10.0f)));
	DrawObject(m_terrainPatch, *m_terrainMaterial, glm::translate(glm::vec3(-10.f, 0.0f, -10.0f)) * glm::scale(glm::vec3(10.0f)));
}

void OceanApplication::DrawOcean()
{
	DrawObject(m_terrainPatch, *m_oceanMaterial, glm::scale(glm::vec3(10.0f)));
	DrawObject(m_terrainPatch, *m_oceanMaterial, glm::translate(glm::vec3(-10.f, 0.0f, 0.0f)) * glm::scale(glm::vec3(10.0f)));
	DrawObject(m_terrainPatch, *m_oceanMaterial, glm::translate(glm::vec3(0.f, 0.0f, -10.0f)) * glm::scale(glm::vec3(10.0f)));
	DrawObject(m_terrainPatch, *m_oceanMaterial, glm::translate(glm::vec3(-10.f, 0.0f, -10.0f)) * glm::scale(glm::vec3(10.0f)));
}

void OceanApplication::DrawSkybox()
{
	// This is based on the code from SkyboxRenderPass::Render from the ituGL library

	m_skyboxMaterial->Use();

	m_skyboxMaterial->SetUniformValue("CameraPosition", m_camera.ExtractTranslation());
	m_skyboxMaterial->SetUniformValue("InvViewProjMatrix", glm::inverse(m_camera.GetViewProjectionMatrix()));

	glDepthFunc(GL_EQUAL);

	m_fullscreenMesh.DrawSubmesh(0);

	glDepthFunc(GL_LESS);
}

void OceanApplication::CreateTerrainMesh(Mesh& mesh, unsigned int gridX, unsigned int gridY)
{
	// Define the vertex structure
	struct Vertex
	{
		Vertex() = default;
		Vertex(const glm::vec3& position, const glm::vec3& normal, const glm::vec2 texCoord)
			: position(position), normal(normal), texCoord(texCoord) {}
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoord;
	};

	// Define the vertex format (should match the vertex structure)
	VertexFormat vertexFormat;
	vertexFormat.AddVertexAttribute<float>(3);
	vertexFormat.AddVertexAttribute<float>(3);
	vertexFormat.AddVertexAttribute<float>(2);

	// List of vertices (VBO)
	std::vector<Vertex> vertices;

	// List of indices (EBO)
	std::vector<unsigned int> indices;

	// Grid scale to convert the entire grid to size 1x1
	glm::vec2 scale(1.0f / (gridX - 1), 1.0f / (gridY - 1));
	
	// Number of columnts and rows
	unsigned int columnCount = gridX;
	unsigned int rowCount = gridY;

	// Iterate over each VERTEX
	for (unsigned int j = 0; j < rowCount; ++j)
	{
		for (unsigned int i = 0; i < columnCount; ++i)
		{
			// Vertex data for this vertex only
			glm::vec3 position(i* scale.x, 0.0f, j* scale.y);
			glm::vec3 normal(0.0f, 1.0f, 0.0f);
			glm::vec2 texCoord(i, j);
			vertices.emplace_back(position, normal, texCoord);

			// Index data for quad formed by previous vertices and current
			if (i > 0 && j > 0)
			{
				unsigned int top_right = j * columnCount + i; // Current vertex
				unsigned int top_left = top_right - 1;
				unsigned int bottom_right = top_right - columnCount;
				unsigned int bottom_left = bottom_right - 1;

				//Triangle 1
				indices.push_back(bottom_left);
				indices.push_back(top_left);
				indices.push_back(bottom_right);

				//Triangle 2
				indices.push_back(bottom_right);
				indices.push_back(top_left);
				indices.push_back(top_right);
			}
		}
	}

	mesh.AddSubmesh<Vertex, unsigned int, VertexFormat::LayoutIterator>(Drawcall::Primitive::Triangles, vertices, indices,
		vertexFormat.LayoutBegin(static_cast<int>(vertices.size()), true /* interleaved */), vertexFormat.LayoutEnd());
}

void OceanApplication::CreateFullscreenMesh(Mesh& mesh)
{
	VertexFormat vertexFormat;
	vertexFormat.AddVertexAttribute<float>(3, VertexAttribute::Semantic::Position);

	std::vector<glm::vec3> vertices;
	vertices.emplace_back(-1.0f, -1.0f, 0.0f);
	vertices.emplace_back(3.0f, -1.0f, 0.0f);
	vertices.emplace_back(-1.0f, 3.0f, 0.0f);
	mesh.AddSubmesh<glm::vec3, VertexFormat::LayoutIterator>(Drawcall::Primitive::Triangles, vertices, vertexFormat.LayoutBegin(3, false), vertexFormat.LayoutEnd());
}

void OceanApplication::InitializeCamera()
{
	// Set view matrix, from the camera position looking to the origin
	m_camera.SetViewMatrix(m_cameraPosition, glm::vec3(0.0f));

	// Set perspective matrix
	float aspectRatio = GetMainWindow().GetAspectRatio();
	m_camera.SetPerspectiveProjectionMatrix(1.0f, aspectRatio, 0.1f, 1000.0f);
}

void OceanApplication::UpdateCamera()
{
	// This is based on the camera movement from exercise 5 with a few modifications (up/down movement)
	
	Window& window = GetMainWindow();

	// Update if camera is enabled (controlled by SPACE key)
	{
		bool enablePressed = window.IsKeyPressed(GLFW_KEY_SPACE);
		if (enablePressed && !m_cameraEnablePressed)
		{
			m_cameraEnabled = !m_cameraEnabled;

			window.SetMouseVisible(!m_cameraEnabled);
			m_mousePosition = window.GetMousePosition(true);
		}
		m_cameraEnablePressed = enablePressed;
	}

	if (!m_cameraEnabled)
		return;

	glm::mat4 viewTransposedMatrix = glm::transpose(m_camera.GetViewMatrix());
	glm::vec3 viewRight = viewTransposedMatrix[0];
	glm::vec3 viewUp = viewTransposedMatrix[1];
	glm::vec3 viewForward = -viewTransposedMatrix[2];

	// Update camera translation
	{
		glm::vec3 inputTranslation(0.0f);

		if (window.IsKeyPressed(GLFW_KEY_A))
			inputTranslation.x = -1.0f;
		else if (window.IsKeyPressed(GLFW_KEY_D))
			inputTranslation.x = 1.0f;

		if (window.IsKeyPressed(GLFW_KEY_E))
			inputTranslation.y = 1.0f;
		else if (window.IsKeyPressed(GLFW_KEY_Q))
			inputTranslation.y = -1.0f;

		if (window.IsKeyPressed(GLFW_KEY_W))
			inputTranslation.z = 1.0f;
		else if (window.IsKeyPressed(GLFW_KEY_S))
			inputTranslation.z = -1.0f;

		inputTranslation *= m_cameraTranslationSpeed;
		inputTranslation *= GetDeltaTime();

		// Double speed if SHIFT is pressed
		if (window.IsKeyPressed(GLFW_KEY_LEFT_SHIFT))
			inputTranslation *= 2.0f;

		m_cameraPosition += inputTranslation.x * viewRight + inputTranslation.y * viewUp + inputTranslation.z * viewForward;
	}

	// Update camera rotation
	{
		glm::vec2 mousePosition = window.GetMousePosition(true);
		glm::vec2 deltaMousePosition = mousePosition - m_mousePosition;
		m_mousePosition = mousePosition;

		glm::vec3 inputRotation(-deltaMousePosition.x, deltaMousePosition.y, 0.0f);

		inputRotation *= m_cameraRotationSpeed;

		viewForward = glm::rotate(inputRotation.x, glm::vec3(0, 1, 0)) * glm::rotate(inputRotation.y, glm::vec3(viewRight)) * glm::vec4(viewForward, 0);
	}

	// Update view matrix
	m_camera.SetViewMatrix(m_cameraPosition, m_cameraPosition + viewForward);
}

void OceanApplication::RenderGUI()
{
	m_imGui.BeginFrame();

	// Camera
	ImGui::Begin("Camera", NULL, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::DragFloat("Translation Speed", &m_cameraTranslationSpeed);
	ImGui::DragFloat("Rotation Speed", &m_cameraRotationSpeed);
	ImGui::Separator();
	ImGui::Text(m_cameraEnabled
		? "Press SPACE to disable camera movement\nUp: Q, Down: E\nLeft: A, Right: D\nForwards: W, Backwards: S\nRotate: Mouse"
		: "Press SPACE to enable camera movement");
	ImGui::End();

	// Terrain
	ImGui::Begin("Terrain", NULL, ImGuiWindowFlags_AlwaysAutoResize);
	// shape/vertex
	ImGui::DragFloat4("Bounds", &m_terrainBounds[0], 0.1f);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("x: min x coord\ny: min y coord\nz: max x coord\nw: max z coord");
	ImGui::DragFloat("Height Scale", &m_terrainHeightScale, 0.1f);
	ImGui::DragFloat("Height Offset", &m_terrainHeightOffset, 0.1f);
	ImGui::DragFloat("Normal Sample Offset", &m_terrainSampleOffset, 0.01f);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("The sample offset used to approximate normals on the terrain and water. If this is too big, the normals become inaccurate. If it is too small, it becomes glitchy.");
	ImGui::Separator();
	// surface
	ImGui::ColorEdit3("Color", &m_terrainColor[0]);
	ImGui::DragFloat("Specular Exponent", &m_terrainSpecularExponent, 1.0f, 0.0f, 1000.0f);
	ImGui::DragFloat("Specular Reflection", &m_terrainSpecularReflection, 0.1f, 0.0f, 1.0f);
	ImGui::End();

	// Ocean
	ImGui::Begin("Ocean", NULL, ImGuiWindowFlags_AlwaysAutoResize);
	// gerstner waves
	ImGui::Text("Wave 1, Wave 2, Wave 3, Wave 4");
	ImGui::DragFloat4("Wave Frequency", &m_oceanWaveFrequency[0], 0.01f);
	ImGui::DragFloat4("Wave Speed", &m_oceanWaveSpeed[0], 0.01f);
	ImGui::DragFloat4("Wave Width", &m_oceanWaveWidth[0], 0.01f);
	ImGui::DragFloat4("Wave Height", &m_oceanWaveHeight[0], 0.01f);
	ImGui::DragFloat4("Wave Direction", &m_oceanWaveDirection[0], 0.01f);
	ImGui::Separator();
	// general vertex settings
	ImGui::DragFloat("Coast Offset", &m_oceanCoastOffset, 0.01f);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("A small offset applied to the terrain height to control how close to the shore the water waves disappear.");
	ImGui::DragFloat("Coast Exponent", &m_oceanCoastExponent, 0.01f);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Applied to the depth when evaulating wave height to ease the transition from shallow to deep ocean.");
	ImGui::DragFloat("Wave Scale", &m_oceanWaveScale, 0.01f);
	ImGui::Separator();
	// surface
	ImGui::ColorEdit4("Color Shallow", &m_oceanColorShallow[0]);
	ImGui::ColorEdit4("Color", &m_oceanColor[0]);
	ImGui::DragFloat("Color Murkiness", &m_oceanMurkiness, 0.01f);
	ImGui::DragFloat("Fake Refraction", &m_oceanFakeRefraction, 0.01f);
	ImGui::Separator();
	ImGui::DragFloat("Fresnel Bias", &m_oceanFresnelBias, 0.01f);
	ImGui::DragFloat("Fresnel Scale", &m_oceanFresnelScale, 0.01f);
	ImGui::DragFloat("Fresnel Power", &m_oceanFresnelPower, 0.01f);
	ImGui::Separator();
	ImGui::DragFloat("Detail Anim Speed", &m_oceanDetailAnimSpeed, 0.01f);
	ImGui::DragFloat("Detail Scale", &m_oceanDetailScale, 0.01f);
	ImGui::End();

	// Light
	ImGui::Begin("Light", NULL, ImGuiWindowFlags_AlwaysAutoResize);
	// ambient light
	ImGui::ColorEdit3("Ambient Light Color", &m_lightAmbientColor[0]);
	ImGui::Separator();
	// directional light
	ImGui::DragFloat3("Light Direction", &m_lightPosition[0], 0.1f);
	ImGui::ColorEdit3("Light Color", &m_lightColor[0]);
	ImGui::DragFloat("Light Intensity", &m_lightIntensity, 0.05f, 0.0f, 100.0f);
	ImGui::End();

	// Scene
	ImGui::Begin("Scene", NULL, ImGuiWindowFlags_AlwaysAutoResize);
	// terrain
	ImGui::Text("Terrain:");
	ImGui::SameLine();
	if (ImGui::Button("*Default")) ApplyPreset(0);
	ImGui::SameLine();
	if (ImGui::Button("NoTerrain")) ApplyPreset(1);
	ImGui::SameLine();
	if (ImGui::Button("Islands")) ApplyPreset(2);
	// skybox
	ImGui::Text("Skybox:");
	ImGui::SameLine();
	if (ImGui::Button("*Dark")) ApplySkybox(0);
	ImGui::SameLine();
	if (ImGui::Button("Cloudy")) ApplySkybox(1);
	ImGui::SameLine();
	if (ImGui::Button("Overcast")) ApplySkybox(2);
	ImGui::SameLine();
	if (ImGui::Button("Light")) ApplySkybox(3);
	ImGui::End();

	m_imGui.EndFrame();
}
