shared matrix	WorldViewProj;
shared float3	LightPosition;
shared float3	LightDiffuseColor;
shared float3	LightAmbiantColor;
shared float3	LightSpecularColor;
shared float	LightDistance;
shared float3	CameraPos;

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
	float4 psPosition	: TEXCOORD1;
};

struct ScreenVertex
{
	float4	Position	: POSITION;
	float2	UV			: TEXCOORD0;
};

texture2D DiffuseMap;
texture2D BloomBlurMap;

sampler2D DiffuseMapSampler = sampler_state
{
	Texture		= <DiffuseMap>;
	MinFilter	= LINEAR;
	MagFilter	= LINEAR;
	MipFilter	= LINEAR;
	AddressU	= WRAP;
	AddressV	= WRAP;
};

sampler2D DiffuseMapSamplerScreen = sampler_state
{
	Texture		= <DiffuseMap>;
	MinFilter	= LINEAR;
	MagFilter	= LINEAR;
	MipFilter	= LINEAR;
	AddressU	= Clamp;
	AddressV	= Clamp;
};

sampler2D BloomMapSamplerScreen = sampler_state
{
	Texture		= <BloomBlurMap>;
	MinFilter	= LINEAR;
	MagFilter	= LINEAR;
	MipFilter	= LINEAR;
	AddressU	= Clamp;
	AddressV	= Clamp;
};

VertexOutput DiffuseVS(VertexInput input) 
{
	VertexOutput output;
	output.Position = mul(float4(input.Position, 1.0f), WorldViewProj);
	output.Normal = input.Normal;
	output.psPosition = output.Position;

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
	float4 texel = float4(tex2D(DiffuseMapSampler, float2(input.UV.x,input.UV.y)).rgb, 1);

	float4 color = float4(LightAmbiantColor, 1);
	
	// Get light direction for this fragment
	float3 lightDir = normalize(input.psPosition - LightPosition);

	// Note: Non-uniform scaling not supported
	float diffuseLighting = saturate(dot(input.Normal, -lightDir));
	if (diffuseLighting > 0.0f)
	{
		color += (float4(LightDiffuseColor,1) * diffuseLighting);
		color = saturate(color);
	}

	color = color * texel;

	return color;
}

ScreenVertex FinalVS(ScreenVertex input)
{

	return input;
}

float4 FinalPS(ScreenVertex input) : COLOR0
{
	// Nous récupèrons les 2 textures : le bloom et l'image.
	float4 tbloom = tex2D(BloomMapSamplerScreen, input.UV);
	float4 tbase = tex2D(DiffuseMapSamplerScreen, input.UV);
 
	// Désaturation d'image de base pour ne pas avoir de couleurs trop vives une fois la texture de bloom ajoutée.
	// Plus l'intensité de la lumière est forte, plus on la diminue : on prend l'opposé en RGB.
	tbase = tbase * (1 - tbloom);
 
	// Finalement, nous assemblons les 2 images.
	return tbase + tbloom;
	//return float4(1, 0, 0, 1);
}



VertexOutput SimpleSunVS(VertexInput input) 
{
	VertexOutput output;
	output.Position = mul(float4(input.Position, 1.0f), WorldViewProj);
	output.Normal = input.Normal;
	output.psPosition = output.Position;
	output.UV = input.UV;

	return output;
}

float4 SimpleSunPS(VertexOutput input) : COLOR0
{
	return float4 (1,1,1,1);
}


ScreenVertex BloomVS(ScreenVertex input)
{
	return input;
}

float4 BloomPS(ScreenVertex input) : COLOR0
{
	float4 pixel = float4(tex2D(DiffuseMapSamplerScreen, float2(input.UV.x,input.UV.y)).rgb, 1);
	//float intensity = dot(pixel, float4(0.3,0.59,0.11,0);
	float4 diff = pixel - float4(0.9,0.9,0.9,0);
	if (diff.x <= 0 || diff.y <= 0 || diff.z <= 0 )
	{
		return (float4(0,0,0,1));
	}
	return (float4(1,1,1,1)); //float4 treshold value
	//return float4(1, 0, 0, 1);
}

ScreenVertex BlurVS(ScreenVertex input)
{

	return input;
}

float4 BlurPS(ScreenVertex input) : COLOR0
{
	float blurOffset	=	0.006;
	float4 color		=	0;
 
	color  = tex2D(DiffuseMapSamplerScreen, float2(input.UV.x + blurOffset, input.UV.y + blurOffset));
	color += tex2D(DiffuseMapSamplerScreen, float2(input.UV.x - blurOffset, input.UV.y - blurOffset));
	color += tex2D(DiffuseMapSamplerScreen, float2(input.UV.x + blurOffset, input.UV.y - blurOffset));
	color += tex2D(DiffuseMapSamplerScreen, float2(input.UV.x - blurOffset, input.UV.y + blurOffset));
	color = color / 4;
 
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

technique final
{
	pass p0
	{
		VertexShader = compile vs_3_0 FinalVS();
		PixelShader  = compile ps_3_0 FinalPS();
	}
}

technique rendersun
{
	pass p0
	{
		VertexShader = compile vs_3_0 SimpleSunVS();
		PixelShader  = compile ps_3_0 SimpleSunPS();
	}
}

technique bloom
{
	pass p0
	{
		VertexShader = compile vs_3_0 BloomVS();
		PixelShader  = compile ps_3_0 BloomPS();
	}
}

technique blur
{
	pass p0
	{
		VertexShader = compile vs_3_0 BlurVS();
		PixelShader  = compile ps_3_0 BlurPS();
	}
}