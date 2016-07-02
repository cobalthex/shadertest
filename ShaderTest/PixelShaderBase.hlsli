#define MS_None   = 0
#define MS_Left   = 1
#define MS_Middle = 2
#define MS_Right  = 4
#define MS_X1     = 8
#define MS_X2     = 16

cbuffer CBFrameValues : register(b0)
{
	uint4 Mouse; //x, y, mouse state (bit flags of MouseState), scroll value

	uint2 Resolution; //in pixels
	float AspectRatio;

	float Time; //in seconds
};

#define MAX_TEXTURES 32

Texture2D Textures[MAX_TEXTURES]; //todo: dynamically generate

sampler SLinearClamp = sampler_state
{
	addressU = Clamp;
	addressV = Clamp;
	mipfilter = NONE;
	minfilter = LINEAR;
	magfilter = LINEAR;
};
