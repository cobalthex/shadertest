float4 main(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD) : SV_TARGET
{
	TexCoord -= 0.5;
	float r = length(TexCoord - float2(0.5, 0.5));
	return (r < 0.5 ? float4(1, 1, 1, 1) : float4(0, 0, 0, 0));
}