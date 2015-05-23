/*
The goal of this little program is to get an understanding of how to 
implement a std linked-list for the bullet class in the main game.
The problem that is solved is how can we keep track of a variable
amount of bullet objects.
This should solve that because we can just keep adding objects to the 
back of the list and still access it pretty 
*/

#include <stdio.h>
#include <iostream>
#include <list>

using namespace std;

class Bullet
{
  public:
    int m_x;
    int m_y;
    void print()
    {
      cout << "The current x: " << m_x << endl;
    }
};

int main()
{
  list<Bullet*> Bullets;
  list<Bullet*>::iterator iter;
  
  Bullet *b = new Bullet();
  Bullet *c = new Bullet();
  b->m_x = 5;
  c->m_x = 8;
  Bullets.insert(Bullets.begin(), b);
  Bullets.insert(Bullets.begin(), c);
  
  for (int i = 0; i < 20; i++)
  {
  
  for (iter=Bullets.begin(); iter != Bullets.end(); iter++)
  {
    (*iter)->print();
    if ((*iter)->m_x > 10)
    {
      // remove the Bullet.
      iter = Bullets.erase(iter);
    }
    else
    {
      // increment the bullet.
      (*iter)->m_x += 1;
    }
  }
  }
  cout << "This works!" << endl;
  return 0;
}
