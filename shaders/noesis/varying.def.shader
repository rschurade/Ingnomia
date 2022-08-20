vec2    a_position : POSITION;      // position
vec4    a_color0 : COLOR;           // color
vec2    a_texcoord0 : TEXCOORD0;    // UV set 0
vec2    a_texcoord1 : TEXCOORD1;    // UV set 1
vec4    a_texcoord2 : TEXCOORD2;    // rect
vec4    a_texcoord3 : TEXCOORD3;    // tile
vec4    a_texcoord4 : TEXCOORD4;    // imagePos
float   a_texcoord5 : TEXCOORD5;    // coverage

flat vec4   v_color0 : COLOR;
vec2        v_uv0 : TEXCOORD0;
vec2        v_uv1 : TEXCOORD1;
vec2        v_uv2 : TEXCOORD8;
vec2        v_uv3 : TEXCOORD7;
vec2        v_st1 : TEXCOORD6;
float       v_coverage : TEXCOORD5;
flat vec4   v_rect : TEXCOORD2;
flat vec4   v_tile : TEXCOORD3;
vec4        v_imagePos : TEXCOORD4;
