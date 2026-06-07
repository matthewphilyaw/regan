#version 330

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoord;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

out vec3 fragWorldPos;
out vec3 fragNormal;
out vec2 fragTexCoord;

void main()
{
    fragWorldPos  = vec3(matModel * vec4(vertexPosition, 1.0));
    fragNormal    = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));
    fragTexCoord  = vertexTexCoord;

    gl_Position = mvp * vec4(vertexPosition, 1.0);
}