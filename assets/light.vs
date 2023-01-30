uniform mediump mat4 viewProjTransform;
uniform mediump vec2 nearPlaneSize; // normalized (at zNear == 1)

attribute vec4 attrib_position;

varying mediump vec4 varying_screenTexCoord;
varying mediump vec3 varying_positionOnNearPlane;



void main()
{
	mediump vec4 position_NDC = attrib_position * viewProjTransform;

	gl_Position = position_NDC;
	
	varying_screenTexCoord = position_NDC;
	varying_screenTexCoord.xy = 0.5*varying_screenTexCoord.xy + 0.5*varying_screenTexCoord.w;
	
	mediump vec2 position_NDC_normalized = 0.5 * position_NDC.xy / position_NDC.w;
	varying_positionOnNearPlane = vec3(nearPlaneSize.xy * position_NDC_normalized, 1.0);
}
