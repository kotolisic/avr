// #include <avr/interrupt.h>

#include <graphics.cpp>
#include <kb.cpp>

Graphics g;
KB kb;

struct vec2  { float x, y; };
struct vec3  { float x, y, z; };
struct vec4i { int   x, y, z, w; };

vec2 projection(vec3 s) {

    float fov = 100.0;
    vec2 a;
    
    a.x = 160 + fov*s.x/s.z;
    a.y = 100 - fov*s.y/s.z;
    
    return a;
}

int main() {

    g.start();
    g.block(0,0,320,200,1);

    vec3 vertex[8] = {

        {-1.0, -1.0,  1.0 + 3.0},
        { 1.0, -1.0,  1.0 + 3.0},
        { 1.0, -1.0, -1.0 + 3.0},
        {-1.0, -1.0, -1.0 + 3.0},

        {-1.0,  1.0,  1.0 + 3.0},
        { 1.0,  1.0,  1.0 + 3.0},
        { 1.0,  1.0, -1.0 + 3.0},
        {-1.0,  1.0, -1.0 + 3.0}
    };

    vec4i points[4] = {
        {0, 1, 2, 3},
        {4, 5, 6, 7},
        {0, 1, 5, 4},
        {2, 3, 7, 6},
    };

    for (int i = 0; i < 4; i++) {

        int  vx = points[ i ].x;
        int  vy = points[ i ].y;
        int  vz = points[ i ].z;
        int  vw = points[ i ].w;
        
        vec2 pa = projection(vertex[ vx ]);
        vec2 pb = projection(vertex[ vy ]);
        vec2 pc = projection(vertex[ vz ]);
        vec2 pd = projection(vertex[ vw ]);

        g.line(pa.x, pa.y, pb.x, pb.y, 14);
        g.line(pb.x, pb.y, pc.x, pc.y, 13);
        g.line(pc.x, pc.y, pd.x, pd.y, 12);
        g.line(pd.x, pd.y, pa.x, pa.y, 11);
    }

    g.print("Hello, World!");

    for(;;);
}
