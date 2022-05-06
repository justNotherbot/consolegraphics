#include <iostream>
#include <math.h>
#include <string>
#include <fstream>
#include <windows.h>

namespace console
{
    class Console
    {
        short width, height;
        std::string title;
        HANDLE sobuf = GetStdHandle(STD_OUTPUT_HANDLE); //Standard output buffer(active console screen buffer)
        HANDLE sibuf = GetStdHandle(STD_INPUT_HANDLE); //Standard input buffer(active console input buffer)
        CHAR_INFO* dsp;
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
            for (int i = 0; i < width * height; i++) //filling pointer with spaces so that nothing overlaps
            {
                dsp[i].Char.UnicodeChar = L' ';
                dsp[i].Attributes = 0x000F;
            }
            return 1;
        }
        void fillCell(short x, short y, wchar_t s = 0x2588, short c = 0x000F) //Fills a cell with some symbol. By default it's a fill symbol
        {
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
        void drawLine(double x, double y, double x1, double y1, wchar_t s = 0x2588, short c = 0x000F) //Draws line on console. Uses linear interpolation.
        {
            double a, tmp_y, f;
            if (x != x1)
            {
                double b;
                a = (y1 - y) / (x1 - x);
                b = y - a * x;
                f = fabs(x - x1) * 0.01;
                for (double n = min(x, x1); n <= max(x, x1); n += f)
                {
                    tmp_y = a * n + b;
                    fillCell(std::round(n), std::round(tmp_y), s, c);
                }
            }
            else
            {
                for (double n = min(y, y1); n < max(y, y1); n++)
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
        double* rotateLine(double x, double y, double x1, double y1, double ang)
        {
            double dist = sqrt((x1 - x) * (x1 - x) + (y1 - y) * (y1 - y)); //current hypothenuse
            double c_ang = acos((x1 - x) / dist);
            if (y > y1) c_ang += 2*(3.14159 - c_ang);
            double rad_ang = ang * 3.14159265 / 180.0; //degrees to radians
            double adjacent = dist * cos(c_ang+rad_ang);
            double opposite = adjacent * tan(c_ang+rad_ang);
            double points[2] = {0, 0};
            points[0] = x + std::round(adjacent);
            points[1] = y + std::round(opposite);
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
            double xprojected = (l * (a - a1)) / (2 * tan(fov / 2) * (b - b1));
            double result = a1 + xprojected;
            return result;
        }
        void drawTri3d(double fov, double cam_coords[], double coords[]) //Project 3d triangle onto the screen surface
        {
            short* tmp;
            tmp = new short[6]; //Coordinates of projected triangle
            double tmp_x;
            double tmp_y;
            bool draw = true;
            for (int i = 0; i < 9; i += 3)
            {
                tmp_x = project(cam_coords[0], cam_coords[2], coords[i], coords[i + 2], width, fov * 3.14159265 / 180.0);
                tmp_y = project(cam_coords[1], cam_coords[2], coords[i + 1], coords[i + 2], height, fov * 3.14159265 / 180.0);
                if (tmp_x < 0 || tmp_y < 0 || tmp_x > width || tmp_y > height) //If projected values exceed bounds, draw nothing
                {
                    draw = false;
                    break;
                }
                tmp[(i / 3) * 2] = std::round(tmp_x);
                tmp[(i / 3) * 2+1] = std::round(tmp_y);
            }
            if (draw) //Drawing triangle
            {
                drawLine(short(tmp[0]), short(tmp[1]), short(tmp[2]), short(tmp[3]));
                drawLine(short(tmp[2]), short(tmp[3]), short(tmp[4]), short(tmp[5]));
                drawLine(short(tmp[4]), short(tmp[5]), short(tmp[0]), short(tmp[1]));
            }
            delete[] tmp;
        }
        void releaseMemory() //call after the main loop/thread quits
        {
            delete[] dsp;
        }        
    };
}
