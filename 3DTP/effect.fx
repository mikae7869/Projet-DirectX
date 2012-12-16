shared matrix	WorldViewProj;
shared float3	LightPosition;
shared float3	LightDiffuseColor;
shared float3	LightAmbiantColor;
shared float3	LightSpecularColor;
shared float	LightDistance;
shared float3	CameraPos;
shared matrix	ViewProj;

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
};

/*
float tapWeight[12] = { 0.55f, 0.5f, 0.45f, 0.4f, 0.35f, 0.3f, 0.2f, 0.1f, 0.05f, 0.02f, 0.01f, 0.001f };
float tapOffset[11] = { 0.001f, 0.005f, 0.010f ,0.015f, 0.020f, 0.025f, 0.030f, 0.035f, 0.040f, 0.045f, 0.050f};*/

float tapWeight[18] = { 0.55f, 0.5f, 0.45f, 0.4f, 0.35f, 0.3f, 0.2f, 0.1f, 0.05f, 0.02f, 0.014f, 0.008f, 0.005f, 0.0032f, 0.0028f, 0.0024f, 0.002f, 0.001f };
float tapOffset[17] = { 0.001f, 0.005f, 0.010f ,0.015f, 0.020f, 0.025f, 0.030f, 0.040f, 0.050f, 0.060f, 0.070f, 0.080f, 0.090f, 0.1f, 0.11f, 0.13, 0.16};

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
	float3 lightDir = normalize(input.psPosition - mul(float4(LightPosition, 1.0f), ViewProj));
	//float3 lightDir = normalize(input.psPosition - LightPosition);

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
	/*float4 tbloom = tex2D(BloomMapSamplerScreen, input.UV);
	float4 tbase = tex2D(DiffuseMapSamplerScreen, input.UV);

	// Désaturation d'image de base pour ne pas avoir de couleurs trop vives une fois la texture de bloom ajoutée.
	// Plus l'intensité de la lumière est forte, plus on la diminue : on prend l'opposé en RGB.
	tbase = tbase * (1 - tbloom);
 
	// Finalement, nous assemblons les 2 images.
	return  tbase + tbloom;
	//return float4(1, 0, 0, 1);*/
	float2 inTex = input.UV;

	float4 original = tex2D( DiffuseMapSamplerScreen, inTex);
	float4 blur		= tex2D( BloomMapSamplerScreen, inTex );
	inTex		   -= 0.5;
	float vignette	= 1 - dot( inTex, inTex );
	blur *= pow( vignette, 4.0 );
	blur *= 2;
	blur = pow (blur, 0.55);

	float4 color = lerp( original, blur, 0.4f );

	return color;
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
	return (float4(1,1,1,1)); 

}


ScreenBlur BlurVS(ScreenVertex input)
{
	ScreenBlur OUT;

	OUT.position = input.Position;
	OUT.Tap0 = input.UV;
	OUT.UV = input.UV;

	
	return OUT;
}

float4 VBlurPS(ScreenVertex input) : COLOR0
{
	float color = 0;
	//color = tex2D(DiffuseMapSamplerScreen, input.UV).r * tapWeight[0];
	
	for (int i = 0; i < 17; i++)
	{
		float2 tap = input.UV + tapOffset[i] * _scale * float2 (0,1);
		color += tex2D(DiffuseMapSamplerScreen, tap) * tapWeight[i+1];
		float2 tapneg = input.UV - tapOffset[i] * _scale * float2 (0,1);
		color += tex2D(DiffuseMapSamplerScreen, tapneg) * tapWeight[i+1];
    }
	
	return float4(color, color, color, 1);
}

float4 HBlurPS(ScreenVertex input) : COLOR0
{
	float color = 0;
	//color = tex2D(DiffuseMapSamplerScreen, input.UV).r * tapWeight[0];
	for (int i = 0; i < 17; i++)
	{
		float2 tap = input.UV + tapOffset[i] * _scale * float2 (1,0);
		color += tex2D(DiffuseMapSamplerScreen, tap) * tapWeight[i+1];
		float2 tapneg = input.UV - tapOffset[i] * _scale * float2 (1,0);
		color += tex2D(DiffuseMapSamplerScreen, tapneg) * tapWeight[i+1];
    }
	
	return float4(color, color, color, 1);
}




