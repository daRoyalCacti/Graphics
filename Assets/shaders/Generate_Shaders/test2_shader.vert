#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
	vec2 foo;
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout(binding = 2) uniform LightingObject {
	mat4 test1;
	mat4 test2;
} lo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

void main() {
    gl_Position = lo.test1 * lo.test2 * ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0) + vec4(ubo.foo, 1.0, 1.0);
    fragTexCoord = inTexCoord;
}
