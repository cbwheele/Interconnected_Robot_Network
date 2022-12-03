#include <Arduino.h>

struct MyLink
{
    uint16_t anchor_addr;
    float range[3];
    float dbm;
    struct MyLink *next;
};

class Coordinates
{
public:
  Coordinates() {
    x = 0.0;
    y = 0.0;
  }
  
  Coordinates(float x_cord, float y_cord) {
    x = x_cord;
    y = y_cord;
  }
  
  float x;
  float y;
  
};

struct MyLink *init_link();
void add_link(struct MyLink *p, uint16_t addr);
struct MyLink *find_link(struct MyLink *p, uint16_t addr);
void fresh_link(struct MyLink *p, uint16_t addr, float range, float dbm);
void print_link(struct MyLink *p);
void delete_link(struct MyLink *p, uint16_t addr);
void make_link_json(struct MyLink *p,String *s);

Coordinates getCoordinates(struct MyLink *p);