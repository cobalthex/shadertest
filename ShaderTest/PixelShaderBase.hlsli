enum MouseState : uint
{
	MS_None = 0,
	MS_Left = 1,
	MS_Middle = 2,
	MS_Right = 4,
	MS_X1 = 8,
	MS_X2 = 16,
};

cbuffer CBFrameValues : register(b0)
{
	uint2 Resolution; //in pixels
	float AspectRatio;

	uint4 Mouse; //x, y, mouse state (bit flags of MouseState), scroll value
	
	float Time; //in seconds
};

Texture2D BackBuffer;
Texture2D Textures[4]; //todo: dynamically generate

SamplerState Samplers[4] : register(s0); //todo: dynamically generate