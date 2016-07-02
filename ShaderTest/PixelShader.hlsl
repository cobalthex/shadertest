float4 main(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD) : SV_TARGET
{
	float4 texel = Textures[0].Sample(SLinearClamp, TexCoord);

	if (length(Mouse.xy - Position.xy) < 10)
		return texel.zyxw;

	TexCoord -= 0.5;
	float len = length(TexCoord);
	float theta = atan2(TexCoord.y, TexCoord.x) + 3.141;
	return len < 0.5 && len > 0.4 && theta < Time % 6.283 ? ((Time / 6.283) % 1) - texel : texel;
}