$input v_color0, v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_texture, 0);
uniform vec4 u_materialColor;
uniform int u_materialBits;

void main()
{
	if (u_materialColor.a == 0.0f) {
	    gl_FragColor = vec4(u_materialColor.r, u_materialColor.g, u_materialColor.b, 1.0f);
	} else {
	    gl_FragColor = v_color0 * u_materialColor * texture2D(s_texture, v_texcoord0);
	}
	if (mod(u_materialBits, 2) == 1.0 && gl_FragColor.a < 0.75f) discard;
}
