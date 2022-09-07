// world
float3x3 normalMatrix;
float4x4 invTransform;
float4x4 worldViewMatrix;
float4x4 worldViewProjMatrix;

// lighting
float  linearAttenuation;
float  lightRange;
float  lightRadius;
float4 lightPosition;
float4 diffuseProduct;
float4 specularProduct;

// texture
texture textureDiffuseColor;
sampler textureSampler = sampler_state
{
    Texture   = (textureDiffuseColor);
    MinFilter = ANISOTROPIC;
    MagFilter = ANISOTROPIC;
};

// depth texture
texture zTexture;
sampler zTextureSampler = sampler_state
{
    Texture   = (zTexture);
    MinFilter = LINEAR;
    MagFilter = LINEAR;
};

//
// Simple Pixel lighting technique
//
struct VS_INPUT_PL
{
    float4 position   : POSITION; 
    float3 normal	  : NORMAL; 
};

struct VS_INPUT_PLT
{
    float4 position   : POSITION; 
    float3 normal	  : NORMAL; 
    float2 texel	  : TEXCOORD0;
};

struct VS_OUTPUT_PL
{
    float4 position			: POSITION; 
    float3 normal			: TEXCOORD0; 
    float3 lightDirection	: TEXCOORD1;
    float3 eyeDirection		: TEXCOORD2; 
};

struct VS_OUTPUT_PLT
{
    float4 position			: POSITION; 
    float3 normal			: TEXCOORD0; 
    float3 lightDirection	: TEXCOORD1;
    float2 texel			: TEXCOORD2;
    float3 eyeDirection		: TEXCOORD3; 
};

struct VS_INPUT_VE
{
	float4 position			: POSITION;
	float3 vNormal0			: TEXCOORD0;	
	float3 vNormal1			: TEXCOORD1;		
	float4 normal			: NORMAL;		
	float3 backNormal		: TEXCOORD2;	
	float4 edge				: TEXCOORD3;	
};

struct VS_OUTPUT_VE
{
	float4 position			: POSITION;
	float4 front			: TEXCOORD0;
    float4 back				: TEXCOORD1; 
	float4 left				: TEXCOORD2;
    float4 right			: TEXCOORD3; 	
	float4 projPos			: TEXCOORD4;
};

struct VS_OUTPUT_ZF
{
	float4 	position		: POSITION;
    float4  depth			: TEXCOORD0; // in word space
};

// General function
float4 DiffuseProduct(float3 normal, float3 lightDirection)
{
	return diffuseProduct * max( dot(normal, lightDirection), 0.0 );
}

float4 SpecularProduct(float3 normal, float3 eyeDirection, float3 lightDirection)
{
	float3 reflect1 = normalize(reflect(lightDirection, normal));
	return specularProduct * pow( max(dot(reflect1, eyeDirection), 0.0), 6.0);
}

// Vertex shader
VS_OUTPUT_PL RenderSceneVS_PL( VS_INPUT_PL vertex )
{
    VS_OUTPUT_PL 	output;
    float3 			position = mul(vertex.position, worldViewMatrix);   
    
    // Output
    output.position			= mul( vertex.position, worldViewProjMatrix );
    output.normal			= normalize( mul( vertex.normal, normalMatrix ) );
    output.lightDirection	= lightPosition - position;
    output.eyeDirection 	= normalize(position);

    return output;    
}

// Pixel shader
float4 RenderScenePS_PL( VS_OUTPUT_PL vertex ) : COLOR0
{
    float4 		output;
	float 		dist = length(vertex.lightDirection);
	float 		attenuation = 1.0 + dist * linearAttenuation;
	
    output =  	DiffuseProduct(vertex.normal, vertex.lightDirection/dist) 
				+ SpecularProduct( vertex.normal, vertex.eyeDirection, vertex.lightDirection);  
	
    return output / attenuation;
}

