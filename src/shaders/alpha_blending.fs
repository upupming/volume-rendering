#version 330 core
out vec4 FragColor;

uniform mat4 viewMatrix;
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

uniform sampler3D volume;

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

// Slab method for ray-box intersection
void ray_box_intersection(Ray ray,AABB box,out float t_0,out float t_1)
{
    vec3 direction_inv=1./ray.direction;
    vec3 t_top=direction_inv*(box.top-ray.origin);
    vec3 t_bottom=direction_inv*(box.bottom-ray.origin);
    vec3 t_min=min(t_top,t_bottom);
    vec2 t=max(t_min.xx,t_min.yz);
    t_0=max(0.,max(t.x,t.y));
    vec3 t_max=max(t_top,t_bottom);
    t=min(t_max.xx,t_max.yz);
    t_1=min(t.x,t.y);
}
// A very simple color transfer function
vec4 color_transfer(float ratio)
{
    vec4 ans=vec4(0,0,0,0);
    const int n=5;
    float opacityRatio[n]=float[](0,.4,.41,.65,1);
    float opacityVal[n]=float[](0,0,.80,.83,.80);
    float colorRatio[n]=float[](0,.2,.6,.65,1);
    vec3 colorVal[n]=vec3[](
        vec3(0,0,0),
        vec3(.9216,.3922,.0667),
        vec3(.8941,.0588,.3804),
        vec3(.2667,.0353,.8941),
        vec3(.502,.8,.1608)
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

void main(){
    vec3 v=getRayDirection();
    // the parametric equation of the ray: p = o + tv, where o is the origin of the ray, given by the position of the camera, and v is its direction, given by the vector going from the camera to the fragment
    // 光线和体数据的入射交点和出射交点分别对应参数 t0 和 t1
    float t_0,t_1;
    vec3 o=rayOrigin;
    Ray casting_ray=Ray(o,v);
    AABB bounding_box=AABB(top,bottom);
    ray_box_intersection(casting_ray,bounding_box,t_0,t_1);
    
    vec3 ray_start=(o+v*t_0-bottom)/(top-bottom);
    vec3 ray_stop=(o+v*t_1-bottom)/(top-bottom);
    
    vec3 ray=ray_stop-ray_start;
    float rayLength=length(ray);
    vec3 stepVector=stepLength*ray/rayLength;
    
    vec3 position=ray_start;
    vec4 color=vec4(0.);
    
    // Ray march until reaching the end of the volume, or color saturation
    while(rayLength>0&&color.a<1.){
        
        float intensity=texture(volume,position).r;
        
        vec4 c=color_transfer(intensity);
        
        // Alpha-blending
        color.rgb=color.rgb+(1-color.a)*c.a*c.rgb;
        color.a=color.a+(1-color.a)*c.a;
        
        rayLength-=stepLength;
        position+=stepVector;
    }
    
    // Blend background
    color.rgb=color.a*color.rgb+(1-color.a)*pow(backgroundColor,vec3(gamma)).rgb;
    color.a=1.;
    
    // Gamma correction
    FragColor.rgb=pow(color.rgb,vec3(1./gamma));
    FragColor.a=color.a;
}
