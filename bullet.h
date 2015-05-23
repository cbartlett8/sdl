#ifndef BULLET_H
#define BULLET_H

#include <iostream>
#include <list>


// Class bullet is going to have a couple of member functions
// that will assist in getting the setting the m_x and m_y.
// Idealily I would want to call update which would
// update the m_x and m_y and then get the 
// Get_x and Get_y as the new position.
class Bullet
{
  private:
    int m_x;
    int m_y;
    int m_velocity;
  public:
    Bullet(int x, int y, int velocity);
    void Update();
    int Get_x();
    int Get_y();
    void Set_x();
    void Set_y();
};

// Then we can put a class named bullets that is a 
// singleton like bullets and it would hold a list
// of all the active bullets.
// When the player or enemy fires a bullet I would want
// to call the generate function which should make a new
// bullet object and insert it into the list. I should
// pass it the x, y, and velocity of what the bullet 
// should have.
// So when the bullet leaves the visible playfield I want
// remove() it. So this just takes it out of the list.
// Get bullet just gets the current bullet
class Bullets
{ 
  private:
    std::list<Bullet*> Bullet_list;
    std::list<Bullet*>::iterator iter;
  public: 
    void Generate(int x, int y, int velocity);
    bullet* GetBullet();
    bool Remove();

  Symbol_table() {m_id = global++;}
  
  // disabling default copy constructor and the default
  // assignment done as a precaution.
  Bullets(const Bullets &);
  const Bullets &operator=(const Bullets &);
};

#endif
