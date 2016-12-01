#version 330 core 
in vec3 fragPos;
out vec4 color;

in vec3 fragNor;
uniform vec3 lightPos;
uniform vec3 MatDif;
uniform vec3 MatSpec;
uniform vec3 MatAmb;
uniform float shine;

void main()
{
    vec3 L = normalize(lightPos - fragPos);
    vec3 norm = normalize(fragNor);
    vec3 lightColor = vec3(1.0, 1.0, 1.0);

    vec3 diff = MatDif * max(0, dot(norm, L)) * lightColor;

    vec3 v = normalize(-fragPos);
    vec3 r = (-L + 2 * max(0, dot(L, norm)) * norm);
    vec3 spec = MatSpec * pow(max(0, dot(v, r)), shine) * lightColor;

    vec3 amb = MatAmb * lightColor;

    color = vec4(diff + spec + amb, 1.0);
}
