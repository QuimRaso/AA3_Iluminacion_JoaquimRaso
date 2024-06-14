#version 440 core

uniform vec2 windowSize;
uniform sampler2D textureSampler;
uniform vec3 sunLight;
uniform vec3 moonLight;
uniform vec3 flashLight;
uniform bool linterna;
uniform bool sun;
uniform bool moon;
uniform vec3 flashLightDirection; // Dirección de la linterna

 float innerConeAngle =5.0f; 
 float outerConeAngle =10.0f; 

in vec2 uvsFragmentShader;
in vec3 normalsFragmentShader;
in vec4 primitivePosition;


out vec4 fragColor;

void main() {
        
        vec2 adjustedTexCoord = vec2(uvsFragmentShader.x, 1.0 - uvsFragmentShader.y);  
        vec4 baseColor = texture(textureSampler, adjustedTexCoord); 
        vec4 ambientColor = vec4(1.0,1.0,1.0,1.0);
        vec4 sunColor= vec4(0.5,0.5,0.0,1.0);
        vec4 moonColor= vec4(0.0,0.17,0.5,1.0);


        if(sun == true)
        {
         float distanceSun = length(sunLight - primitivePosition.xyz);
        // Atenuación por distancia
        float attenuationSun = 100.0 / (distanceSun * distanceSun);


        vec3 lightDirection = normalize(sunLight - primitivePosition.xyz);
        float sourceLightAngle = dot(normalsFragmentShader, lightDirection);

        fragColor += vec4(baseColor.rgb * sourceLightAngle,1.0) * (sunColor) * ambientColor * attenuationSun;

        
        }
        if(moon == true)
        {
       float distanceMoon = length(moonLight - primitivePosition.xyz);
        // Atenuación por distancia
        float attenuationMoon = 100.0 / (distanceMoon * distanceMoon);


        vec3 lightDirection = normalize(moonLight - primitivePosition.xyz);
        float sourceLightAngle = dot(normalsFragmentShader, lightDirection);

        fragColor += vec4(baseColor.rgb * sourceLightAngle,1.0) * (moonColor) * ambientColor * attenuationMoon;
        }
      

        if(linterna == true)
        {
        //vec3 flashLightDirection = normalize(flashLight - primitivePosition.xyz);
        //float flashLighttAngle = dot(normalsFragmentShader, flashLightDirection);

        //fragColor += vec4(baseColor.rgb * flashLighttAngle,1.0) * (ambientColor);

         vec3 lightDir = normalize(flashLight - primitivePosition.xyz);
        float distance = length(flashLight - primitivePosition.xyz);
        float theta = dot(lightDir, normalize(flashLightDirection));
        
        float epsilon = cos(innerConeAngle) - cos(outerConeAngle);
        float intensity = clamp((theta - cos(outerConeAngle)) / epsilon, 0.0, 0.5);

        // Atenuación por distancia
        float attenuation = 5.0 / (distance * distance);

        vec4 lightContribution = vec4(baseColor.rgb * max(dot(normalsFragmentShader, lightDir), 0.0), 1.0) * ambientColor * intensity * attenuation;
        fragColor += lightContribution;
        }


        
        
}
