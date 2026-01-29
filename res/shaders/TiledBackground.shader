#shader vertex
#version 330 core

layout (location = 0) in vec2 aLocalXZ;   // [0..1] quad in XZ
layout (location = 1) in vec2 aUV;

layout (location = 2) in vec2 iOriginXZ;  // world-space origin (x,z)
layout (location = 3) in float iLayer;    // texture array layer index
layout (location = 4) in float iHasData;  // 1.0 if valid, else 0.0
layout (location = 5) in float iTileWorldSize;

out vec2 vUV;
flat out float vLayer;
flat out float vHasData;
out vec2 vWorldXZ;
	out vec3 vWorldPos;
	out float vScreenY;
	out float vForwardDist;

	uniform mat4 view;
	uniform mat4 projection;
	uniform float u_planeY;
	uniform vec2 u_cameraPosXZ;
	uniform vec2 u_cameraForwardXZ;

void main()
{
    vec3 worldPos = vec3(
        iOriginXZ.x + aLocalXZ.x * iTileWorldSize,
        u_planeY,
        iOriginXZ.y + aLocalXZ.y * iTileWorldSize
    );

    vec4 viewPos = view * vec4(worldPos, 1.0);
    vec4 clip = projection * viewPos;

    vUV = aUV;
    vLayer = iLayer;
    vHasData = iHasData;
	vWorldXZ = worldPos.xz;
	vWorldPos = worldPos;
	vScreenY = clip.y / max(1e-6, clip.w) * 0.5 + 0.5;
	vForwardDist = dot(worldPos.xz - u_cameraPosXZ, u_cameraForwardXZ);
	gl_Position = clip;
}

#shader fragment
#version 330 core

in vec2 vUV;
flat in float vLayer;
flat in float vHasData;
in vec2 vWorldXZ;
	in vec3 vWorldPos;
	in float vScreenY;
	in float vForwardDist;

out vec4 FragColor;

uniform sampler2DArray u_tiles;
uniform vec4 u_fallbackColor;
uniform float u_gridSpacing;
uniform int u_majorEvery;
uniform float u_minorLineWidth;
uniform float u_majorLineWidth;
uniform vec4 u_minorLineColor;
uniform vec4 u_majorLineColor;

	uniform float u_horizonBlendStart;
	uniform float u_horizonBlendEnd;
	uniform float u_horizonBlendPixels;
	uniform float u_farDistance;

// Sky gradient colors used as the blend target (sky itself is rendered in a separate pass).
uniform vec3 u_skyTopColor;
uniform vec3 u_skyBottomColor;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

float gridLine1D(float coord, float spacing, float halfWidth)
{
    if (halfWidth <= 0.0 || spacing <= 0.0)
    {
        return 0.0;
    }

    // Work in normalized cell space to keep derivatives stable.
    float x = coord / spacing;
    float w = halfWidth / spacing;

    // Distance to nearest integer boundary (0 at grid line).
    float f = fract(x);
    float d = min(f, 1.0 - f);

    // Anti-alias width based on screen-space derivatives of the *continuous* coordinate.
    float aa = max(fwidth(x), 0.0001);

    // Keep lines from collapsing into subpixel flicker: enforce a minimum screen-space thickness.
    // (This also reduces the "every other line looks thicker" aliasing you can get on dense grids.)
    w = max(w, aa * 0.5);

    // Clamp to half a cell; beyond that we’re effectively averaging multiple lines anyway.
    aa = min(aa, 0.5);

    return 1.0 - smoothstep(w - aa, w + aa, d);
}

void main()
{
    vec4 baseColor = u_fallbackColor;
    if (vHasData < 0.5)
    {
        baseColor = u_fallbackColor;
    }
    else
    {
        baseColor = texture(u_tiles, vec3(vUV, vLayer));
    }

    float grid = max(0.0001, u_gridSpacing);
    float majorGrid = grid * max(u_majorEvery, 1);
    float minorHalfW = max(u_minorLineWidth, 0.0);
    float majorHalfW = max(u_majorLineWidth, 0.0);

    float minorAlpha = max(
        gridLine1D(vWorldXZ.x, grid, minorHalfW),
        gridLine1D(vWorldXZ.y, grid, minorHalfW)
    );

    float majorAlpha = max(
        gridLine1D(vWorldXZ.x, majorGrid, majorHalfW),
        gridLine1D(vWorldXZ.y, majorGrid, majorHalfW)
    );

	// Ground->sky blending near horizon:
	// Fade both base color and line intensity to avoid a hard seam and reduce distant aliasing.
	// IMPORTANT: Use *camera-forward distance* so "end" lines up with farDistance selection (not camera absolute view depth).
	float farD = max(u_farDistance, 0.0);
	float startClamp = u_horizonBlendStart;
	float endD = (u_horizonBlendEnd > 0.0) ? min(u_horizonBlendEnd, farD) : farD;
	endD = max(endD, 0.0001);

	// Keep the transition to just a few pixels, regardless of world distance.
	float px = max(u_horizonBlendPixels, 0.0);
	float w = max(fwidth(vForwardDist), 1e-6) * px;
	float effectiveStart = endD - w;
	if (startClamp > 0.0)
	{
		effectiveStart = max(effectiveStart, startClamp);
	}
	float blendT = smoothstep(effectiveStart, endD, vForwardDist);

    // Fade lines out slightly faster than the base color to reduce high-frequency shimmer.
    float lineFade = 1.0 - blendT;
    minorAlpha *= lineFade;
    majorAlpha *= lineFade;

    vec4 color = baseColor;
    color = mix(color, u_minorLineColor, minorAlpha);
    color = mix(color, u_majorLineColor, majorAlpha);

	// Blend the farthest ground color toward the sky gradient (tinted toward major-line magenta),
	// to soften the horizon seam without affecting the sky pass.
	vec3 skyColor = mix(u_skyBottomColor, u_skyTopColor, saturate(vScreenY));
	vec3 targetColor = mix(skyColor, u_majorLineColor.rgb, 0.65);
	color.rgb = mix(color.rgb, targetColor, blendT);
	FragColor = vec4(color.rgb, 1.0);
}