// Vertex shader
VS_OUTPUT_PLT RenderSceneVS_PLT( VS_INPUT_PLT vertex )
{
    VS_OUTPUT_PLT 	output;
    float3 			position = mul(vertex.position, worldViewMatrix);   
    
    // Output
    output.position			= mul( vertex.position, worldViewProjMatrix );
    output.normal			= normalize( mul( vertex.normal, normalMatrix ) );
    output.lightDirection	= lightPosition - position;
    output.eyeDirection 	= normalize(position);
    output.texel			= vertex.texel;
	
    return output;    
}

// Pixel shader
float4 RenderScenePS_PLT(VS_OUTPUT_PLT vertex) : COLOR0
{
    float4 		output;
	float 		dist = length(vertex.lightDirection);
	float 		attenuation = 1.0 + dist * linearAttenuation;
	
    output = DiffuseProduct( vertex.normal, vertex.lightDirection/dist ) * tex2D(textureSampler, vertex.texel) 
				  + SpecularProduct( vertex.normal, vertex.eyeDirection, vertex.lightDirection);
	
    return output / attenuation;
}

technique Lighting
{
    pass P0
    {          
        VertexShader = compile vs_2_0 RenderSceneVS_PL();
        PixelShader  = compile ps_2_0 RenderScenePS_PL(); 
		
		ColorWriteEnable = red | green | blue;
		
		CullMode = CCW;		
		
        ZEnable = true;
        ZFunc = LessEqual;
		ZWriteEnable = false;   
		DepthBias = 0.0;	
		SlopeScaleDepthBias = 0.0;
		
        StencilEnable = true;
		StencilRef = 0x10;
        StencilFunc = LessEqual;
        StencilPass = Keep;
	
        AlphaBlendEnable = true;
        BlendOp = Add;
        SrcBlend = DestAlpha;
        DestBlend = One;	
    }
	
    pass P1
    {          
        VertexShader = compile vs_2_0 RenderSceneVS_PLT();
        PixelShader  = compile ps_2_0 RenderScenePS_PLT(); 
		
		ColorWriteEnable = red | green | blue;
		CullMode = CCW;		
		
        ZEnable = true;
        ZFunc = LessEqual;
		ZWriteEnable = false;   
		DepthBias = 0.0;	
		SlopeScaleDepthBias = 0.0;
		
        StencilEnable = true;
		StencilRef = 0x10;
        StencilFunc = LessEqual;
        StencilPass = Keep;
	
        AlphaBlendEnable = true;
        BlendOp = Add;
        SrcBlend = DestAlpha;
        DestBlend = One;		
    }	
}

// Vertex shader
float4 ExtrudeFromLight( VS_INPUT_VE vertex ) : POSITION
{
	float4  vertPos  = vertex.position; // + silhouettePlane * dot(silhouettePlane, vertex.position);
	float4  lightVec = vertPos - lightPosition;// + float4(vertex.vNormal0 * 0.15, 0.0);
	float4 	extruded;
	
	if ( dot(lightVec.xyz, vertex.normal.xyz) < 0.0 )
	{	
		extruded = vertPos + normalize(lightVec) * (lightRange - length(lightVec));
	}
	else extruded = vertex.position;// + normalize(lightVec) * 0.2;// * 0.5;
	
    return mul(extruded, worldViewProjMatrix);    
}

float4 ExtrudeOuter( VS_INPUT_VE vertex ) : POSITION
{
	float4	extruded;
	float3  dir;
	float3  sphereVert0;
		
	dir = vertex.position.xyz - lightPosition;
	if ( dot(dir, vertex.normal.xyz) < 0.0 )
	{
		dir = vertex.vNormal0 * lightRadius;
		sphereVert0 = lightPosition - dir;

		extruded = vertex.position;	
		dir = vertex.position.xyz - sphereVert0;
		extruded.xyz += normalize(dir) * (lightRange - length(dir));
	}
	else 
		extruded = vertex.position;
		
    return mul(extruded, worldViewProjMatrix);    
}

// Pixel shader
float4 Fill() : COLOR0
{
    return 0.05;
}

