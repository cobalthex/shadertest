struct VSOutput
{
	linear float4 position : SV_POSITION;
	linear float2 texCoord : TEXCOORD;
};

VSOutput main(float2 Position : POSITION, float2 TexCoord : TEXCOORD)
{
	VSOutput vsout;
	vsout.position = float4(Position, 0, 1);
	vsout.texCoord = TexCoord;
	return vsout;
}