float g_fSigma = 0.5f;



ScreenVertex PostProcessVS(ScreenVertex input)
{
	return input;
}


float4  ExposureControl(ScreenBlur input) : COLOR0 
{
	//return tex2D(DiffuseMapSamplerScreen, input.UV);
	float Luminance = 0.08f;
	static const float fMiddleGray = 0.18f;
	static const float fWhiteCutoff = 0.8f;
    float4 Color;

    Color = tex2D( DiffuseMapSamplerScreen, input.UV ) * fMiddleGray / ( Luminance + 0.001f );
    Color *= ( 1.0f + ( Color / ( fWhiteCutoff * fWhiteCutoff ) ) );
    Color /= ( 1.0f + Color );

    return Color;
}

float4 ToneMapPS (ScreenVertex input) : COLOR0
{

	float exposureLevel = 1.0;
	float gammaLevel = 0.5;
	float deFogLevel = 0.0;

	float3 fogColor = { 1.0, 1.0, 1.0 };

    half3 hdrTexelColor = tex2D( DiffuseMapSamplerScreen, input.UV );
    
    // Apply de-fogging
	hdrTexelColor = max( 0, hdrTexelColor - (deFogLevel * fogColor) );
	
	// Apply expsosure
    hdrTexelColor *= pow( 2.0, exposureLevel);
    
    // Apply gamma correction (you could use a texture lookups for this)
	hdrTexelColor = pow( hdrTexelColor, gammaLevel );
    float4 color = half4( hdrTexelColor.rgb, 1.0 );
	return color;
}

float4 earthHaloHBlurPS (ScreenVertex input) : COLOR0
{
	
	float color = tex2D(DiffuseMapSamplerScreen, input.UV).r * tapWeight[0];
	
	for (int i = 0; i < 2; i++)
	{
		float2 tap = input.UV + tapOffset[i] * _scale * float2 (0,1);
		color += tex2D(DiffuseMapSamplerScreen, tap) * tapWeight[i+1];
		float2 tapneg = input.UV - tapOffset[i] * _scale * float2 (0,1);
		color += tex2D(DiffuseMapSamplerScreen, tapneg) * tapWeight[i+1];
    }
	
	return float4(color, color, color, 1);
}

float4 earthHaloVBlurPS (ScreenVertex input) : COLOR0
{
	float color = 0;
	color = tex2D(DiffuseMapSamplerScreen, input.UV).r * tapWeight[0];
	
	for (int i = 0; i < 2; i++)
	{
		float2 tap = input.UV + tapOffset[i] * _scale * float2 (1,0);
		color += tex2D(DiffuseMapSamplerScreen, tap) * tapWeight[i+1];
		float2 tapneg = input.UV - tapOffset[i] * _scale * float2 (1,0);
		color += tex2D(DiffuseMapSamplerScreen, tapneg) * tapWeight[i+1];
    }
	
	return float4(color, color, color, 1);
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

technique blurH
{
	pass p0
	{
		VertexShader = compile vs_3_0 PostProcessVS();
		PixelShader  = compile ps_3_0 HBlurPS();
	}

}

technique blurV
{
	pass p1
	{
		VertexShader = compile vs_3_0 PostProcessVS();
		PixelShader  = compile ps_3_0 VBlurPS();
	}
}

technique exposure
{
    pass p0
    {
        VertexShader = compile vs_3_0 PostProcessVS();
        PixelShader = compile ps_3_0 ExposureControl();
    }
}

technique ToneMap
{
	pass p0
	{
		VertexShader = compile vs_3_0 PostProcessVS();
        PixelShader = compile ps_3_0 ToneMapPS();
	}
}

technique earthHaloHBlur
{
	pass p0
	{
		VertexShader = compile vs_3_0 PostProcessVS();
        PixelShader = compile ps_3_0 earthHaloHBlurPS();
	}
}

technique earthHaloVBlur
{
	pass p0
	{
		VertexShader = compile vs_3_0 PostProcessVS();
        PixelShader = compile ps_3_0 earthHaloVBlurPS();
	}
}