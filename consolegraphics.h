#include <iostream>
#include <math.h>
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
            {
                s[i] = str[i];
            }
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
            for (int i = 0; i < width * height; i++)
                dsp[i].Char.UnicodeChar = ' ';
            return 1;
        }
        void fillCell(short x, short y, wchar_t s = 0x2588, short c = 0x000F) //Fills a cell with some symbol. By default it's a fill symbol
        {
            dsp[y * width + x].Char.UnicodeChar = s;
            dsp[y * width + x].Attributes = c;
        }
        void updateScreen() //test
        {
            WriteConsoleOutput(sobuf, dsp, { width, height }, { 0, 0 }, &coords);
            for (int i = 0; i < width * height; i++) //filling pointer with spaces so that nothing overlaps
            {
                dsp[i].Char.UnicodeChar = L' ';
                dsp[i].Attributes = 0x000F;
            }                
        }
        void drawLine(double x, double y, double x1, double y1, wchar_t s = 0x2588, short c = 0x000F) //Draws line on console. Uses linear equation principle.
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
        void drawNgon(short coord[], int n, wchar_t s = 0x2588, short c = 0x000F) //Connects points of ngon in the same order as they are specified
        {
            for (int i = 0; i < n+2; i += 2)
            {
                drawLine(coord[i], coord[i + 1], coord[i + 2], coord[i + 3], s, c);
            }
            drawLine(coord[0], coord[1], coord[n * 2 - 2], coord[n * 2 - 1], s, c);
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
        void releaseMemory() //call after the main loop/thread quits
        {
            delete[] dsp;
        }        
    };
}
