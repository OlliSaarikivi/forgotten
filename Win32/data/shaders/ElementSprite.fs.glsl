#version 140
uniform sampler2DArray SpriteArray;
uniform float SpriteIndex;
in vec2 texCoord;
out vec4 fragColor;
void main(void){
	fragColor = texture(SpriteArray, vec3(texCoord, SpriteIndex), 0);
}