#version 330 core

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;

out vec3 WorldPosition;
out vec3 WorldNormal;
out vec2 TexCoord;
out mat3 TBN;
out vec2 TexSquish; // Basically how much the texture is squished due to wave movement

// world
uniform mat4 WorldMatrix;
uniform mat4 ViewProjMatrix;
uniform float Time;

// terrain info
uniform sampler2D Heightmap;
uniform vec4 HeightmapBounds; // xy = min coord, zw = max coord
uniform float HeightScale;
uniform float HeightOffset;

// shape
uniform vec4 WaveFrequency;
uniform vec4 WaveSpeed;
uniform vec4 WaveDirectionX; // I pass the direction uniform this way to better group thing together in the c++ code.
uniform vec4 WaveDirectionY;
uniform vec4 WaveHeight;
uniform vec4 WaveWidth;
uniform float CoastOffset;
uniform float CoastExponent;
uniform float WaveScale;

// shading
uniform float NormalSampleOffset;

// convert world coordinates to texture coordinates
vec2 worldToTextureCoord(vec2 worldSpacePosition)
{
	return (worldSpacePosition - HeightmapBounds.xy) / (HeightmapBounds.zw - HeightmapBounds.xy);
}

// get depth
float getDepth(vec3 worldPosition)
{
	float height = texture(Heightmap, worldToTextureCoord(worldPosition.xz)).r
		* HeightScale + HeightOffset;
	return -height;
}

// get vertex offset produced by a Gerstner wave
vec3 gerstnerWave(vec3 worldPosition, float speed, float frequency, float height, float width, vec2 direction)
{
	float waveTime = (((worldPosition.x * direction.x) + (worldPosition.z * direction.y) + (Time * speed)) * frequency);
	float cosWT = cos(waveTime); // no reason to do this twice
	vec3 offset = vec3(cosWT * direction.x * width, sin(waveTime) * height, cosWT * direction.y * width); // gerstner wave happens here B)
	return offset;
}

// get the final world position from the original world position
vec3 getPosition(vec3 worldPosition)
{
	float waveScale = max(0, getDepth(worldPosition) + CoastOffset);
	waveScale = min(waveScale, pow(waveScale, CoastExponent));
	vec3 wave = gerstnerWave(worldPosition, WaveSpeed.x, WaveFrequency.x, WaveHeight.x, WaveWidth.x, vec2(WaveDirectionX.x, WaveDirectionY.x));
	wave += gerstnerWave(worldPosition, WaveSpeed.y, WaveFrequency.y, WaveHeight.y, WaveWidth.y, vec2(WaveDirectionX.y, WaveDirectionY.y));
	wave += gerstnerWave(worldPosition, WaveSpeed.z, WaveFrequency.z, WaveHeight.z, WaveWidth.z, vec2(WaveDirectionX.z, WaveDirectionY.z));
	wave += gerstnerWave(worldPosition, WaveSpeed.w, WaveFrequency.w, WaveHeight.w, WaveWidth.w, vec2(WaveDirectionX.w, WaveDirectionY.w));
	return worldPosition + wave * waveScale * WaveScale;
}

// approximate normal
vec3 getNormal(vec3 worldPosition, float sampleOffset)
{
	// to get the normal, we sample the height at the position and at an adjacent position on each axis
	vec3 baseSample = getPosition(worldPosition);
	vec3 xSample = getPosition(vec3(worldPosition.x + sampleOffset, worldPosition.yz));
	vec3 zSample = getPosition(vec3(worldPosition.xy, worldPosition.z + sampleOffset));
	// we then get the vectors from the base position to the two extra samples
	vec3 tangent = xSample - baseSample;
	vec3 biTangent = zSample - baseSample;
	// set texture squish
	TexSquish = vec2(length(tangent), length(biTangent));
	// normalize for next step (we can reuse TexSquish for an easy optimization)
	tangent = tangent / TexSquish.x;
	biTangent = biTangent / TexSquish.y;
	// finally, we approximate the normal by taking the cross product of these two vectors
	vec3 normal = normalize(cross(biTangent, tangent));
	// before returning the final normal, we use these vectors to create a TBN vector for later use
	TBN = mat3(tangent, biTangent, normal);
	// return the normal
	return normal;
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
