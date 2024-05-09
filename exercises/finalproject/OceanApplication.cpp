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

OceanApplication::OceanApplication()
	: Application(1024, 1024, "Ocean demo")
	, m_gridX(128), m_gridY(128)
	, m_startTime(std::chrono::steady_clock::now())
	// Shader loaders
	, m_vertexShaderLoader(Shader::Type::VertexShader)
	, m_fragmentShaderLoader(Shader::Type::FragmentShader)
	// Camera
	, m_cameraPosition(0, 30, 30)
	, m_cameraTranslationSpeed(20.0f)
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
	, m_oceanColor(glm::vec4(1.0f))
	, m_oceanSpecularExponent(700.0f)
	, m_oceanSpecularReflection(1.0f)
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

	// Build textures and keep them in a list
	InitializeTextures();
	// Build materials and keep them in a list
	InitializeMaterials();
	// Build meshes and keep them in a list
	InitializeMeshes();

	// Initialize Camera
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

	// Clear color and depth
	GetDevice().Clear(true, Color(0.0f, 0.0f, 0.0f, 1.0f), true, 1.0f);

	// Terrain patches
	DrawObject(m_terrainPatch, *m_terrainMaterial, glm::scale(glm::vec3(10.0f)));
	DrawObject(m_terrainPatch, *m_terrainMaterial, glm::translate(glm::vec3(-10.f, 0.0f, 0.0f)) * glm::scale(glm::vec3(10.0f)));
	DrawObject(m_terrainPatch, *m_terrainMaterial, glm::translate(glm::vec3(0.f, 0.0f, -10.0f)) * glm::scale(glm::vec3(10.0f)));
	DrawObject(m_terrainPatch, *m_terrainMaterial, glm::translate(glm::vec3(-10.f, 0.0f, -10.0f)) * glm::scale(glm::vec3(10.0f)));

	// Water patches
	DrawObject(m_terrainPatch, *m_oceanMaterial, glm::scale(glm::vec3(10.0f)));
	DrawObject(m_terrainPatch, *m_oceanMaterial, glm::translate(glm::vec3(-10.f, 0.0f, 0.0f)) * glm::scale(glm::vec3(10.0f)));
	DrawObject(m_terrainPatch, *m_oceanMaterial, glm::translate(glm::vec3(0.f, 0.0f, -10.0f)) * glm::scale(glm::vec3(10.0f)));
	DrawObject(m_terrainPatch, *m_oceanMaterial, glm::translate(glm::vec3(-10.f, 0.0f, -10.0f)) * glm::scale(glm::vec3(10.0f)));

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

	// Load terrain textures
    m_terrainTexture = LoadTexture("textures/dirt.png");
    //m_grassTexture = LoadTexture("textures/grass.jpg");
    //m_rockTexture = LoadTexture("textures/rock.jpg");
    //m_snowTexture = LoadTexture("textures/snow.jpg");
	m_heightmapTexture = LoadTexture("textures/heightmap.png", GL_CLAMP_TO_EDGE);

	//m_heightmapTexture00 = CreateHeightMap(m_gridX, m_gridY, glm::ivec2(0, 0));
	//m_heightmapTexture10 = CreateHeightMap(m_gridX, m_gridY, glm::ivec2(-1, 0));
	//m_heightmapTexture01 = CreateHeightMap(m_gridX, m_gridY, glm::ivec2(0, -1));
	//m_heightmapTexture11 = CreateHeightMap(m_gridX, m_gridY, glm::ivec2(-1, -1));

	// Load water texture here
	m_oceanTexture = LoadTexture("textures/water.png");
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

	// Terrain shader program
	//Shader terrainVS = m_vertexShaderLoader.Load("shaders/terrain.vert");
	//Shader terrainFS = m_fragmentShaderLoader.Load("shaders/terrain.frag");
	//std::shared_ptr<ShaderProgram> terrainShaderProgram = std::make_shared<ShaderProgram>();
	//terrainShaderProgram->Build(terrainVS, terrainFS);

	// Terrain blinn-phong shader program
	Shader terrainBPVS = m_vertexShaderLoader.Load("shaders/blinn-phong-terrain.vert");
	Shader terrainBPFS = m_fragmentShaderLoader.Load("shaders/blinn-phong-terrain.frag");
	std::shared_ptr<ShaderProgram> terrainBPShaderProgram = std::make_shared<ShaderProgram>();
	terrainBPShaderProgram->Build(terrainBPVS, terrainBPFS);

	// Terrain blinn-phong material
	m_terrainMaterial = std::make_shared<Material>(terrainBPShaderProgram);
	m_terrainMaterial->SetUniformValue("Heightmap", m_heightmapTexture);
	m_terrainMaterial->SetUniformValue("Albedo", m_terrainTexture);
	m_terrainMaterial->SetUniformValue("AmbientReflection", 1.0f);
	m_terrainMaterial->SetUniformValue("DiffuseReflection", 1.0f);

	// Terrain materials
	//m_terrainMaterial00 = std::make_shared<Material>(terrainShaderProgram);
	//m_terrainMaterial00->SetUniformValue("Color", glm::vec4(1.0f));
	//m_terrainMaterial00->SetUniformValue("Heightmap", m_heightmapTexture00);
	//m_terrainMaterial00->SetUniformValue("ColorTexture0", m_dirtTexture);
	//m_terrainMaterial00->SetUniformValue("ColorTexture1", m_grassTexture);
	//m_terrainMaterial00->SetUniformValue("ColorTexture2", m_rockTexture);
	//m_terrainMaterial00->SetUniformValue("ColorTexture3", m_snowTexture);
	//m_terrainMaterial00->SetUniformValue("ColorTextureRange01", glm::vec2(-0.2f, 0.0f));
	//m_terrainMaterial00->SetUniformValue("ColorTextureRange12", glm::vec2(0.1f, 0.2f));
	//m_terrainMaterial00->SetUniformValue("ColorTextureRange23", glm::vec2(0.25f, 0.3f));
	//m_terrainMaterial00->SetUniformValue("ColorTextureScale", glm::vec2(0.125f));
	//
	//m_terrainMaterial10 = std::make_shared<Material>(*m_terrainMaterial00);
	//m_terrainMaterial10->SetUniformValue("Heightmap", m_heightmapTexture10);
	//
	//m_terrainMaterial01 = std::make_shared<Material>(*m_terrainMaterial00);
	//m_terrainMaterial01->SetUniformValue("Heightmap", m_heightmapTexture01);
	//
	//m_terrainMaterial11 = std::make_shared<Material>(*m_terrainMaterial00);
	//m_terrainMaterial11->SetUniformValue("Heightmap", m_heightmapTexture11);
	
	// Water shader
	Shader waterVS = m_vertexShaderLoader.Load("shaders/ocean.vert");
	Shader waterFS = m_fragmentShaderLoader.Load("shaders/blinn-phong-terrain.frag");
	std::shared_ptr<ShaderProgram> waterShaderProgram = std::make_shared<ShaderProgram>();
	waterShaderProgram->Build(waterVS, waterFS);
	
	// Water material
	m_oceanMaterial = std::make_shared<Material>(waterShaderProgram);
	m_oceanMaterial->SetUniformValue("Heightmap", m_heightmapTexture);
	m_oceanMaterial->SetUniformValue("Albedo", m_oceanTexture);
	m_oceanMaterial->SetUniformValue("AmbientReflection", 1.0f);
	m_oceanMaterial->SetUniformValue("DiffuseReflection", 1.0f);
	m_oceanMaterial->SetBlendEquation(Material::BlendEquation::Add);
	m_oceanMaterial->SetBlendParams(Material::BlendParam::SourceAlpha, Material::BlendParam::OneMinusSourceAlpha);
	
	// Initial call to UpdateUniforms to initialize the uniform values
	UpdateUniforms();
}

