float4 main(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD) : SV_TARGET
{
	if (length(Mouse.xy - Position.xy) < 10)
		return float4(1, 0, 0, 1);

	TexCoord -= 0.5;
	float len = length(TexCoord);
	float theta = atan2(TexCoord.y, TexCoord.x) + 3.141;
	return len < 0.5 && len > 0.4 && theta < Time % 6.283 ? float4(0, (Time / 6.283) % 1, 1, 1) : float4(0, 0, 0, 0);
}