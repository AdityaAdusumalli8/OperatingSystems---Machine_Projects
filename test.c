// test.c - Skyline test driver
//

#include "console.h"
#include "string.h"
#include "skyline.h"

struct skyline_star skyline_stars[SKYLINE_STARS_MAX];
uint16_t skyline_star_cnt;
struct skyline_beacon skyline_beacon;
struct skyline_window * skyline_win_list;

void main(void){
   // skyline_star_cnt = 0;
   // # add_star(0,0,(uint16_t)(0xaabb));
   // # add_star(20,20,(uint16_t)(0xaabc));
   // # remove_star(0,0);

   console_printf("star color: %x\n ", skyline_win_list);
   add_window(0,0,10,10, 0xbeef);
   console_printf("star color: %x\n ", skyline_win_list-> color);
}

