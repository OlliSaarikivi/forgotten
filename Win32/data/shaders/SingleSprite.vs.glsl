#version 140
uniform mat4 TransformView, Projection;
in vec2 Position;
out vec2 texCoord;
void main(void){
	gl_Position = Projection * TransformView * vec4(Position, 0.0, 1.0);
	texCoord = Position;
}