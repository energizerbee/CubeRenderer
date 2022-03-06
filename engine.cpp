#include <cmath>
#include <array>
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <unistd.h>

// Welcome to a small project I got bored and decided to do. The end result is to create a rasterizer capable of simple shading from the ground up only using the bash
// console as a canvas. This project is still unfinished but please check out what I have created so far, as of now it is capable of simple shading with a wireframe cube :)

using namespace std;

string GetStdoutFromCommand(string cmd) {

    string data;
    FILE * stream;
    const int max_buffer = 256;
    char buffer[max_buffer];
    cmd.append(" 2>&1");

    stream = popen(cmd.c_str(), "r");

    if (stream) {
        while (!feof(stream))
            if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
        pclose(stream);
    }
    return data;
}

// pretend that this is a #define because i am too lazy to change all of the variable names
int WIDTH = 0;
int HEIGHT = 0;

int *canvas;

// I am going to take a second and thank javidx9/OneLoneCoder for a portion of the code... Thanks man, you are the goat. Keep making good stuff!

struct vertex
{
    float x,y,z;
};


struct triangle
{
    vertex p[3];
};


struct mesh
{
    vector<triangle> tris;
};


struct fourbyfour
{
    float m[4][4] = { 0 };
};


vector<vertex> pixels;

// A bunch of functions so the code space doesn't get messy.
vertex vec_add(vertex &a, vertex &b)
{
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}


vertex vec_sub(vertex &a, vertex &b)
{
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}


vertex vec_mul(vertex &a, float b)
{
    return { a.x * b, a.y * b, a.z * b };
}


vertex vec_div(vertex &a, float b)
{
    return { a.x / b, a.y / b, a.z / b };
}


float dot_product(vertex &a, vertex &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}


vertex normalize(vertex &n)
{
    float l = sqrtf(dot_product(n, n));
    return { n.x / l, n.y / l, n.z / l };
}


vertex cross_product(vertex &a, vertex &b)
{
    vertex c;
    c.x = a.y * b.z - a.z * b.y;
    c.y = a.z * b.x - a.x * b.z;
    c.z = a.x * b.y - a.y * b.x;
    return c;
}

// These are here for when the rasterizer will eventually be implemented.
bool compareByValueX(const vertex &a, const vertex &b)
{
    return a.x < b.x;
}


bool compareByValueY(const vertex &a, const vertex &b)
{
    return a.y < b.y;
}


// The beautiful matrix math courtesy of javidx9, what a legend.
void matrix(vertex &i, vertex &o, fourbyfour &m)
{
    o.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + m.m[3][0];
    o.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + m.m[3][1];
    o.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + m.m[3][2];
    float w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + m.m[3][3];

    if (w != 0.0f)
        o.x /= w; o.y /= w; o.z /= w;
}


void color(vertex norm, vertex point, triangle &triProj, vertex Lamp)
{
    float shading = 255 * dot_product(norm, Lamp);
    
    triProj.p[0].z = shading;
    triProj.p[1].z = shading;
    triProj.p[2].z = shading;
}


void screen()
{
    for(int i=0; i<=pixels.size(); i++)
    {
        int x = (int)pixels[i].x;
        int y = (int)pixels[i].y;
        int depth = abs((int)pixels[i].z);
        if(x >= WIDTH or y >= WIDTH)
            continue;
        else if(x < 0 or  y < 0)
            continue;
        else
            canvas[(y * WIDTH) + x] = depth;
    }
}


