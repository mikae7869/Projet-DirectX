shared matrix WorldViewProj;

struct VertexInput
{
	float3	Position	: POSITION;
	float2	UV			: TEXCOORD0;
    float3	Normal		: NORMAL;
};

struct VertexOutput
{
	float4	Position	: POSITION;
	float2	UV			: TEXCOORD0;
    float3	Normal		: NORMAL;
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
	output.Normal = input.Normal;

	float  p = 3.141592f;
	float3 x = float3(1, 0, 0);
	float3 z = float3(0, 0, 1);

	float3 norm = input.Normal;

	// find theta using the normal and z
	float theta = acos(dot(norm,z));

	// find the x-y projection of the normal
	float3 proj = normalize(float3(norm.x, norm.y, 0));

	// find phi using the x-y projection and x
	float phi = acos(dot(proj,x));

	// if x-y projection is in quadrant 3 or 4, then phi = 2PI - phi
	if (proj.y < 0)
			phi = 2*p - phi;

	float3 coords = float3(phi / (2*p), theta / p, 0);
  
	output.UV = coords;

	return output;
}

float4 DiffusePS(VertexOutput input) : COLOR0
{
	float4 color = float4(tex2D(DiffuseMapSampler,float2(input.UV.x,input.UV.y)).rgb,1);
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