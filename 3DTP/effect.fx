shared matrix	WorldViewProj;
shared float3	LightPosition;
shared float3	LightDiffuseColor;
shared float3	LightAmbiantColor;
shared float3	LightSpecularColor;
shared float	LightDistance;
shared float3	CameraPos;

float2	g_vSourceDimensions;
float2	g_vDestinationDimensions;

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

struct ScreenBlur {
	float4 position : POSITION;

	float2 Tap0 : TEXCOORD0;
	float2 Tap1 : TEXCOORD1;
	float2 Tap2 : TEXCOORD2;
	float2 Tap3 : TEXCOORD3;
	float2 Tap1Neg : TEXCOORD4;
	float2 Tap2Neg : TEXCOORD5;
	float2 Tap3Neg : TEXCOORD6;
	float2 UV : TEXCOORD7;
	float2 Direction : TEXCOORD8;
};



//float tapWeight[7] = { 0.5f, 0.4f, 0.3f, 0.2f, 0.1f, 0.05f, 0.02f };
//float tapOffset[6] = { 0.003f, 0.006f, 0.009f, 0.012f, 0.016f, 0.022f};

float tapWeight[10] = { 0.9f, 0.9f, 0.45f, 0.4f, 0.35f, 0.3f, 0.2f, 0.1f, 0.05f, 0.02f };
float tapOffset[9] = { 0.000f, 0.003f, 0.006f ,0.010f, 0.013f, 0.016f, 0.019f, 0.022f, 0.028f};

static const float BlurWeights[13] = 
{
    0.002216,
    0.008764,
    0.026995,
    0.064759,
    0.120985,
    0.176033,
    0.199471,
    0.176033,
    0.120985,
    0.064759,
    0.026995,
    0.008764,
    0.002216,
};

float PixelKernel[13] =
{
    -6,
    -5,
    -4,
    -3,
    -2,
    -1,
     0,
     1,
     2,
     3,
     4,
     5,
     6,
};

float _scale = 1;


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

sampler2D PointSampler0 = sampler_state
{
    Texture = <DiffuseMap>;
    MinFilter = point;
    MagFilter = point;
    MipFilter = point;
    MaxAnisotropy = 1;
    AddressU = CLAMP;
    AddressV = CLAMP;
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
	return  tbase + tbloom;
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
/*
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
}*/

ScreenBlur BlurVS(ScreenVertex input, uniform float2 DIRECTION)
{
	ScreenBlur OUT;

	OUT.position = input.Position;
	OUT.Direction = DIRECTION;
	OUT.Tap0 = input.UV;
	OUT.UV = input.UV;
	OUT.Tap1 = input.UV + tapOffset[0] * _scale * DIRECTION;
	OUT.Tap1Neg = input.UV - tapOffset[0] * _scale * DIRECTION;
	OUT.Tap2 = input.UV + tapOffset[1] * _scale * DIRECTION;
	OUT.Tap2Neg = input.UV - tapOffset[1] * _scale * DIRECTION;
	OUT.Tap3 = input.UV + tapOffset[2] * _scale * DIRECTION;
	OUT.Tap3Neg = input.UV - tapOffset[2] * _scale * DIRECTION;
	
	return OUT;
}

float4 BlurPS(ScreenBlur input) : COLOR0
{
	float color = 0;
	//color = tex2D(DiffuseMapSamplerScreen, input.UV).r * tapWeight[0];
	
	for (int i = 0; i < 9; i++)
	{
		float2 tap = input.UV + tapOffset[i] * _scale * float2 (0,1);
		color += tex2D(DiffuseMapSamplerScreen, tap) * tapWeight[i+1];
		float2 tapneg = input.UV - tapOffset[i] * _scale * float2 (0,1);
		color += tex2D(DiffuseMapSamplerScreen, tapneg) * tapWeight[i+1];
    }
	for (int i = 0; i < 9; i++)
	{
		float2 tap = input.UV + tapOffset[i] * _scale * float2 (1,0);
		color += tex2D(DiffuseMapSamplerScreen, tap) * tapWeight[i+1];
		float2 tapneg = input.UV - tapOffset[i] * _scale * float2 (1,0);
		color += tex2D(DiffuseMapSamplerScreen, tapneg) * tapWeight[i+1];
    }


	/*for (int i = 0; i < 13; i++)
	{
		float2 tap = input.UV + PixelKernel[i] * _scale * input.Direction;
		color += tex2D(DiffuseMapSamplerScreen, tap).r * BlurWeights[i];
    }
	for (int i = 0; i < 3; i++)
	{
		float2 tapneg = input.UV - PixelKernel[i] * _scale * input.Direction;
		color += tex2D(DiffuseMapSamplerScreen, tapneg).r * BlurWeights[i];
    }*/


	
	return float4(color, color, color, 1);
}



float g_fSigma = 0.5f;



ScreenVertex PostProcessVS(ScreenVertex input)
{
	return input;
}

float CalcGaussianWeight(int iSamplePoint)
{
	float g = 1.0f / sqrt(2.0f * 3.14159 * g_fSigma * g_fSigma);  
	return (g * exp(-(iSamplePoint * iSamplePoint) / (2 * g_fSigma * g_fSigma)));
}

float4 GaussianBlurH (in float2 in_vTexCoord : TEXCOORD0,
						uniform int iRadius,
						uniform int bEncodeLogLuv)	: COLOR0
{
    float4 vColor = 0;
	float2 vTexCoord = in_vTexCoord;

    for (int i = -iRadius; i < iRadius; i++)
    {   
		float fWeight = CalcGaussianWeight(i);
		vTexCoord.x = in_vTexCoord.x + (i / g_vSourceDimensions.x);
		float4 vSample = tex2D(PointSampler0, vTexCoord);
		/*if (bEncodeLogLuv)
			vSample = float4(LogLuvDecode(vSample), 1.0f);*/
		vColor += vSample * fWeight;
    }

	/*if (bEncodeLogLuv)
		vColor = LogLuvEncode(vColor.rgb);*/
	
	return vColor;
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
		VertexShader = compile vs_3_0 BlurVS(float2(0,1));
		PixelShader  = compile ps_3_0 BlurPS();
	}
	pass p1
	{
		VertexShader = compile vs_3_0 BlurVS(float2(1,0));
		PixelShader  = compile ps_3_0 BlurPS();
	}
}

technique GaussianBlur
{
    pass p0
    {
        VertexShader = compile vs_3_0 PostProcessVS();
        PixelShader = compile ps_3_0 GaussianBlurH(6, false);
    }
}