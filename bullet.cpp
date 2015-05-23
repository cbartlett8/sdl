#include "bullet.h"

Bullet::Bullet(int x, int y, int velocity)
{
  m_x = x;
  m_y = y;
  m_velocity = velocity;
}

void Bullet::Update()
{
  m_x += velocity;
}

int Bullet::Get_x()
{
  return m_x;
}

int Bullet::Get_y()
{
  return m_y;
}

void Bullet::Set_x(int x)
{
  m_x = x;
}

void Bulet::Set_y(int y)
{
  m_y = y;
}

// We generate an bullet object and insert into the list.
void Bullets::Generate(int x, int y, int velocity)
{
  Bullet *temp = new Bullet(x, y, velocity);
  Bullet_list.insert(Bullet_list.begin(), temp);
}
