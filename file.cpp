/*
This is just a tester program to see all the potiential pitfalls
of using the stl for file operations and grabbing information 
out of text files.
*/
#include <string>
#include <iostream>
#include <fstream>    // for all our file operations.
struct Coords
{
  int m_x;
  int m_y;
  int m_w;
  int m_h;
  int m_max_frames;
  
  Coords(int x, int y, int w, int h, int max_frame)
  {
    m_x = x;
    m_y = y;
    m_w = w;
    m_h = h;
    m_max_frames = max_frame;
  }
  void print()
  {
    //std::cout << "x: " << m_x << " y: " << m_y << " w: " << m_w << " h: " << m_h << " m_f: " << m_max_frames << std::endl;
    printf("x: %i y: %i w: %i h: %i m_f: %i\n", m_x, m_y, m_w, m_h, m_max_frames);
  }
};

class Actor
{
  public:
    int m_x;
    int m_y;
    std::string run;
    std::string hit;
    Coords *coords_run;
    Coords *coords_hit;
    
    Actor(int x)
    {
      m_x = x;
    }
    void set_run(std::string temp)
    {
      run = temp;
    }
    void set_hit(std::string temp)
    {
      hit = temp;
    }
    void set_coords_run(int x, int y, int w, int h, int max_frame)
    {
      coords_run = new Coords(x,y,w,h,max_frame);
    }
    void set_coords_hit(int x, int y, int w, int h, int max_frame)
    {
      coords_hit = new Coords(x, y, w, h, max_frame);
    }
    void print()
    {
      std::cout << "Actor is holding: " << std::endl;
      if (run != "")
      {
        std::cout << "run: " << run << std::endl;
        coords_run->print();
      }
      if (hit != "")
      {
        std::cout << " hit: " << hit << std::endl;
        coords_hit->print();
      }
    }
};


int Convert(char c)
{
  std::cout << "Convert c got: " << c << std::endl;
  switch(c)
  {
    case '0':
      return 0;
    case '1':
      return 1;
    case '2':
      return 2;
    case '3':
      return 3;
    case '4':
      return 4;
    case '5':
      return 5;
    case '6':
      return 6;
    case '7':
      return 7;
    case '8':
      return 8;
    case '9':
      return 9;
    default:
      return -1;
  }
}

int main()
{
  std::ifstream ifs;
  
  ifs.open("test.txt");
  
  char c[256];
  
  // Every line we are taking in is a description of what the
  // object we want to display is.
  while(ifs.getline(c, 256))
  {
    std::cout << c;
    
    // We grab the first character to dictate the type of 
    // Actor we are making.
    Actor *temp = new Actor(4);
    int ia = c[0] - '0';
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int max_frames = 0;
    printf("Version: %i\n", ia);
    switch (ia)
    {
      case 3:   // This is the level for the enemies.
        printf("We have a three.\n");
        break;
        
      case 2:  // This is the level for the coins and collectibles.
        // Basically here just for hit animation.
        ifs.getline(c, 256);
        int x_1, x_2, x_3;

        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        x = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("x: %i\n", x);
        
        ifs.getline(c, 256);
        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        y = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("y: %i\n", y);
        
        ifs.getline(c, 256);
        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        w = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("w: %i\n", w);
        
        ifs.getline(c, 256);
        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        h = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("h: %i\n", h);
        
        ifs.getline(c, 256);
        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        max_frames = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("max_frames: %i\n", max_frames);
        
        temp->set_coords_hit(x, y, w, h, max_frames);
        
        // surface is the file
        ifs.getline(c, 256);
        temp->set_hit(c);
        
        temp->print();
      
      case 1:  // This is the level for like platforms.
      {
        // So this level takes in: x, y, w, h, Surface_name
        ifs.getline(c, 256);
        int x_1, x_2, x_3;

        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        x = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("x: %i\n", x);
        
        ifs.getline(c, 256);
        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        y = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("y: %i\n", y);
        
        ifs.getline(c, 256);
        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        w = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("w: %i\n", w);
        
        ifs.getline(c, 256);
        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        h = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("h: %i\n", h);
        
        ifs.getline(c, 256);
        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        max_frames = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("max_frames: %i\n", max_frames);
        
        temp->set_coords_run(x, y, w, h, max_frames);
        
        // surface is the file
        ifs.getline(c, 256);
        temp->set_run(c);
        
        temp->print();
        
        ifs.getline(c, 256);
        char exit_statement = 'E';
        printf("This is c: %s\n", c);
        if (c[0] == exit_statement)
        {
          printf("We correctly identified the EXIT Statement\n");
        }
        //y = convert(c[])
        break;
      }
      default:
        printf("We don't have a zero.\n");
    }
    int b = ia * 5;
    printf("b: %i\n", b);
  }
  
  ifs.close();
  
  return 0;
}
