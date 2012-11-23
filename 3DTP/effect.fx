shared matrix WorldViewProj;

struct VertexInput
{
	float3 Position : POSITION;
	float2 UV		: TEXCOORD0;
};

struct VertexOutput
{
	float4 Position : POSITION;
	float2 UV		: TEXCOORD0;
};

texture2D DiffuseMap;

sampler2D DiffuseMapSampler = sampler_state
{
	Texture		= <DiffuseMap>;
	MinFilter	= LINEAR;
	MagFilter	= LINEAR;
	MipFilter	= LINEAR;
	AddressU	= WRAP;
	AddressV	= WRAP;
};

VertexOutput DiffuseVS(VertexInput input) 
{
	VertexOutput output;
	output.Position = mul(float4(input.Position, 1.0f), WorldViewProj);
	output.UV = input.UV;
	return output;
}

float4 DiffusePS(VertexOutput input) : COLOR0
{
	float4 color = tex2D(DiffuseMapSampler, input.UV);
	return color;
}

technique diffuse
{
  pass p0
  {
    VertexShader = compile vs_3_0 DiffuseVS();
    PixelShader  = compile ps_3_0 DiffusePS();
  }
}