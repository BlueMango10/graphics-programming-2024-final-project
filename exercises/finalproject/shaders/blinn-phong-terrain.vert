#version 330 core

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;

out vec3 WorldPosition;
out vec3 WorldNormal;
out vec2 TexCoord;

uniform mat4 WorldMatrix;
uniform mat4 ViewProjMatrix;
uniform sampler2D Heightmap;
uniform vec4 HeightmapBounds; // xy = min coord, zw = max coord
uniform float HeightScale;
uniform float HeightOffset;
uniform float NormalSampleOffset;

// Convert world coordinates to texture coordinates
vec2 worldToTextureCoord(vec2 worldSpacePosition)
{
	return (worldSpacePosition - HeightmapBounds.xy) / (HeightmapBounds.zw - HeightmapBounds.xy);
}

// Get the final world position from the original world position
vec3 getPosition(vec3 worldPosition)
{
	worldPosition.y = texture(Heightmap, worldToTextureCoord(worldPosition.xz)).r
	 * HeightScale + HeightOffset;
	return worldPosition;
}

// Approximate normal
vec3 getNormal(vec3 worldPosition, float sampleOffset)
{
	// to get the normal, we sample the height at the position and at an adjacent position on each axis
	vec3 baseSample = getPosition(worldPosition - vec3(sampleOffset / 2));
	vec3 xSample = getPosition(vec3(baseSample.x + sampleOffset, baseSample.yz));
	vec3 zSample = getPosition(vec3(baseSample.xy, baseSample.z + sampleOffset));
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
	WorldPosition = getPosition(WorldPosition);

	// normal
	WorldNormal = getNormal(WorldPosition, NormalSampleOffset);

	gl_Position = ViewProjMatrix * vec4(WorldPosition, 1.0);
}