float4 GetLRPlane(float3 pos, float3 sv0, float3 sv1)
{
	float4 plane;
	
	plane.xyz = normalize( cross(pos - sv0, pos - sv1) );
	plane.w = -dot(pos, plane.xyz);	

	return plane;
} 

float4 GetFBPlane(float3 pos, float3 edge, float3 sv)
{
	float4 plane;
	
	plane.xyz = normalize( cross(edge, pos - sv) );
	plane.w = -dot(pos, plane.xyz);
	
	return plane;
} 

// Vertex shader
VS_OUTPUT_VE ExtrudePenumbra( VS_INPUT_VE vertex )
{
	VS_OUTPUT_VE 	result;
	float4  		extruded;
	float3			dir;
	float3  		sphereVert0, sphereVert1;
	
	// discard
	dir = vertex.position - lightPosition;
	if ( dot(dir, vertex.normal) * dot(dir, vertex.backNormal) > 0.0 )
	{
		result.front = result.back = result.left = result.right = 0.0;
		result.projPos = result.position = 0.0;//mul(vertex.position - dir, worldViewProjMatrix);//9999999.0;
		return result;
	}

	// make it a little bigger to eliminate holes between umbra && penumbra
	sphereVert0 = lightPosition + vertex.vNormal0 * 0.15;// + (vertex.vNormal0 + vertex.vNormal1) * 0.05;
	sphereVert1 = lightPosition - vertex.vNormal0 * lightRadius;
	extruded = vertex.position;	
	if ( vertex.normal.w > 0.0 ) //  vertex.normal.w > 0.0 
	{
		dir = vertex.position.xyz - sphereVert0;
		extruded.xyz += normalize(dir) * (lightRange - length(dir));
	}
	else if ( vertex.normal.w < 0.0 )
	{
		dir = vertex.position.xyz - sphereVert1;	
		extruded.xyz += normalize(dir) * (lightRange - length(dir));
	}		
	result.projPos = result.position = mul(extruded, worldViewProjMatrix);
	
	// Find planes of penumbra cone
	dir = vertex.edge * vertex.edge.w;
	result.front = GetFBPlane(vertex.position.xyz, dir, sphereVert1);
	result.back = -GetFBPlane(vertex.position.xyz, dir, sphereVert0);

	if (vertex.edge.w > 0.0)
	{
		result.left = -GetLRPlane(vertex.position.xyz, sphereVert0, sphereVert1);
		result.right = GetLRPlane(vertex.position.xyz + vertex.edge.xyz, lightPosition, sphereVert0 - vertex.vNormal1 * lightRadius);
	}
	else
	{
		result.right = -GetLRPlane(vertex.position.xyz, sphereVert0, sphereVert1);
		result.left = GetLRPlane(vertex.position.xyz + vertex.edge.xyz, lightPosition, sphereVert0 - vertex.vNormal1 * lightRadius);
	}
	
    return result;    
}

// Pixel shader
float4 PenumbraAlpha( VS_OUTPUT_VE vertex ) : COLOR0
{
	float2 projPos = vertex.projPos / vertex.projPos.w;
	float2 texel = float2(projPos.x + 1.0, -projPos.y + 1.0) / 2.0;
	float4 vertPos = mul(float4(projPos.x, projPos.y, tex2D(zTextureSampler, texel).r, 1.0), invTransform);

	float  distInner = dot(vertPos, vertex.back);
	float  distOuter = dot(vertPos, vertex.front);

	if (dot(vertPos, vertex.left) * dot(vertPos, vertex.right) > 0.0 && distInner * distOuter > 0.0)
	{
		float alpha = distInner / (distOuter + distInner);
		return 3*pow(alpha, 2.0) - 2*pow(alpha, 3.0);
	}
	else
		return 1.0;
}


