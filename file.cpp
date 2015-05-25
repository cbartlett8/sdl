/*
This is just a tester program to see all the potiential pitfalls
of using the stl for file operations and grabbing information 
out of text files.
*/
#include <iostream>
#include <fstream>    // for all our file operations.

class Actor
{
  public:
    int m_x;
    int m_y;
    Actor(int x)
    {
      m_x = x;
    }
};


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
    printf("ia: %i\n", ia);
    switch (ia)
    {
      case 0:
        printf("We have a zero.\n");
        break;
      default:
        printf("We don't have a zero.\n");
    }
    int b = ia * 5;
    printf("b: %i\n", b);
  }
  
  ifs.close();
  
  return 0;
}
