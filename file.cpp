/*
This is just a tester program to see all the potiential pitfalls
of using the stl for file operations and grabbing information 
out of text files.
*/

#include <fstream>    // for all our file operations.

int main()
{
  std::ifstream ifs;
  
  ifs.open("test.txt");
  
  ifs.close();
  
  return 0;
}
