#version 140
uniform sampler2D Sprite;
in vec2 texCoord;
out vec4 fragColor;
void main(void){
	fragColor = texture(Sprite, texCoord);
}