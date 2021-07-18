#version 330 core
out vec3 FragColor;

struct Material{
    vec4 specular;
    float shininess;
};
struct Light{
    vec3 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};
uniform Material material;
uniform Light light;

uniform mat4 viewMatrix;
uniform mat3 normalMatrix;
uniform vec3 rayOrigin;
uniform vec3 top;
uniform vec3 bottom;
uniform vec3 backgroundColor;
uniform vec2 viewportSize;
// 宽高比
uniform float aspectRatio;
// 眼睛到投射平面的距离
uniform float focalLength;
uniform float stepLength;
uniform float gamma;
uniform bool reverseGradient;
uniform float opacityThreshold;
uniform float colorThreshold;

uniform usampler3D volume;

// Ray
struct Ray{
    vec3 origin;
    vec3 direction;
};
// Axis-aligned bounding box
struct AABB{
    vec3 top;
    vec3 bottom;
};

// 计算当前像素对应的光线方向
vec3 getRayDirection(){
    vec3 rayDirection;
    // 转为 [-1, 1] 的 NDC 坐标
    rayDirection.xy=2.*gl_FragCoord.xy/viewportSize-1.;
    // 宽方向上需要进行缩放
    rayDirection.x*=aspectRatio;
    // focalLength 表示 NDC 下眼睛到投射平面的距离
    rayDirection.z=-focalLength;
    // 光线方向和 origin 一样需要进行 view 的逆变换
    rayDirection=(inverse(viewMatrix)*vec4(rayDirection,0)).xyz;
    return rayDirection;
}

float max3(vec3 v){
    return max(max(v.x,v.y),v.z);
}
float min3(vec3 v){
    return min(min(v.x,v.y),v.z);
}
// Slab method for ray-box intersection
void ray_box_intersection(Ray ray,AABB box,out float t_enter,out float t_exit)
{
    vec3 direction_inv=1./ray.direction;
    // 前后两个交点坐标
    vec3 t_top=direction_inv*(box.top-ray.origin);
    vec3 t_bottom=direction_inv*(box.bottom-ray.origin);
    // 在 x, y, z 的 enter 面上的参数 t
    vec3 t_xyz_enter=min(t_top,t_bottom);
    // 在 x, y, z 的 exit 面上的参数 t
    vec3 t_xyz_exit=max(t_top,t_bottom);
    t_enter=max(0,max3(t_xyz_enter));
    t_exit=min3(t_xyz_exit);
}

// A very simple color transfer function
vec4 color_transfer(float ratio)
{
    vec4 ans=vec4(0,0,0,0);
    
    const int n=3,MAX=4946;
    float opacityRatio[n]=float[](0,opacityThreshold,4946);
    float opacityVal[n]=float[](0,0,1);
    float colorRatio[n]=float[](0,colorThreshold,4946);
    vec3 colorVal[n]=vec3[](
        vec3(.23,.29,.75),
        vec3(.098,.3176,.7922),
        vec3(.70,.01,.14)
    );
    
    for(int i=0;i<n-1;i++){
        if(ratio>=opacityRatio[i]&&ratio<opacityRatio[i+1]){
            float newRatio=(ratio-opacityRatio[i])/(opacityRatio[i+1]-opacityRatio[i]);
            ans.a=opacityVal[i]*(1-newRatio)+opacityVal[i+1]*newRatio;
        }
        if(ratio>=colorRatio[i]&&ratio<colorRatio[i+1]){
            float newRatio=(ratio-colorRatio[i])/(colorRatio[i+1]-colorRatio[i]);
            ans.rgb=colorVal[i]*(1-newRatio)+colorVal[i+1]*newRatio;
        }
    }
    
    return ans;
}

// Estimate normal from a finite difference approximation of the gradient
vec3 normal(vec3 position,float intensity)
{
    float d=stepLength;
    float dx=texture(volume,position+vec3(d,0,0)).r-intensity;
    float dy=texture(volume,position+vec3(0,d,0)).r-intensity;
    float dz=texture(volume,position+vec3(0,0,d)).r-intensity;
    return normalize(normalMatrix*((reverseGradient?-1:1)*vec3(dx,dy,dz)));
}

void main(){
    vec3 v=getRayDirection();
    // the parametric equation of the ray: p = o + tv, where o is the origin of the ray, given by the position of the camera, and v is its direction, given by the vector going from the camera to the fragment
    // 光线和体数据的入射交点和出射交点分别对应参数 t0 和 t1
    float t_0,t_1;
    vec3 o=rayOrigin;
    Ray casting_ray=Ray(o,v);
    AABB bounding_box=AABB(top,bottom);
    ray_box_intersection(casting_ray,bounding_box,t_0,t_1);
    
    // 这里 ray_start 和 ray_stop 都是在 bottom 和 top 之间的比例，在 0~1 之间，所以后面的 texture() 采样用这个值的话，是在单位立方体坐标上采样的，正好符合 texture 的使用方法
    vec3 ray_start=(o+v*t_0-bottom)/(top-bottom);
    vec3 ray_stop=(o+v*t_1-bottom)/(top-bottom);
    
    vec3 ray=ray_stop-ray_start;
    float rayLength=length(ray);
    vec3 stepVector=stepLength*ray/rayLength;
    
    vec3 position=ray_start;
    // 背景需要抵消后面的 gamma 矫正，因为 Qt 里面没有校正
    vec4 color=vec4(pow(backgroundColor,vec3(gamma)),0);
    
    // Ray march until reaching the end of the volume, or color saturation
    while(rayLength>0&&color.a<1.){
        
        float intensity=texture(volume,position).r;
        
        vec4 c=color_transfer(intensity);
        
        // 传输函数的颜色作为材质的 ambient 和 diffuse 属性
        vec4 ambient=light.ambient*c;
        // diffuse
        vec3 norm=normal(position,intensity);
        vec3 lightDir=normalize(light.position-position);
        float diff=max(dot(norm,lightDir),0.);
        vec4 diffuse=light.diffuse*(diff*c);
        
        // specular
        vec3 viewDir=-normalize(v);
        vec3 h=normalize(lightDir+viewDir);
        float spec=pow(max(dot(norm,h),0.),material.shininess);
        vec4 specular=light.specular*(spec*material.specular);
        
        c=diffuse+specular+ambient;
        
        // Alpha-blending
        color.rgb=color.rgb+(1-color.a)*c.a*c.rgb;
        color.a=color.a+(1-color.a)*c.a;
        
        rayLength-=stepLength;
        position+=stepVector;
    }
    
    // Gamma correction
    FragColor=pow(color.rgb,vec3(1./gamma));
}
