static const uint MS_None = 0;
static const uint MS_Left = 1;
static const uint MS_Middle = 2;
static const uint MS_Right = 4;
static const uint MS_X1 = 8;
static const uint MS_X2 = 16;

cbuffer CBFrameValues : register(b0)
{
	uint4 Mouse; //x, y, mouse state (bit flags of MouseState), scroll value

	uint2 Resolution; //in pixels
	float AspectRatio;

	float Time; //in seconds
};
 
Texture2D BackBuffer;
Texture2D Textures[4]; //todo: dynamically generate

SamplerState Samplers[4] : register(s0); //todo: dynamically generate


float4 main(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD) : SV_TARGET
{
	if (length(Mouse.xy - Position.xy) < 10)
		return float4(1, 0, 0, 1);

	TexCoord -= 0.5;
	float len = length(TexCoord);
	float theta = atan2(TexCoord.y, TexCoord.x) + 3.141;
	return len < 0.5 && len > 0.4 && theta < Time % 6.283 ? float4(0, (Time / 6.283) % 1, 1, 1) : float4(0, 0, 0, 0);
}