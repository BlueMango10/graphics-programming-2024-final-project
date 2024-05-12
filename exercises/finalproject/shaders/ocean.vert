#version 330 core

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;

out vec3 WorldPosition;
out vec3 WorldNormal;
out vec2 TexCoord;
out float Depth;

// World
uniform mat4 WorldMatrix;
uniform mat4 ViewProjMatrix;
uniform float Time;

// Terrain info
uniform sampler2D Heightmap;
uniform vec4 HeightmapBounds; // xy = min coord, zw = max coord
uniform float HeightScale;
uniform float HeightOffset;

// Shape
uniform float WaveFrequency;
uniform float WaveSpeed;
uniform vec2 WaveDirection;
uniform float WaveHeight;
uniform float CoastOffset;

// Shading
uniform float NormalSampleOffset;

// Convert world coordinates to texture coordinates
vec2 worldToTextureCoord(vec2 worldSpacePosition)
{
	return (worldSpacePosition - HeightmapBounds.xy) / (HeightmapBounds.zw - HeightmapBounds.xy);
}

// Get depth
float getDepth(vec3 worldPosition)
{
	float height = texture(Heightmap, worldToTextureCoord(worldPosition.xz)).r
		* HeightScale + HeightOffset;
	return -height;
}

// Get vertex offset produced by a Gerstner wave
vec3 gerstnerWave(vec3 worldPosition, float speed, float frequency, float height, vec2 direction /*width is just the magnitude of the direction*/)
{
	float waveTime = (((worldPosition.x * direction.x) + (worldPosition.z * direction.y) + (Time * speed)) * frequency);
	float cosWT = cos(waveTime); // no reason to do this twice
	vec3 offset = vec3(cosWT * direction.x, sin(waveTime) * height, cosWT * direction.y); // gerstner wave happens here B)
	return offset;
}

// Get the final world position from the original world position
vec3 getPosition(vec3 worldPosition)
{
	/*
	float waveTime = ((worldPosition.x + (Time * WaveSpeed)) * WaveFrequency);
	float waveHeight = max(0, Depth + CoastOffset);
	vec3 wavePosition = worldPosition;
	wavePosition.y = worldPosition.y + (sin(waveTime) * WaveHeight);
	wavePosition.x = worldPosition.x + (cos(waveTime) * WaveWidth);
	worldPosition = mix(worldPosition, wavePosition, waveHeight);
	return worldPosition;
	*/
	float waveScale = max(0, Depth + CoastOffset);
	vec3 wave = gerstnerWave(worldPosition, WaveSpeed, WaveFrequency, WaveHeight, WaveDirection);
	//wave += gerstnerWave(worldPosition, WaveSpeed, WaveFrequency, WaveHeight, vec2(0, WaveWidth));
	return worldPosition + wave * waveScale;
}

// Approximate normal
vec3 getNormal(vec3 worldPosition, float sampleOffset)
{
	// to get the normal, we sample the height at the position and at an adjacent position on each axis
	vec3 baseSample = getPosition(worldPosition);
	vec3 xSample = getPosition(vec3(worldPosition.x + sampleOffset, worldPosition.yz));
	vec3 zSample = getPosition(vec3(worldPosition.xy, worldPosition.z + sampleOffset));
	// we then get the vectors from the base position to the two extra samples
	vec3 xDiff = xSample - baseSample;
	vec3 zDiff = zSample - baseSample;
	// finally, we approximate the normal by taking the cross product of these two vectors
	return normalize(cross(zDiff, xDiff));
}

void main()
{
	// find base values
	WorldPosition = (WorldMatrix * vec4(VertexPosition, 1.0)).xyz;
	WorldNormal = (WorldMatrix * vec4(VertexNormal, 0.0)).xyz;
	TexCoord = WorldPosition.xz;

	// position
	Depth = getDepth(WorldPosition);
	WorldPosition = getPosition(WorldPosition);

	// normal
	WorldNormal = getNormal(WorldPosition, NormalSampleOffset);

	gl_Position = ViewProjMatrix * vec4(WorldPosition, 1.0);
}
