#include <SoftwareSerial.h>
#include <EEPROM.h>

SoftwareSerial disp(0, 9);

struct Button
{
  int pin;
  
  int debounce_time;
  int debounce_delay;

  /*
   * true: return true once when it turns on
   * false: return true once when it changes value
   */
  
  boolean want_on;

  boolean last_value;
  boolean last_good_value;
  boolean has_changed;

};

struct Button done = { 8, 0, 50, true, false, false, false };
struct Button timer = { 13, 0, 50, false, false, false, false };




int debounce_delay = 50;
int disp_delay = 10;

int dbg_led = 13;

int task_cnt = 34;
int task_loc = 99;

char * tasks[35] = {
/*
  "                \n              ",
*/
  "Wipe front of   \nappliances    ",
  "Trash can       \n              ",
  "Tidy books,     \nlinens        ",
  "Organize the    \nfront closet  ",
  "Fridge, Freezer \nwipe & tidy   ",
  "Wipe cabinets   \n(front)       ",
  "Mop floor       \n              ",
  
  "Wipe front of   \nappliances    ",
  "Utensil holders \nFridge top    ",
  "Fridge, Stove   \n(under)       ",
  "Seals: disposal/\nfridge/washer ",
  "Windows, and    \nSills         ",
  "Wipe decorations\n              ",
  "Mop floor       \n              ",
  
  "Wipe front of   \nappliances    ",
  "Tidy cabinets   \n              ",
  "Cycle cloths    \nsponge        ",
  "Cabinet         \ninteriors     ",
  "Wipe walls      \n              ",
  "Wipe cabinets   \n(front)       ",
  "Mop floor       \n              ",
  
  "Wipe front of   \nappliances    ",
  "Organize pots   \npans, drawers ",
  "Scrub sink      \nwipe faucet   ",
  "Fridge, Freezer \nwipe & tidy   ",
  "Clean microwave \nOven->Clean   ",
  "Wipe            \nbaseboards    ",
  "Mop floor       \n              ",
  
  "Wipe front of   \nappliances    ",
  "Dust: switches, \nlights, vents ",
  "Wipe down bike  \nsmears        ",
  "Clean hallway   \nand stoop     ",
  "Clean the mantel\n              ",
  "Organize the    \nback closet   ",
  "Mop floor       \n              "
};

boolean timer_enabled = false;
int timer_pos   = 0;
int timer_ticks = 0;

int timer_pin   = 11;
int timer_num   = 4;
int timer_len   = 3000;

boolean screen_is_on = true;

void setup()
{
  pinMode(done.pin, INPUT);
  pinMode(dbg_led, OUTPUT);
  pinMode(timer_pin, OUTPUT);
  disp.begin(9600);
  delay(10);
  screen(false);
}

void loop()
{
  if (false) {
    // For sampling the values
    screen(true);
    for (int i = 0; i <= task_cnt; i++) {
      disp_write_task(i);
      delay(500);
    }
    screen(false);
  
    return;
  }
  
  
  boolean update_disp = false;
  
  if (task_loc > task_cnt) {
    int eeloc = EEPROM.read(0);
    if (eeloc > task_cnt) {
      eeloc = 0;
    }
    
    task_loc = eeloc;
    
    update_disp = true;
  }
  
  if (read_btn(&done)) {

    update_disp = true;
    if (screen_is_on) {
      task_loc++;
      screen(false);
    } else {
      screen(true);
    }

    timer_end();
    
  }
  
  if (update_disp) {
    disp_write_task(task_loc);

    EEPROM.write(0, task_loc);
  }
  
  if (read_btn(&timer)) {
    screen(true);
    timer_press();
  }
  
  timer_tick();
  
  delay(100);
}

void screen(boolean enable)
{
  if (enable) {
    screen_is_on = true;
    backlightOn();
  } else {
    screen_is_on = false;
    backlightOff();
  }
    
}

void timer_end()
{
  timer_enabled = false;
  timer_alert();
  delay(100);
  analogWrite(timer_pin, 0);
  delay(100);
  timer_alert();
  delay(100);
  analogWrite(timer_pin, 0);
}

void timer_press()
{
  timer_alert();
  delay(100);
  analogWrite(timer_pin, 0);
  delay(100);
  timer_alert();
  delay(100);
  analogWrite(timer_pin, 0);
  

  timer_ticks = 0;

  if (!timer_enabled) {
    timer_enabled = true;
    timer_pos = 0;
  }
  
  if (timer_pos++ > timer_num) {
    timer_enabled = false;
  }

  analogWrite(timer_pin, 0);
}

void timer_tick()
{
  if (!timer_enabled) {
    return;
  }

  if (timer_ticks++ > timer_len) {
    timer_alert();
  }
}

void timer_alert()
{
  analogWrite(timer_pin, 249);
}

boolean read_btn(struct Button *btn)
{
  int val = digitalRead(btn->pin);
  
  
  
  if (val != btn->last_value) {
    btn->debounce_time = millis();
  }
  
  btn->last_value = val;
  
  if ((millis() - btn->debounce_time) < btn->debounce_delay) {
    return false;
  }
  
  if (val != btn->last_good_value) {
    // Note that the switch HAS gone low
    // Reset the "saw press" val since any press after
    // a debounced LOW means it hasn't been seen again
    btn->has_changed = true;
  }

  btn->last_good_value = val;

  if (btn->has_changed) {
    btn->has_changed = false;
    
    if (btn->want_on) {
      return btn->last_good_value;
    } else {
      return true;
    }
  }
  
  return false;
}

void disp_write_task(int loc) {
  disp_write(tasks[loc]);
  
  int dow = loc + 1;
  if (dow > 9) {
    goTo(1, 14);
  } else {
    goTo(1, 15);
  }
  
  disp.print(dow);
  delay(disp_delay);
  goTo(1, 16);
}

void disp_write(char * msg) {
  int wide = 16;
  int height = 2;
  
  int wpos = 0;
  int hpos = 0;
  
  clearLCD();
  for (int i = 0; i < strlen(msg); i++) {
    if (msg[i] == '\n') {
      hpos++;
      wpos = 0;
      continue;
    }
    goTo(hpos, wpos);
    disp.print(msg[i]);
    wpos++;
  }
}

void goTo(int row, int col) {
  disp.write(0xFE);   //command flag
  disp.write((col + row*64 + 128));    //position 
  delay(disp_delay);
}
void clearLCD(){
  goTo(0, 0);
  disp.print("                ");
  goTo(1, 0);
  disp.print("                ");
  
  disp.write(0xFE);   //command flag
  disp.write(0x01);   //clear command.
  delay(disp_delay);
}
void backlightOn() {  //turns on the backlight
  disp.write(0x7C);   //command flag for backlight stuff
  disp.write(157);    //light level.
  delay(disp_delay);
}
void backlightOff(){  //turns off the backlight
  disp.write(0x7C);   //command flag for backlight stuff
  disp.write(128);     //light level for off.
  delay(disp_delay);
}

