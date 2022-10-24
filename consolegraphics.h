#include <iostream>
#include <math.h>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#define NOMINMAX
#include <windows.h>

namespace console
{
    struct vec2d {
        double x, y;
    };
    struct vec3d {
        double x, y, z;
    };
    struct light{
        vec3d position;
        double intensity;
    };
    struct triangle {
        vec3d vertices[3];
        vec3d nml;
    };
    struct mesh {
        std::vector<triangle> tris;
        int fetchFromObj(std::string path)
        {
            std::ifstream file(path);
            std::vector<vec3d> verts;
            std::vector<vec3d> nmls;
            if (file.is_open())
            {
                std::string line;
                while (getline(file, line))
                {
                    //Fetching vertices and normals
                    if (line[0] == 'v')
                    {
                        //Vertices
                        if (line[1] == ' ')
                        {
                            vec3d tmp;
                            std::string junk;
                            std::stringstream s(line);
                            s >> junk >> tmp.x >> tmp.y >> tmp.z;
                            verts.push_back(tmp); //Inserting a new vertex into a pile of others
                        }
                        //Normals
                        else if (line[1] == 'n')
                        {
                            vec3d tmp;
                            std::string junk;
                            std::stringstream s(line);
                            s >> junk >> tmp.x >> tmp.y >> tmp.z;
                            nmls.push_back(tmp); //Inserting a new normal into a pile of others
                        }
                    }
                    //Fetching faces
                    else if (line[0] == 'f')
                    {
                        triangle tmp;
                        std::string junk;
                        int vert_data[3] = { 0 };
                        //This stuff will be supported later
                        int nml_data = 0;
                        int tex_data[3] = { 0 };
                        //Removing '/' symbols from string so it can be sliced properly
                        for (int i = 0; i < line.length(); i++)
                            if (line[i] == '/')
                                line[i] = ' ';
                        std::stringstream s(line);
                        s >> junk >> vert_data[0] >> tex_data[0] >> nml_data >> vert_data[1] >> tex_data[1] >> nml_data >> vert_data[2] >> tex_data[2] >> nml_data;
                        tmp.vertices[0] = verts.at(vert_data[0] - 1);
                        tmp.vertices[1] = verts.at(vert_data[1] - 1);
                        tmp.vertices[2] = verts.at(vert_data[2] - 1);
                        tmp.nml = nmls.at(nml_data - 1);
                        tris.push_back(tmp); //Inserting a new triangle into a pile of others
                    }
                }
                file.close();
                return 1;
            }
            return 0; //Oops. Something went wrong
        }
    };
    class Console
    {
        short width, height;
        std::string title;
        sf::RenderWindow window;
        HANDLE sobuf = GetStdHandle(STD_OUTPUT_HANDLE); //Standard output buffer(active console screen buffer)
        HANDLE sibuf = GetStdHandle(STD_INPUT_HANDLE); //Standard input buffer(active console input buffer)
        CHAR_INFO* dsp;
        double* depth_buffer;
        _SMALL_RECT coords = { 0, 0, 1, 1 };
    private:
        wchar_t* str2cwchr(std::string str)
        {
            wchar_t* s;
            s = new wchar_t[str.length()+1];            
            for (int i = 0; i < str.length(); i++)
                s[i] = str[i];
            s[str.length()] = '\0'; //Add a null pointer at the end so that we won't read any memory blocks that don't belong to the string
            return s;
        }
        double countTriArea(double x1, double y1, double x2, double y2, double x3, double y3)
        {
            if (y2 > y1 && y2 > y3)
                return (abs(x2 - x1) * 0.5 * (y1 + y2)) + (abs(x3 - x2) * 0.5 * (y3 + y2)) - (abs(x3 - x1) * 0.5 * (y1 + y3));
            return (abs(x3 - x1) * 0.5 * (y1 + y3)) - (abs(x2 - x1) * 0.5 * (y1 + y2)) - (abs(x3 - x2) * 0.5 * (y3 + y2));
        }
        int minimum(int a, int b)
        {
            if (a < b)
                return a;
            return b;
        }
        int maximum(int a, int b)
        {
            if (a > b)
                return a;
            return b;
        }
        //bool inTri(double coords[], double x, double y)
        //{
        //    for (int i = 0; i < 4; i += 2)
        //    {
        //        if (coords[i] > coords[i + 2])
        //        {
        //            std::swap(coords[i], coords[i + 2]);
        //            std::swap(coords[i + 1], coords[i + 3]);
        //        }
        //    }
        //    double a1 = countTriArea(coords[0], coords[1], coords[2], coords[3], coords[4], coords[5]);
        //    double a2 = countTriArea(coords[0], coords[1], x, y, coords[4], coords[5]);
        //    double a3 = countTriArea(coords[0], coords[1], coords[2], coords[3], x, y);
        //    double a4 = countTriArea(coords[0], coords[1], coords[2], coords[3], coords[4], coords[5]);
        //}
    public:
        Console(short a, short b, std::string c) //constructor
        {
            width = a;
            height = b;
            title = c;
        }
        int createConsole() //returns 0 if something went wrong, 1 if console has been created successfully
        {
            const wchar_t* newtitle = str2cwchr(title);
            SetConsoleWindowInfo(sobuf, TRUE, &coords); //setting a small window size first(idk why but it works)
            if (!SetConsoleScreenBufferSize(sobuf, { width, height })) //setting screen buffer size to be width*height
            {
                return 0;
            }
            coords.Right = width - 1;
            coords.Bottom = height - 1;
            SetConsoleWindowInfo(sobuf, TRUE, &coords); //setting a bigger window size
            if (!SetConsoleActiveScreenBuffer(sobuf)) //setting our modified screen buffer to the actual screen buffer
            {
                return 0;
            }
            if (!SetConsoleTitle(newtitle))
            {
                return 0;
            }
            dsp = new CHAR_INFO[width * height];
            depth_buffer = new double[width * height] {666};
            for (int i = 0; i < width * height; i++) //filling pointer with spaces so that nothing overlaps
            {
                dsp[i].Char.UnicodeChar = L' ';
                dsp[i].Attributes = 0x000F;
            }
         
            return 1;
        }
        int createWindow()
        {
            window.create(sf::VideoMode(width, height), title);
            return window.isOpen();
        }
        void closeWindow()
        {
            window.close();
        }
        void fillCell(short x, short y, wchar_t s = 0x2588, short c = 0x000F) //Fills a cell with some symbol. By default it's a fill symbol
        {
            window.draw(&vertex, 1, sf::Points);
            dsp[y * width + x].Char.UnicodeChar = s;
            dsp[y * width + x].Attributes = c;
        }
        void updateScreen(bool refresh = true) //disable refresh for precision
        {
            WriteConsoleOutput(sobuf, dsp, { width, height }, { 0, 0 }, &coords);
            if (refresh)
            {
                for (int i = 0; i < width * height; i++) //filling pointer with spaces so that nothing overlaps
                {
                    dsp[i].Char.UnicodeChar = L' ';
                    dsp[i].Attributes = 0x000F;
                }
            }
        }
        void clear_z_buffer()
        {
            for (int i = 0; i < width * height; i++)
                depth_buffer[i] = 666;
        }
        void drawLine(double x, double y, double x1, double y1, wchar_t s = 0x2588, short c = 0x000F) //Draws line on console. Uses linear interpolation.
        {
            double a, tmp_y, f;
            if (x != x1)
            {
                double b;
                a = (y1 - y) / (x1 - x);
                b = y - a * x;
                f = fabs(x - x1) / width;
                for (double n = minimum(x, x1); n <= maximum(x, x1); n += f)
                {
                    tmp_y = a * n + b;
                    fillCell(std::round(n), std::round(tmp_y), s, c);
                }
            }
            else
            {
                for (double n = minimum(y, y1); n <= maximum(y, y1); n++)
                {
                    fillCell(x, n, s, c);
                }
            }
        }
        void drawNgon(short *coord[], int n, wchar_t s = 0x2588, short c = 0x000F) //Connects points of ngon in the same order as they are specified
        {
            for (int i = 0; i < n+2; i += 2)
            {
                if (*coord[i] >= 0 && *coord[i + 1] >= 0 && *coord[i + 2] >= 0 && *coord[i + 3] >= 0)
                    drawLine(*coord[i], *coord[i + 1], *coord[i + 2], *coord[i + 3], s, c);
                else
                    break;
            }
            drawLine(*coord[0], *coord[1], *coord[n * 2 - 2], *coord[n * 2 - 1], s, c);
        }
        void fillTri(short coordinates[], wchar_t s = 0x2588, short c = 0x000F)
        {
            short* tmp[6];
            //Copying adress of each number to pointer
            for (int i = 0; i < 6; i++)
                tmp[i] = &coordinates[i];
            //Sorting verticies
            for (int j = 0; j < 2; j++)
            {
                for (int i = 1; i <= 3; i += 2)
                {
                    if (*tmp[i] > *tmp[i + 2])
                    {
                        short* t1 = tmp[i];
                        short* t2 = tmp[i - 1];
                        tmp[i] = tmp[i + 2];
                        tmp[i - 1] = tmp[i + 1];
                        tmp[i + 2] = t1;
                        tmp[i + 1] = t2;
                    }
                }
            }
            //Drawing the top part
            double increment_x = double(*tmp[2] - *tmp[0]) / double(*tmp[3] - *tmp[1]);
            double decrement_x = double(*tmp[4] - *tmp[0]) / double(*tmp[5] - *tmp[1]);
            for (int i = *tmp[1]; i <= *tmp[3]; i++)
            {
                drawLine(*tmp[0] + std::round(decrement_x * (i - *tmp[1])), i, std::round(*tmp[0] + increment_x * (i - *tmp[1])), i, s, c);
            }
            //Drawing the bottom part
            increment_x = double(*tmp[4] - *tmp[2]) / double(*tmp[5] - *tmp[3]);
            for (int i = *tmp[3] + 1; i <= *tmp[5]; i++)
            {
                drawLine(*tmp[0] + std::round(decrement_x * (i - *tmp[1])), i, std::round(*tmp[2] + increment_x * (i - *tmp[3])), i, s, c);
            }
        }
        void drawCircle(int h, int v, int r, wchar_t s = 0x2588, short c = 0x000F) //Draws circle. It actually looks more like an oval because of console cells' width/hight ratio
        {
            double y;
            for (double x = (double)h - (double)r; x <= (double)h + (double)r; x += 0.1) //Using the equation of a circle on coordinate space here
            {
                y = sqrt(r * r - (x - h) * (x - h)) + (double)v;
                fillCell((short)x, (short)y, s, c);
                fillCell((short)x, (short)v - ((short)y - v), s, c);
            }
            fillCell((short)h + r - 1, (short)v, s, c);
        }
        void drawSprite(short x, short y, double scale, std::string texture, short width, short height) //Draw sprite given coordinates of its center and scale
        {
            short color = 0;
            width = width * scale;
            height = height * scale;
            for (short i = y - short(height / 2); i < y + short(height / 2); i++)
            {
                for (short j = x - short(width / 2); j < x + short(width / 2); j++)
                {
                    if (texture[static_cast<int64_t>(std::round(i / scale)) * static_cast<int64_t>(width/scale) + static_cast<int64_t>(j / scale)] == 'b')
                        color = 12;
                    else if (texture[static_cast<int64_t>(std::round(i / scale)) * static_cast<int64_t>(width/scale) + static_cast<int64_t>(j / scale)] == 'a')
                        color = 14;
                    else if (texture[static_cast<int64_t>(std::round(i / scale)) * static_cast<int64_t>(width/scale) + static_cast<int64_t>(j / scale)] == 'r')
                        color = 4;
                    else if (texture[static_cast<int64_t>(std::round(i / scale)) * static_cast<int64_t>(width/scale) + static_cast<int64_t>(j / scale)] == 'g')
                        color = 2;
                    else
                        color = 7;
                    fillCell(j, i, 0x2588, color);
                }
            }
        }
        vec2d rotateLine(double x, double y, double x1, double y1, double ang)
        {
            vec2d points;
            points.x = x1;
            points.y = y1;
            if ((x != x1 || y != y1) && ang != 0)
            {
                double dist = sqrt((x1 - x) * (x1 - x) + (y1 - y) * (y1 - y)); //current hypothenuse
                double c_ang = acos((x1 - x) / dist);
                if (y > y1) c_ang += 2 * (3.14159 - c_ang);
                double rad_ang = ang * 3.14159265 / 180.0; //degrees to radians
                double adjacent = adjacent = dist * cos(c_ang + rad_ang);
                double opposite = adjacent * tan(c_ang + rad_ang);
                points.x = x + adjacent;
                points.y = y + opposite;
            }
            return points;
        }
        double* moveByAngle(double x, double y, double dist, double ang, double fov)
        {
            double rad_ang = ang * 3.14159265 / 180.0 + fov;
            double points[2] = {0.0, 0.0};
            points[0] = x - cos(rad_ang) * dist;
            points[1] = y - sin(rad_ang) * dist;
            return points;
        }
        double project(double a1, double b1, double a, double b, double l, double fov) //Project 2 dimensional point onto a line on that plane
        {
            double xprojected = (a - a1) / (tan(fov / 2) * (b - b1));
            double result = l * (a1 + xprojected) / 2;
            return result;
        }
        void drawTri3d(double fov, const double cam_coords[], triangle tri, wchar_t ch = 0x2588, short col = 0x000F) //Project 3d triangle onto the screen surface
        {
            short tmp[6] = {0};
            double z_coords[3] = { 0 }; //z for z buffer
            double tmp_x;
            double tmp_y;
            bool draw = true;
            for (int i = 0; i < 3; i++)
            {
                tmp_x = project(cam_coords[0], cam_coords[2], tri.vertices[i].x, tri.vertices[i].z, width, fov * 3.14159265 / 180.0);
                tmp_y = project(cam_coords[1], cam_coords[2], tri.vertices[i].y, tri.vertices[i].z, height, fov * 3.14159265 / 180.0);
                if (tmp_x < 0 || tmp_y < 0 || tmp_x > width || tmp_y > height) //If projected values exceed bounds, draw nothing
                {
                    draw = false;
                    break;
                }
                tmp[i * 2] = std::round(tmp_x);
                tmp[i * 2 + 1] = std::round(tmp_y);
                z_coords[i] = tri.vertices[i].z; //keep z
            }
            if (draw) //Drawing triangle
            {
                //Sorting verticies
                for (int j = 0; j < 2; j++)
                {
                    for (int i = 1; i <= 3; i += 2)
                    {
                        if (tmp[i] > tmp[i + 2])
                        {
                            std::swap(tmp[i], tmp[i + 2]);
                            std::swap(tmp[i - 1], tmp[i + 1]);
                            std::swap(z_coords[(i + 1) / 2 - 1], z_coords[(i + 1) / 2]);
                        }
                    }
                }
                //Drawing the top part
                double increment_x = double(tmp[2] - tmp[0]) / double(tmp[3] - tmp[1]);
                double decrement_x = double(tmp[4] - tmp[0]) / double(tmp[5] - tmp[1]);
                double increment_z = (z_coords[1] - z_coords[0]) / double(tmp[3] - tmp[1]);
                double decrement_z = (z_coords[2] - z_coords[0]) / double(tmp[5] - tmp[1]);
                for (int i = tmp[1]; i <= tmp[3]; i++)
                {
                    double x1 = std::round(tmp[0] + decrement_x * (i - tmp[1]));
                    double z2 = z_coords[0] + increment_z * (i - tmp[1]); 
                    double x2 = std::round(tmp[0] + increment_x * (i - tmp[1]));
                    double z1 = z_coords[0] + decrement_z * (i - tmp[1]);
                    if (x1 > x2)
                    {
                        std::swap(x1, x2);
                        std::swap(z1, z2);
                    }
                    double z_shift = (z2 - z1) / (x2 - x1); //z change due to change in y
                    for (double j = x1; j <= x2; j++)
                    {
                        double curr_z = z1 + z_shift * (j - x1);
                        if (curr_z < depth_buffer[int(j) + i * width]) //only fill pixel if it's closer than previous
                        {
                            fillCell(j, i, ch, col);
                            depth_buffer[int(j) + i * width] = curr_z; //update z buffer
                        }
                    }
                }
                //Drawing the bottom part
                increment_x = double(tmp[4] - tmp[2]) / double(tmp[5] - tmp[3]);
                increment_z = (z_coords[2] - z_coords[1]) / double(tmp[5] - tmp[3]);
                for (int i = tmp[3] + 1; i <= tmp[5]; i++)
                {
                    double x1 = std::round(tmp[0] + decrement_x * (i - tmp[1])); //
                    double z2 = z_coords[1] + increment_z * (i - tmp[3]);
                    double x2 = std::round(tmp[2] + increment_x * (i - tmp[3]));
                    double z1 = z_coords[0] + decrement_z * (i - tmp[1]);
                    if (x1 > x2)
                    {
                        std::swap(x1, x2);
                        std::swap(z1, z2);
                    }
                    double z_shift = (z2 - z1) / (x2 - x1); //z change due to change in y
                    for (double j = x1; j <= x2; j++)
                    {
                        double curr_z = z1 + z_shift * (j - x1);
                        if (curr_z < depth_buffer[int(j) + i * width])
                        {
                            fillCell(j, i, ch, col);
                            depth_buffer[int(j) + i * width] = curr_z;
                        }
                    }
                }
            }
        }
        double* findTriCenter(double coords[]) //Calculates incenter of a triangle given coordinates of vertices on a plane
        {
            double a, b, c, cx, cy;
            double* ret;
            a = sqrt((coords[0] - coords[2]) * (coords[0] - coords[2]) + (coords[1] - coords[3]) * (coords[1] - coords[3]));
            b = sqrt((coords[2] - coords[4]) * (coords[2] - coords[4]) + (coords[5] - coords[3]) * (coords[5] - coords[3]));
            c = sqrt((coords[4] - coords[0]) * (coords[4] - coords[0]) + (coords[5] - coords[1]) * (coords[5] - coords[1]));
            if (a == 0 || c == 0) //If one of the sides is 0, output center of the non zero side
            {
                cx = coords[2] + (coords[4] - coords[2]) / 2;
                cy = coords[3] + (coords[5] - coords[3]) / 2;
                ret = new double[2]{ cx, cy };
                return ret;
            }
            else if (b == 0)
            {
                cx = coords[0] + (coords[2] - coords[0]) / 2;
                cy = coords[1] + (coords[3] - coords[1]) / 2;
                ret = new double[2]{ cx, cy };
                return ret;
            }
            else
            {
                cx = (a * coords[4] + b * coords[0] + c * coords[2]) / (a + b + c);
                cy = (a * coords[5] + b * coords[1] + c * coords[3]) / (a + b + c);
                ret = new double[2]{ cx, cy };
                return ret;
            }
        }
        double* findTriCenter3d(double coords[])
        {
            double* tri1;
            double* tri2;
            double* ret;
            double* tmp1;
            double* tmp2;
            ret = new double[3];
            tri1 = new double[6]{ coords[0], coords[1], coords[3], coords[4], coords[6], coords[7] };
            tri2 = new double[6]{ coords[1], coords[2], coords[4], coords[5], coords[7], coords[8] };
            tmp1 = findTriCenter(tri1);
            tmp2 = findTriCenter(tri2);
            ret[0] = tmp1[0];
            ret[1] = tmp1[1];
            ret[2] = tmp2[1];
            delete[] tri1;
            delete[] tri2;
            delete[] tmp1;
            delete[] tmp2;
            return ret;
        }   
    };
}