void OceanApplication::InitializeMeshes()
{
	CreateTerrainMesh(m_terrainPatch, m_gridX, m_gridY);
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
	m_oceanMaterial->SetUniformValue("HeightmapBounds", m_terrainBounds);
	m_oceanMaterial->SetUniformValue("HeightScale", m_terrainHeightScale);
	m_oceanMaterial->SetUniformValue("HeightOffset", m_terrainHeightOffset);
	m_oceanMaterial->SetUniformValue("NormalSampleOffset", m_terrainSampleOffset);
	
	// fragment
	m_oceanMaterial->SetUniformValue("Color", m_oceanColor);
	m_oceanMaterial->SetUniformValue("SpecularExponent", m_oceanSpecularExponent);
	m_oceanMaterial->SetUniformValue("SpecularReflection", m_oceanSpecularReflection);
	
	m_oceanMaterial->SetUniformValue("AmbientColor", m_lightAmbientColor);
	m_oceanMaterial->SetUniformValue("LightColor", m_lightColor * m_lightIntensity);
	m_oceanMaterial->SetUniformValue("LightDirection", m_lightPosition);
	
	m_oceanMaterial->SetUniformValue("CameraPosition", m_cameraPosition);
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

std::shared_ptr<Texture2DObject> OceanApplication::LoadTexture(const char* path, GLenum wrapMode)
{
	std::shared_ptr<Texture2DObject> texture = std::make_shared<Texture2DObject>();
	int width = 0;
	int height = 0;
	int components = 0;

	// Load the texture data here
	unsigned char* data = stbi_load(path, &width, &height, &components, 4);

	texture->Bind();
	texture->SetImage(0, width, height, TextureObject::FormatRGBA, TextureObject::InternalFormatRGBA, std::span<const unsigned char>(data, width * height * 4));
	texture->SetParameter(TextureObject::ParameterEnum::WrapS, wrapMode);
	texture->SetParameter(TextureObject::ParameterEnum::WrapT, wrapMode);
	texture->SetParameter(TextureObject::ParameterEnum::MagFilter, GL_LINEAR);
	texture->SetParameter(TextureObject::ParameterEnum::MinFilter, GL_LINEAR);

	// Generate mipmaps
	texture->GenerateMipmap();

	// Release texture data
	stbi_image_free(data);

	return texture;
}

std::shared_ptr<Texture2DObject> OceanApplication::CreateHeightMap(unsigned int width, unsigned int height, glm::ivec2 coords)
{
	std::shared_ptr<Texture2DObject> heightmap = std::make_shared<Texture2DObject>();
	
	std::vector<float> pixels(height * width);
	for (unsigned int j = 0; j < height; ++j)
	{
		for (unsigned int i = 0; i < width; ++i)
		{
			float x = static_cast<float>(i) / (width - 1) + coords.x;
			float y = static_cast<float>(j) / (height - 1) + coords.y;
			pixels[j * width + i] = stb_perlin_fbm_noise3(x, y, 0.0f, 1.9f, 0.5f, 8) * 0.5f;
		}
	}

	heightmap->Bind();
	heightmap->SetImage<float>(0, width, height, TextureObject::FormatR, TextureObject::InternalFormatR16F, pixels);
	heightmap->GenerateMipmap();

	return heightmap;
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

	bool open = true;
	bool closed = false;

	// Camera
	ImGui::Begin("Camera", &open, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::DragFloat("Translation Speed", &m_cameraTranslationSpeed);
	ImGui::DragFloat("Rotation Speed", &m_cameraRotationSpeed);
	ImGui::Separator();
	ImGui::Text(m_cameraEnabled
		? "Press SPACE to disable camera movement\nUp: Q, Down: E\nLeft: A, Right: D\nForwards: W, Backwards: S\nRotate: Mouse"
		: "Press SPACE to enable camera movement");
	ImGui::End();

	// Terrain
	ImGui::Begin("Terrain", &open, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::DragFloat4("Bounds", &m_terrainBounds[0], 0.1f);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("x: min x coord\ny: min y coord\nz: max x coord\nw: max z coord");
	ImGui::DragFloat("Height Scale", &m_terrainHeightScale, 0.1f);
	ImGui::DragFloat("Height Offset", &m_terrainHeightOffset, 0.1f);
	ImGui::DragFloat("Normal Sample Offset", &m_terrainSampleOffset, 0.01f);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("If this is too big, the normals become inaccurate. If it is too small, it becomes glitchy.");
	ImGui::Separator();
	ImGui::ColorEdit3("Color", &m_terrainColor[0]);
	ImGui::DragFloat("Specular Exponent", &m_terrainSpecularExponent, 1.0f, 0.0f, 1000.0f);
	ImGui::DragFloat("Specular Reflection", &m_terrainSpecularReflection, 0.1f, 0.0f, 1.0f);
	ImGui::End();

	// Ocean
	ImGui::Begin("Ocean", &open, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::ColorEdit4("Color", &m_oceanColor[0]);
	ImGui::DragFloat("Specular Exponent", &m_oceanSpecularExponent, 1.0f, 0.0f, 1000.0f);
	ImGui::DragFloat("Specular Reflection", &m_oceanSpecularReflection, 0.1f, 0.0f, 1.0f);
	ImGui::End();

	// Light
	ImGui::Begin("Light", &open, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::ColorEdit3("Ambient Light Color", &m_lightAmbientColor[0]);
	ImGui::Separator();
	ImGui::DragFloat3("Light Direction", &m_lightPosition[0], 0.1f);
	ImGui::ColorEdit3("Light Color", &m_lightColor[0]);
	ImGui::DragFloat("Light Intensity", &m_lightIntensity, 0.05f, 0.0f, 100.0f);
	ImGui::End();

	m_imGui.EndFrame();
}
