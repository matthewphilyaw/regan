#version 330

#define MAX_LIGHTS 128

in vec3 fragWorldPos;
in vec3 fragNormal;
in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec4 ambient;

uniform vec4 lightPosIntensity[MAX_LIGHTS];  // xyz = position, w = intensity
uniform vec4 lightColor[MAX_LIGHTS];         // rgb = color,    a = unused
uniform vec4 lightAabbMin[MAX_LIGHTS];       // xyz = box min,  w = unused
uniform vec4 lightAabbMax[MAX_LIGHTS];       // xyz = box max,  w = unused
uniform int  lightCount;
uniform float edgeFade;

void main()
{
    vec3 albedo = texture(texture0, fragTexCoord).rgb;
    vec3 N = normalize(fragNormal);

    vec3 lit = ambient.rgb;

    for (int i = 0; i < lightCount; i++)
    {
        vec3 lmin = lightAabbMin[i].xyz;
        vec3 lmax = lightAabbMax[i].xyz;

        vec3 d = min(fragWorldPos - lmin, lmax - fragWorldPos);
        if (any(lessThan(d, vec3(0.0)))) continue;

        vec3 fade = clamp(d / edgeFade, 0.0, 1.0);
        float atten = fade.x * fade.y * fade.z;

        vec3 L = normalize(lightPosIntensity[i].xyz - fragWorldPos);
        float ndotl = max(dot(N, L), 0.0);

        lit += lightColor[i].rgb * ndotl * atten * lightPosIntensity[i].w;
    }

    finalColor = vec4(albedo * lit, 1.0);
}