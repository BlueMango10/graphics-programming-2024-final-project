#version 330 core

in vec3 WorldPosition;
in vec3 WorldNormal;
in vec2 TexCoord;
in mat3 TBN;
in vec2 TexSquish; // Basically how much the texture is squished due to wave movement

out vec4 FragColor;

// Water
uniform float Time;
uniform float DetailAnimSpeed;
uniform float DetailScale;

// Terrain info
uniform sampler2D Heightmap;
uniform vec4 HeightmapBounds; // xy = min coord, zw = max coord

// surface
uniform sampler2D NormalMap;
uniform sampler2D FoamTexture;
uniform samplerCube SkyboxTexture;
uniform float FresnelBias;
uniform float FresnelScale;
uniform float FresnelPower;
uniform vec4 Color;

// light
uniform vec3 AmbientColor;
uniform vec3 LightColor;
uniform vec3 LightDirection;
uniform vec3 CameraPosition;

// Convert world coordinates to texture coordinates
vec2 worldToTextureCoord(vec2 worldSpacePosition)
{
	return (worldSpacePosition - HeightmapBounds.xy) / (HeightmapBounds.zw - HeightmapBounds.xy);
}

vec3 getNormalFromMap(vec2 tiling, vec2 offset)
{
	vec3 normal = texture(NormalMap, TexCoord * tiling + offset).rgb;
	normal = normal * 2.0 - 1.0;
	normal = normalize(TBN * normal);
	return normal;
}

vec3 getCombinedAnimatedNormal()
{
	float scaledTime = Time * DetailAnimSpeed;
	vec3 normal = getNormalFromMap(vec2(1.0, 1.0) * DetailScale, vec2(scaledTime, scaledTime));
	normal += getNormalFromMap(vec2(1.1, 1.1) * DetailScale, vec2(-scaledTime, -scaledTime));
	normal += getNormalFromMap(vec2(0.4, 0.4) * DetailScale, vec2(scaledTime, -scaledTime));
	normal += getNormalFromMap(vec2(0.5, 0.5) * DetailScale, vec2(-scaledTime, scaledTime));
	return normalize(normal);
}

float fresnel(vec3 incident, vec3 normal, float bias, float scale, float power)
{
	float r = bias + scale * pow(1 + dot(incident, normal), power);
	return clamp(r, 0, 1);
}

void main()
{
	vec3 viewDirection = normalize(CameraPosition - WorldPosition);
	vec3 normal = getCombinedAnimatedNormal();

	vec3 fixedViewDirection = vec3(-viewDirection.x, -viewDirection.y, viewDirection.z); // I'm not sure why the z axis is flipped. It seems correct in all other instances.

	vec3 reflectedViewDirection = reflect(fixedViewDirection, normal);
	vec4 reflectedColor = vec4(texture(SkyboxTexture, reflectedViewDirection).rgb, 1.0);
	
	vec3 refractedViewDirection = refract(fixedViewDirection, normal, 1.0 / 1.33);
	vec4 refractedColor = vec4(texture(SkyboxTexture, refractedViewDirection).rgb, 1.0);
	// Disgusting depth approximation. We actually want the scene depth here!
	refractedColor = mix(Color, refractedColor, texture(Heightmap, worldToTextureCoord(WorldPosition.xz)));
	
	float reflectionCoefficient = fresnel(fixedViewDirection, normal, FresnelBias, FresnelScale, FresnelPower);

	FragColor = mix(refractedColor, reflectedColor, reflectionCoefficient);

	// add foam
	float totalSquish = 1-length(TexSquish);
	vec3 foamColor = vec3(clamp(dot(normalize(LightDirection), normalize(normal)), 0.0, 1.0)) * LightColor + AmbientColor;
	float foamyness = totalSquish - 0.75;
	FragColor = mix(FragColor, vec4(foamColor, 1.0), clamp(foamyness * 10 * texture(FoamTexture, TexCoord).r, 0.0, 1.0));
}
