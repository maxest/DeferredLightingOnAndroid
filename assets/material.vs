uniform mediump mat4 viewProjTransform;

attribute vec4 attrib_position;

varying mediump vec4 varying_screenTexCoord;



void main()
{
	mediump vec4 position_NDC = attrib_position * viewProjTransform;

	gl_Position = position_NDC;
	
	varying_screenTexCoord = position_NDC;
	varying_screenTexCoord.xy = 0.5*varying_screenTexCoord.xy + 0.5*varying_screenTexCoord.w;
}
