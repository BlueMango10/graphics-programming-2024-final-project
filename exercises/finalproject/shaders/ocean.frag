#version 330 core

in vec3 WorldPosition;
in vec3 WorldNormal;
in vec2 TexCoord;
in mat3 TBN;

out vec4 FragColor;

// Water
uniform float Time;
uniform float DetailAnimSpeed;
uniform float DetailScale;

// Terrain info
uniform sampler2D Heightmap;
uniform vec4 HeightmapBounds; // xy = min coord, zw = max coord

// surface
uniform vec4 Color;
uniform float AmbientReflection;
uniform float DiffuseReflection;
uniform float SpecularReflection;
uniform float SpecularExponent;
uniform sampler2D NormalMap;

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

vec3 ambient(vec3 color)
{
	return AmbientColor * AmbientReflection * color;
}

vec3 diffuse(vec3 color, vec3 lightDirection, vec3 normal)
{
	return LightColor * DiffuseReflection * color * max(dot(lightDirection, normal), 0.0);
}

vec3 specular(vec3 lightDirection, vec3 viewDirection, vec3 normal)
{
	vec3 halfVector = normalize(lightDirection + viewDirection);
	return LightColor * SpecularReflection * pow(max(dot(halfVector, normal), 0.0), SpecularExponent);
}

vec3 blinnPhong(vec3 color, vec3 lightDirection, vec3 viewDirection, vec3 normal)
{
	return ambient(color) + diffuse(color, lightDirection, normal) + specular(lightDirection, viewDirection, normal);
}

void main()
{
	//vec3 color = Color.rgb * texture(NormalMap, TexCoord).rgb;
	vec3 color = Color.rgb;
	vec3 viewDirection = normalize(CameraPosition - WorldPosition);

	FragColor = vec4(blinnPhong(color, normalize(LightDirection), viewDirection, normalize(getCombinedAnimatedNormal())), Color.a);

	//FragColor = texture(Heightmap, worldToTextureCoord(WorldPosition.xz)) * Color; //debug heightmap
	//FragColor = mix(vec4(0.5), vec4(1.0), vec4(WorldNormal, 1)); //debug normals
}
