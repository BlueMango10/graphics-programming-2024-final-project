#version 330 core

in vec3 WorldPosition;
in vec3 WorldNormal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec4 Color;
uniform sampler2D Heightmap;
uniform vec4 HeightmapBounds; // xy = min coord, zw = max coord

// surface
uniform float AmbientReflection;
uniform float DiffuseReflection;
uniform float SpecularReflection;
uniform float SpecularExponent;
uniform sampler2D Albedo;

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

vec3 ambient(vec3 albedo)
{
	return AmbientColor * AmbientReflection * albedo;
}

vec3 diffuse(vec3 albedo, vec3 lightDirection, vec3 normal)
{
	return LightColor * DiffuseReflection * albedo * max(dot(lightDirection, normal), 0.0);
}

vec3 specular(vec3 lightDirection, vec3 viewDirection, vec3 normal)
{
	vec3 halfVector = normalize(lightDirection + viewDirection);
	return LightColor * SpecularReflection * pow(max(dot(halfVector, normal), 0.0), SpecularExponent);
}

vec3 blinnPhong(vec3 albedo, vec3 lightDirection, vec3 viewDirection, vec3 normal)
{
	return ambient(albedo) + diffuse(albedo, lightDirection, normal) + specular(lightDirection, viewDirection, normal);
}

void main()
{
	vec3 albedo = Color.rgb * texture(Albedo, WorldPosition.xz).rgb;
	vec3 viewDirection = normalize(CameraPosition - WorldPosition);

	FragColor = vec4(blinnPhong(albedo, normalize(LightDirection), viewDirection, normalize(WorldNormal)), 1.0);

	//FragColor = texture(Heightmap, worldToTextureCoord(WorldPosition.xz)) * Color; //debug heightmap
	//FragColor = mix(vec4(0.5), vec4(1.0), vec4(WorldNormal, 1)); //debug normals
}
