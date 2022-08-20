vec3  a_position  : POSITION;
vec2  a_texcoord0 : TEXCOORD0;
vec4  a_color0    : COLOR0;

noperspective vec2  v_texcoord0 : TEXCOORD0 = vec2(0.0, 0.0);
vec4  v_color0    : COLOR0    = vec4(1.0, 0.0, 0.0, 1.0);

flat uvec4 v_texcoord1 : TEXCOORD7 = uvec4(0, 0, 0, 0);
flat uvec4 v_texcoord2 : TEXCOORD6 = uvec4(0, 0, 0, 0);
flat uvec4 v_texcoord3 : TEXCOORD5 = uvec4(0, 0, 0, 0);