void defineline(float x1, float y1, float z1, float x2, float y2, float z2, std::vector<vertex> &vect)
{
    // The bresenham line algorithm courtesy of this soldier, keep grinding man! https://github.com/encukou/bresenham
    float dx, dy, xx, xy, yx, yy, xsign, ysign, D, y;
    
    dx = x2 - x1;
    dy = y2 - y1;
    
    if( dx > 0 )
        xsign = 1;
    else
        xsign = -1;
    
    if( dy > 0 )
        ysign = 1;
    else
        ysign = -1;
    
    dx = abs(dx);
    dy = abs(dy);
    
    if( dx > dy)
    {
        xx = xsign;
        xy = 0;
        yx = 0;
        yy = ysign;
    }
    else
    {
        int temp = dx;
        dx = dy;
        dy = temp;
        
        xx = 0;
        xy = ysign;
        yx = xsign;
        yy = 0;
    }
    D = 2 * dy - dx;
    y = 0;
    
    for(int x = 0; x < dx; x++)
    {
        vect.push_back( { floor((x1 + x * xx + y * yx)), floor((y1 + x * xy + y * yy)), z2 } );
        if(D >= 0)
        {
            y++;
            D -= 2 * dx;
        }
        D += 2*dy;
    }
}


int main()
{
    // Get terminal size
    stringstream commandh(GetStdoutFromCommand("tput lines"));
    stringstream commandw(GetStdoutFromCommand("tput cols"));
    
    // I have to do this to keep it global 😢😢
    commandh >> HEIGHT;
    commandw >> WIDTH;
    
    canvas = new int[HEIGHT * WIDTH];
    
    fourbyfour projected;
    fourbyfour matRotZ, matRotX;
    float fTheta = 0.0f;
    float fTime = 0.1f;
    
    float fNear = 0.1f;
    float fFar = 1000.0f;
    float fFov = 90.0f;
    float fAspectRatio = (float)HEIGHT / (float)WIDTH;
    float fFovRad = 1.0f / tanf(fFov * 0.5f / 180.0f * 3.14159f);

    projected.m[0][0] = fAspectRatio * fFovRad;
    projected.m[1][1] = fFovRad;
    projected.m[2][2] = fFar / (fFar - fNear); 
    projected.m[3][2] = (-fFar * fNear) / (fFar - fNear);
    projected.m[2][3] = 1.0f;
    projected.m[3][3] = 0.0f;
    
    mesh cube;
    vertex Camera;
    vertex Lamp = { 2.0f, 0.0f, 2.0f };
    //normalize lamp
    Lamp = normalize(Lamp);
    
    cube.tris = {
        // SOUTH
        { 0.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f },

        // EAST                                                  
        { 1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,    1.0f, 1.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 1.0f,    1.0f, 0.0f, 1.0f },

        //                                                    
        { 1.0f, 0.0f, 1.0f,    1.0f, 1.0f, 1.0f,    0.0f, 1.0f, 1.0f },
        { 1.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f,    0.0f, 0.0f, 1.0f },

        // WEST                                                 
        { 0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f,    0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f },

        // TOP                                                       
        { 0.0f, 1.0f, 0.0f,    0.0f, 1.0f, 1.0f,    1.0f, 1.0f, 1.0f },
        { 0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 1.0f,    1.0f, 1.0f, 0.0f },

        // BOTTOM                                                    
        { 1.0f, 0.0f, 1.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f,    1.0f, 0.0f, 0.0f },
    };
    
    // It will be like this until i feel like implementing keystrokes
    while(true)
    {
        // Overwrite all values in array with black
        for(int i = 0; i < HEIGHT * WIDTH; i++)
            canvas[i] = 0;
        
        fTheta += 1.0f * fTime;
        
        // rotation matrix for the x axis
        matRotX.m[0][0] = 1;
        matRotX.m[1][1] = cosf(fTheta * 0.5f);
        matRotX.m[1][2] = sinf(fTheta * 0.5f);
        matRotX.m[2][1] = -sinf(fTheta * 0.5f);
        matRotX.m[2][2] = cosf(fTheta * 0.5f);
        matRotX.m[3][3] = 1;
        
        // rotation matrix for the z axis
        matRotZ.m[0][0] = cosf(fTheta);
        matRotZ.m[0][1] = sinf(fTheta);
        matRotZ.m[1][0] = -sinf(fTheta);
        matRotZ.m[1][1] = cosf(fTheta);
        matRotZ.m[2][2] = 1;
        matRotZ.m[3][3] = 1;
        
        for(auto tri : cube.tris) {
            triangle projectedTriangle, triRotatedZ, triRotated;
            
            // rotate
            matrix(tri.p[0], triRotatedZ.p[0], matRotZ);
            matrix(tri.p[1], triRotatedZ.p[1], matRotZ);
            matrix(tri.p[2], triRotatedZ.p[2], matRotZ);
            
            matrix(triRotatedZ.p[0], triRotated.p[0], matRotX);
            matrix(triRotatedZ.p[1], triRotated.p[1], matRotX);
            matrix(triRotatedZ.p[2], triRotated.p[2], matRotX);
            
            // scale
            triRotated.p[0].x *= 5.0f; triRotated.p[0].y *= 5.0f;
            triRotated.p[1].x *= 5.0f; triRotated.p[1].y *= 5.0f;
            triRotated.p[2].x *= 5.0f; triRotated.p[2].y *= 5.0f;
            
            //translate
            triRotated.p[0].z += 5.0f; triRotated.p[0].y += 8.0f; triRotated.p[0].x += 8.0f; 
            triRotated.p[1].z += 5.0f; triRotated.p[1].y += 8.0f; triRotated.p[1].x += 8.0f;
            triRotated.p[2].z += 5.0f; triRotated.p[2].y += 8.0f; triRotated.p[2].x += 8.0f;
            
            // calculate normal 
            vertex normal, lineA, lineB;
            lineA = vec_sub(triRotated.p[1], triRotated.p[0]);
            lineB = vec_sub(triRotated.p[2], triRotated.p[0]);
            normal = cross_product(lineA, lineB);
            normal = normalize(normal);
        
            if(normal.x * (triRotated.p[0].x - Camera.x) + normal.y * (triRotated.p[0].y - Camera.y) + normal.z * (triRotated.p[0].z - Camera.z) < 0.0f) {
                // project
                matrix(triRotated.p[0], projectedTriangle.p[0], projected);
                matrix(triRotated.p[1], projectedTriangle.p[1], projected);
                matrix(triRotated.p[2], projectedTriangle.p[2], projected);
                
                // scale it up weirdly because SOME IDIOT decided to do it in ascii instead of a windowing system

                projectedTriangle.p[0].x *= 0.5f * (float)WIDTH; projectedTriangle.p[0].y *= 0.07f * (float)WIDTH;
                projectedTriangle.p[1].x *= 0.5f * (float)WIDTH; projectedTriangle.p[1].y *= 0.07f * (float)WIDTH;
                projectedTriangle.p[2].x *= 0.5f * (float)WIDTH; projectedTriangle.p[2].y *= 0.07f * (float)WIDTH;
                
                color(normal, triRotated.p[0], projectedTriangle, Lamp);
                // create the wireframe
                defineline((int)projectedTriangle.p[0].x, (int)projectedTriangle.p[0].y, (int)projectedTriangle.p[0].z, (int)projectedTriangle.p[1].x, (int)projectedTriangle.p[1].y, (int)projectedTriangle.p[1].z, pixels);
                defineline((int)projectedTriangle.p[1].x, (int)projectedTriangle.p[1].y, (int)projectedTriangle.p[1].z, (int)projectedTriangle.p[2].x, (int)projectedTriangle.p[2].y, (int)projectedTriangle.p[2].z, pixels);
                defineline((int)projectedTriangle.p[2].x, (int)projectedTriangle.p[2].y, (int)projectedTriangle.p[2].z, (int)projectedTriangle.p[0].x, (int)projectedTriangle.p[0].y, (int)projectedTriangle.p[0].z, pixels);
            }
        }
        screen();
        
        // Draw the pixels and output them and you done
        for(int i = 0; i < HEIGHT * WIDTH; i++) {
            //this is not the fastest way but it should do for now
            stringstream output;
            output << "\033[48;2;" << canvas[i] << ";" << canvas[i] << ";" << canvas[i] << "m ";
            cout << output.str();
        }
        // and lastly some clean up to start the next frame
        pixels.clear();
        // So the program doesnt go too crazy
        usleep(100 * 1000);
    }
    return 0;
}