technique ShowPenumbraCone
{
    pass P0
    {          
        VertexShader = compile vs_2_0 ExtrudeOuter();
        PixelShader  = compile ps_2_0 Fill(); 
		
		CullMode = CW;
		
        AlphaBlendEnable = true;   
        BlendOp = Add;
        SrcBlend = DestAlpha;
        DestBlend = One;		
		ColorWriteEnable = red | green | blue | alpha;

		ZEnable = true;
        ZWriteEnable = false;
        ZFunc = Less;
		
		StencilEnable = false;
		SlopeScaleDepthBias = 0.0;
		DepthBias = 0.0;
    }
}

technique Shadow
{
    pass P0
    {          
        VertexShader = compile vs_2_0 ExtrudeFromLight();
        PixelShader  = compile ps_2_0 Fill(); 
		
		CullMode = None;
		
        AlphaBlendEnable = false;   
        BlendOp = Add;
        SrcBlend = DestAlpha;
        DestBlend = One;		
		ColorWriteEnable = false;

		ZEnable = true;
        ZWriteEnable = false;
        ZFunc = LessEqual;

		SlopeScaleDepthBias = 0.0;
		DepthBias = 0.0;
		
        TwoSidedStencilMode = true;
        StencilEnable = true;
	    StencilMask = 0xFF;
        StencilWriteMask = 0xFF;	
        Ccw_StencilFunc = Always;
        Ccw_StencilZFail = Incr;
        Ccw_StencilPass = Keep;
        StencilFunc = Always;
        StencilZFail = Decr;
        StencilPass = Keep;	
    }	
	
    pass P1
    {          
        VertexShader = compile vs_2_0 ExtrudePenumbra();
        PixelShader  = compile ps_2_0 PenumbraAlpha(); 
		
        CullMode = None;
		ColorWriteEnable = alpha;
        
		ZEnable = true;
		ZWriteEnable = false;
		ZFunc = Greater;
        
		SlopeScaleDepthBias = 0.1;
		DepthBias = 0.0001;
		
		AlphaBlendEnable = true;
		BlendOp = Min;
        SrcBlend = One;
        DestBlend = One;

		StencilEnable = true;
		TwoSidedStencilMode = false;
		StencilRef = 0x10;
	    StencilWriteMask = 0;	
        StencilFunc = LessEqual;
        StencilPass = Keep;		
    }
}


// VertexShader
VS_OUTPUT_ZF ZFillVS( float4 position : POSITION )
{
	VS_OUTPUT_ZF 	result;
	
	result.position = mul(position, worldViewProjMatrix);
	result.depth	= result.position;//result.position.z/result.position.w;
	
	return result;
}

// Pixel shader
float4 ZFillPS( VS_OUTPUT_ZF vertex ) : COLOR0
{
	return float4(vertex.depth.z/vertex.depth.w, 0.0f, 0.0f, 1.0f);
}

technique ZFill
{
    pass P0
    {          
        VertexShader = compile vs_2_0 ZFillVS();
        PixelShader  = compile ps_2_0 ZFillPS(); 

        StencilEnable = false;
        AlphaBlendEnable = false;	
		CullMode = CCW;		
        ZEnable = true;
		ZWriteEnable = true;    
		ZFunc = LessEqual;
		DepthBias = 0.0;
   		ColorWriteEnable = red | green | blue | alpha;		
    }
}

// VertexShader
float4 ClearStencilAlphaVS( float4 position : POSITION ) : POSITION
{
	return position;
}

// Pixel shader
float4 ClearStencilAlphaPS() : COLOR0
{
	return 1.0f;
}

technique ClearStencilAlpha
{
    pass P0
    {          
        VertexShader = compile vs_2_0 ClearStencilAlphaVS();
        PixelShader  = compile ps_2_0 ClearStencilAlphaPS(); 

		CullMode = none;
		ZEnable = false;	
		AlphaBlendEnable = false;
		StencilEnable = true;
        StencilMask = 0xFF;		
        StencilWriteMask = 0xFF;
        StencilFunc = Always;
		StencilRef = 0x10;
		StencilPass = Replace;
		ColorWriteEnable = alpha;
    }
}

