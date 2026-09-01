#version 330 core

in vec3 vs_position;
in vec3 vs_color;
in vec3 vs_normal;
in vec2 vs_texcoord;

out vec4 fs_color;

void main()
{
    // Directional light source for 3D surface shading
    vec3 lightDir = normalize(vec3(0.5, 0.8, 0.4));
    vec3 norm = length(vs_normal) > 0.001 ? normalize(vs_normal) : vec3(0.0, 1.0, 0.0);
    
    // Ambient & Diffuse lighting
    float ambient = 0.35;
    float diff = max(dot(norm, lightDir), 0.0);
    
    vec3 shadedColor = vs_color * (ambient + diff * 0.65);
    fs_color = vec4(shadedColor, 1.f);
}
