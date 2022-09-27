#include "link.h"

//#define SERIAL_DEBUG

#include <cmath>

struct MyLink *init_link()
{
#ifdef SERIAL_DEBUG
    Serial.println("init_link");
#endif
    struct MyLink *p = (struct MyLink *)malloc(sizeof(struct MyLink));
    p->next = NULL;
    p->anchor_addr = 0;
    p->range[0] = 0.0;
    p->range[1] = 0.0;
    p->range[2] = 0.0;

    return p;
}

Coordinates getCoordinates(struct MyLink *p) {

    Serial.println("This is inside of getCoordinates function");
    struct MyLink *temp = p;

    float firstReading = -1;
    float secondReading = -1;

    while (temp->next != NULL)
    {
        temp = temp->next;
        char link_json[50];
        Serial.println(temp->anchor_addr);
        Serial.println(temp->range[0]);

        if (firstReading == -1) {
          firstReading = temp->range[0];
        } else {
          secondReading = temp->range[0];
        }
        //*s += link_json;
        if (temp->next != NULL)
        {
            //*s += ",";
        }
    }


    Coordinates returnCoordinates = Coordinates(0.0, 0.0);

    
    if (firstReading != -1 && secondReading != -1) {
    // Here I have both firstReading and secondReading as the distance to each of the two anchors:

      Serial.print("Two distances: ");
      Serial.print(firstReading);
      Serial.print(", ");
      Serial.println(secondReading);


      // Do math to create coordinates from that:
      // a is first measurement
      // b is second measurement
      // c is distance between anchors
      float a = firstReading;
      float b = secondReading;
      float c = 3.0;
      
      float cos_a = (b * b + c*c - a * a) / (2 * b * c);
      float x = b * cos_a;
      float y = b * sqrt(1 - cos_a * cos_a);

      // Set the coordinates inside of the returnCoordinates objects
      returnCoordinates.x = x;
      returnCoordinates.y = y;

      Serial.print("Calculated coordinates: ");
      Serial.print(x);
      Serial.print(", ");
      Serial.println(y);

      // Send the data over UART to the MSP432
      
    }


    return returnCoordinates;
}

void add_link(struct MyLink *p, uint16_t addr)
{
#ifdef SERIAL_DEBUG
    Serial.println("add_link");
#endif
    struct MyLink *temp = p;
    //Find struct MyLink end
    while (temp->next != NULL)
    {
        temp = temp->next;
    }

    Serial.println("add_link:find struct MyLink end");
    //Create a anchor
    struct MyLink *a = (struct MyLink *)malloc(sizeof(struct MyLink));
    a->anchor_addr = addr;
    a->range[0] = 0.0;
    a->range[1] = 0.0;
    a->range[2] = 0.0;
    a->dbm = 0.0;
    a->next = NULL;

    //Add anchor to end of struct MyLink
    temp->next = a;

    return;
}

struct MyLink *find_link(struct MyLink *p, uint16_t addr)
{
#ifdef SERIAL_DEBUG
    Serial.println("find_link");
#endif
    if (addr == 0)
    {
        Serial.println("find_link:Input addr is 0");
        return NULL;
    }

    if (p->next == NULL)
    {
        Serial.println("find_link:Link is empty");
        return NULL;
    }

    struct MyLink *temp = p;
    //Find target struct MyLink or struct MyLink end
    while (temp->next != NULL)
    {
        temp = temp->next;
        if (temp->anchor_addr == addr)
        {
            // Serial.println("find_link:Find addr");
            return temp;
        }
    }

    Serial.println("find_link:Can't find addr");
    return NULL;
}

void fresh_link(struct MyLink *p, uint16_t addr, float range, float dbm)
{
#ifdef SERIAL_DEBUG
    Serial.println("fresh_link");
#endif
    struct MyLink *temp = find_link(p, addr);
    if (temp != NULL)
    {
        temp->range[2] = temp->range[1];
        temp->range[1] = temp->range[0];

        temp->range[0] = (range + temp->range[1] + temp->range[2]) / 3;
        temp->dbm = dbm;
        return;
    }
    else
    {
        Serial.println("fresh_link:Fresh fail");
        return;
    }
}

void print_link(struct MyLink *p)
{
#ifdef SERIAL_DEBUG
    Serial.println("print_link");
#endif
    struct MyLink *temp = p;

    while (temp->next != NULL)
    {
        //Serial.println("Dev %d:%d m", temp->next->anchor_addr, temp->next->range);
        Serial.println(temp->next->anchor_addr, HEX);
        Serial.println(temp->next->range[0]);
        Serial.println(temp->next->dbm);
        temp = temp->next;
    }

    return;
}

void delete_link(struct MyLink *p, uint16_t addr)
{
#ifdef SERIAL_DEBUG
    Serial.println("delete_link");
#endif
    if (addr == 0)
        return;

    struct MyLink *temp = p;
    while (temp->next != NULL)
    {
        if (temp->next->anchor_addr == addr)
        {
            struct MyLink *del = temp->next;
            temp->next = del->next;
            free(del);
            return;
        }
        temp = temp->next;
    }
    return;
}

void make_link_json(struct MyLink *p, String *s)
{
#ifdef SERIAL_DEBUG
    Serial.println("make_link_json");
#endif
    *s = "{\"links\":[";
    struct MyLink *temp = p;

    while (temp->next != NULL)
    {
        temp = temp->next;
        char link_json[50];
        sprintf(link_json, "{\"A\":\"%X\",\"R\":\"%.1f\"}", temp->anchor_addr, temp->range[0]);
        *s += link_json;
        if (temp->next != NULL)
        {
            *s += ",";
        }
    }
    *s += "]}";
    Serial.println(*s);
}
