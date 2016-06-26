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

SamplerState Samplers[4] : register(s0);
