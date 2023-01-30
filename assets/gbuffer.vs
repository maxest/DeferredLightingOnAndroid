uniform mediump mat4 viewTransform;
uniform mediump mat4 viewProjTransform;

attribute vec4 attrib_position;
attribute vec3 attrib_normal;

varying mediump float varying_depth_view;
varying mediump vec2 varying_normal_view;



void main()
{
	mediump vec4 position_view = attrib_position * viewTransform;
	mediump vec3 normal_view = (vec4(attrib_normal, 0.0) * viewTransform).xyz;

	gl_Position = attrib_position * viewProjTransform;
	
	varying_depth_view = position_view.z;
	varying_normal_view = normal_view.xy;
}
