/***************** EXTERNAL LIBRARIES ************************/
#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/flash.h"	  // Use for load and save flash
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/gpio.h"    // Use for GPIO
#include "jsmn.h"             // Use for JSON parsing
#include <string.h>           // Use for string
#include "hardware/i2c.h"     // Use for I2C Display 16x2
#include "pico/binary_info.h" // Use for I2C Display 16x2
#include "hardware/rtc.h"     // Use for rtc
#include "pico/util/datetime.h"
#include "hardware/adc.h"     // Use for sensor measurement

/******************* DEFINE LIST ************************/

// DEBUG printf
#define DEBUG           false
#define LOW             0
#define HIGH            1
// FLASH
#define FLASH_TARGET_OFFSET (256 * 1024)

// JSON
#define BUFFER_LENGTH 500

// GPIO
#define GPIO_ENCODER_A  21
#define GPIO_ENCODER_B  20
#define GPIO_UP         25
#define GPIO_DOWN       24
#define GPIO_SET        16
#define GPIO_BACK       17
#define GPIO_SENSOR_A   27
#define GPIO_SENSOR_B   26
#define GPIO_HEATER     10
#define GPIO_LED        28
#define GPIO_LED_RP     25
#define GPIO_LCD_SDA    0
#define GPIO_LCD_SCL    1

#define GPIO_LED_RP 25

// LCD - Modes for lcd_send_byte
#define LCD_CHARACTER  1
#define LCD_COMMAND    0
#define MAX_LINES      2
#define MAX_CHARS      16

// LCD - Position custom haracters 
#define Xtemperatura  11
#define Ytemperatura  1
#define Xtiempo       2
#define Ytiempo       1
#define Xfuego        7
#define Yfuego        1

#define CYCLES        5



/******************* GLOBAL VARIANT ************************/
// LCD - commands
const int LCD_CLEARDISPLAY = 0x01;
const int LCD_RETURNHOME = 0x02;
const int LCD_ENTRYMODESET = 0x04;
const int LCD_DISPLAYCONTROL = 0x08;
const int LCD_CURSORSHIFT = 0x10;
const int LCD_FUNCTIONSET = 0x20;
const int LCD_SETCGRAMADDR = 0x40;
const int LCD_SETDDRAMADDR = 0x80;

// LCD - flags for display entry mode
const int LCD_ENTRYSHIFTINCREMENT = 0x01;
const int LCD_ENTRYLEFT = 0x02;

// LCD - flags for display and cursor control
const int LCD_BLINKON = 0x01;
const int LCD_CURSORON = 0x02;
const int LCD_DISPLAYON = 0x04;

// LCD - flags for display and cursor shift
const int LCD_MOVERIGHT = 0x04;
const int LCD_DISPLAYMOVE = 0x08;

// LCD - flags for function set
const int LCD_5x10DOTS = 0x04;
const int LCD_2LINE = 0x08;
const int LCD_8BITMODE = 0x10;

// LCD - flag for backlight control
const int LCD_BACKLIGHT = 0x08;

const int LCD_ENABLE_BIT = 0x04;

// LCD - By default these LCD display drivers are on bus address 0x27
static int addr = 0x27;

// LCD - Custom character
const char grado[8]={ // char 0
    0b00011000,
    0b00011000,
    0b00000011,
    0b00000100,
    0b00000100,
    0b00000100,
    0b00000011,
    0b00000000
};

const char izq[8]={ // char 1
    0b00000011,
    0b00000111,
    0b00001111,
    0b00011111,
    0b00001111,
    0b00000111,
    0b00000011,
    0b00000000
    };

const char der[8]={ // char 2
    0b00011000,
    0b00011100,
    0b00011110,
    0b00011111,
    0b00011110,
    0b00011100,
    0b00011000,
    0b00000000
    };

const char simboloCalor1[8]={// char 3
    0b00001001,    
    0b00001001,    
    0b00010010,    
    0b00010010,    
    0b00001001,    
    0b00001001,    
    0b00011111,    
    0b00011111
    };

const char simboloCalor2[8]={// char 4
    0b00010010,    
    0b00010010,    
    0b00001001,    
    0b00001001,    
    0b00010010,    
    0b00010010,    
    0b00011111,    
    0b00011111
    };

const char enie[8]={  
    0b00011110,    
    0b00000000,    
    0b00011110,    
    0b00010001,    
    0b00010001,    
    0b00010001,    
    0b00010001,    
    0b00000000
    };
                

// FLASH
const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);

// Interface
const char *msg[16];

// RTC
char datetime_buf[256];
char *datetime_str = &datetime_buf[0];

// Start on Friday 5th of June 2020 15:45:00
datetime_t t = {
    .year  = 2020,
    .month = 06,
    .day   = 05,
    .dotw  = 5, // 0 is Sunday, so 5 is Friday
    .hour  = 22,
    .min   = 11,
    .sec   = 20
};

// Control
int msj = 10;
int menu = 0;
int ciclo = 1; 

float tempActual = 0;

uint16_t cycleLife[] = {0,0,0,0,0,0};
uint8_t temperature[] = {0,0,0,0,0,0,0};
uint8_t cycles = 0;
uint8_t offsetA = 0;
uint8_t offsetB = 0;
uint8_t slopeA = 0;
uint8_t slopeB = 0;

/******************* FUNCTIONS LISTS ************************/
uint16_t read_serial(uint8_t *buffer);
static int jsoneq(const char *json, jsmntok_t *tok, const char *s);
int parse(char *input);
void i2c_write_byte(uint8_t val);
void lcd_toggle_enable(uint8_t val);
void lcd_send_byte(uint8_t val, int mode);
void lcd_clear(void);
void lcd_set_cursor(int line, int position);
static void inline lcd_char(char val);
void lcd_string(const char *s);
void lcd_init(void);
void lcd_create_symbol(const char *input);
void msg_init(void);
void i2c_lcd_init(void);
void gpio_init_all(void);
void save_flash(void);
void load_flash(void);
void lcd_string_temperature(float temperatureIn, int x, int y);
void lcd_string_time(uint16_t time , int x, int y);

/******************* MAIN PROGRAM ************************/
int main(void){
 	stdio_init_all();
 	gpio_init_all();
  i2c_lcd_init();
  lcd_init();
  msg_init();
  rtc_init();
  rtc_set_datetime(&t);
  sleep_us(64); // Compensation for response
  load_flash();
  lcd_set_cursor(1,1);
  lcd_char(0);
  lcd_char(1);
  lcd_char(2);
  lcd_char(3);
  lcd_string_temperature(20.523, 0, 0);
  sleep_ms(5000);
  lcd_string_time( 491, 0, 0);
  sleep_ms(5000);
	while(true){
		char buffer_Serial[1024];
		uint16_t buffer_length = read_serial(buffer_Serial);

		if( DEBUG ){
      printf("Buffer length: %d\n", buffer_length);
		  printf("Salida: %s", buffer_Serial);
    }
		gpio_put(GPIO_LED_RP,HIGH);
    lcd_set_cursor(1,0);
    rtc_get_datetime(&t);
    
    datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
    char msg_out[10] = "";
    sprintf(msg_out, "%d:%d:%d", t.hour, t.min, t.sec);  
    printf("%s\n",msg_out);
    lcd_string(msg_out);
		sleep_ms(1000);
		gpio_put(GPIO_LED_RP,LOW);
		sleep_ms(1000);

    if (! gpio_get(GPIO_SET)){
      parse(buffer_Serial);
    }

	}
	return 0;
}



/******************* FUNCTIONS  ************************/
uint16_t read_serial(uint8_t *buffer){
  uint16_t buffer_index= 0;
  while (true) {
    int c = getchar_timeout_us(100);
    if (c != PICO_ERROR_TIMEOUT && buffer_index < BUFFER_LENGTH) {
      buffer[buffer_index++] = (c & 0xFF);
    } else {
	  buffer[buffer_index++] = '\0';
      break;
    }
  }
  return buffer_index;
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

int parse(char *input) {
  int i;
  int r;
  jsmn_parser p;
  jsmntok_t t[128]; /* We expect no more than 128 tokens */

  jsmn_init(&p);
  r = jsmn_parse(&p, input, strlen(input), t, sizeof(t) / sizeof(t[0]));
  if (r < 0) {
    printf("Failed to parse JSON: %d\n", r);
    return 1;
  }

  if ( DEBUG ) printf("\n\n\nValor de r: %d\n\n\n\n", r);

  /* Assume the top-level element is an object */
  if (r < 1 || t[0].type != JSMN_OBJECT) {
     if ( DEBUG ) printf("Object expected\n");
    return 1;
  }

  /* Loop over all keys of the root object */
  for (i = 1; i < r; i++) {
    if (jsoneq(input, &t[i], "action") == 0) {
      /* We may use strndup() to fetch string value */
      int temp_length = t[i+1].end - t[i+1].start;
      char temp[temp_length + 1];
      memcpy(temp, &input[t[i+1].start], temp_length); 
      temp[temp_length] = '\0';
      printf("\n\n%s\n\n",temp);
      lcd_set_cursor(0,0);
      lcd_string(temp);
      sleep_ms(1000);
      //printf("- action: %.*s\n", t[i + 1].end - t[i + 1].start,
      //      input + t[i + 1].start);
      i++;
    } else if (jsoneq(input, &t[i], "bot") == 0) {
      /* We may additionally check if the value is either "true" or "false" */
      printf("- bot: %.*s\n", t[i + 1].end - t[i + 1].start,
             input + t[i + 1].start);
      i++;
    } else if (jsoneq(input, &t[i], "top") == 0) {
      /* We may want to do strtol() here to get numeric value */
      printf("- top: %.*s\n", t[i + 1].end - t[i + 1].start,
             input + t[i + 1].start);
      i++;
    } else if (jsoneq(input, &t[i], "parameters") == 0) {
      int j;
      printf("- parameters:\n");

      if (t[i+1].type == JSMN_OBJECT){
        if (jsoneq(input, &t[i+2], "top") == 0) 
        printf("-parameters - top: %.*s\n",t[i + 3].end - t[i + 3].start,
             input + t[i + 3].start);
      }

      /*
      if (t[i + 1].type != JSMN_OBJECT) {
        continue; // Skip the next parse
      }

    
      for (j = 0; j < t[i + 1].size; j++) {
        jsmntok_t *g = &t[i + j + 2];
        printf("  * %.*s\n", g->end - g->start, input + g->start);
      }*/
      
      i += t[i + 1].size + 1;
    } else {
      printf("Unexpected key: %.*s\n", t[i].end - t[i].start,
             input + t[i].start);
    }
  }
  return 0;
}

/* Quick helper function for single byte transfers */
void i2c_write_byte(uint8_t val) {
#ifdef i2c_default
    i2c_write_blocking(i2c_default, addr, &val, 1, false);
#endif
}

void lcd_toggle_enable(uint8_t val) {
    // Toggle enable pin on LCD display
    // We cannot do this too quickly or things don't work
#define DELAY_US 600
    sleep_us(DELAY_US);
    i2c_write_byte(val | LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
    i2c_write_byte(val & ~LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
}

// The display is sent a byte as two separate nibble transfers
void lcd_send_byte(uint8_t val, int mode) {
    uint8_t high = mode | (val & 0xF0) | LCD_BACKLIGHT;
    uint8_t low = mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;

    i2c_write_byte(high);
    lcd_toggle_enable(high);
    i2c_write_byte(low);
    lcd_toggle_enable(low);
}

void lcd_clear(void) {
    lcd_send_byte(LCD_CLEARDISPLAY, LCD_COMMAND);
}

// go to location on LCD
void lcd_set_cursor(int line, int position) {
    int val = (line == 0) ? 0x80 + position : 0xC0 + position;
    lcd_send_byte(val, LCD_COMMAND);
}

static void inline lcd_char(char val) {
    lcd_send_byte(val, LCD_CHARACTER);
}

void lcd_string(const char *s) {
  while (*s) {
      lcd_char(*s++);
  }
}

void lcd_init(void) {
  lcd_send_byte(0x03, LCD_COMMAND);
  lcd_send_byte(0x03, LCD_COMMAND);
  lcd_send_byte(0x03, LCD_COMMAND);
  lcd_send_byte(0x02, LCD_COMMAND);

  lcd_send_byte(LCD_ENTRYMODESET | LCD_ENTRYLEFT, LCD_COMMAND);
  lcd_send_byte(LCD_FUNCTIONSET | LCD_2LINE, LCD_COMMAND);
  lcd_send_byte(LCD_DISPLAYCONTROL | LCD_DISPLAYON, LCD_COMMAND);
  lcd_clear();
  lcd_send_byte(LCD_SETCGRAMADDR, LCD_COMMAND);
  lcd_create_symbol(grado);
  lcd_send_byte( LCD_SETCGRAMADDR + 8, LCD_COMMAND);
  lcd_create_symbol(izq);
  lcd_send_byte( LCD_SETCGRAMADDR + 16, LCD_COMMAND);
  lcd_create_symbol(der);
  lcd_send_byte( LCD_SETCGRAMADDR + 24, LCD_COMMAND);
  lcd_create_symbol(simboloCalor1);
  lcd_send_byte( LCD_SETCGRAMADDR + 32, LCD_COMMAND);
  lcd_create_symbol(simboloCalor2);
  lcd_clear();
}

void lcd_create_symbol(const char *input){
  for(int i=0;i<7;i++){
    lcd_char(input[i]);
  }
}

void msg_init(void){  
  msg[0] = "  Bano de agua  ";   // 1 
  msg[1] = " Arla Foods Ing.";   // 2
  msg[2] = "ENTER para cont.";   // 3
  msg[3] = "  Ciclo         ";   // 4
  msg[4] = "T        T      ";   // 5
  msg[5] = "Ciclo           ";   // 6
  msg[6] = "  Finalizado    ";   // 7
  msg[7] = "     MANUAL     ";   // 8
  msg[8] = "ERROR en sensor ";   // 9
  msg[9] = "      OFF       ";   //10
  msg[10]= "BOMBA ENCENDIDA ";   //11
  msg[11]= "BOMBA APAGADA   ";   //12
  msg[12]= " Configuracion  ";   //13
  msg[13]= "Potencia        ";   //14
  msg[14]= "JC Instalaciones";   //15
  msg[16]= "V4.0 20/07/2022 ";   //16
}

void gpio_init_all(void){
	gpio_init(GPIO_ENCODER_A);
  gpio_set_dir(GPIO_ENCODER_A, GPIO_IN);
  gpio_pull_up(GPIO_ENCODER_A);
	gpio_init(GPIO_ENCODER_B);
  gpio_set_dir(GPIO_ENCODER_B, GPIO_IN);
  gpio_pull_up(GPIO_ENCODER_B);
	gpio_init(GPIO_UP);
  gpio_set_dir(GPIO_UP, GPIO_IN);
  gpio_pull_up(GPIO_UP);
	gpio_init(GPIO_DOWN);
  gpio_set_dir(GPIO_DOWN, GPIO_IN);
  gpio_pull_up(GPIO_DOWN);
	gpio_init(GPIO_SET);
  gpio_set_dir(GPIO_SET, GPIO_IN);
  gpio_pull_up(GPIO_SET); 
	gpio_init(GPIO_BACK);
  gpio_set_dir(GPIO_BACK, GPIO_IN);
  gpio_pull_up(GPIO_BACK);
	gpio_init(GPIO_HEATER);
  gpio_set_dir(GPIO_HEATER, GPIO_OUT);
	gpio_init(GPIO_LED);
  gpio_set_dir(GPIO_LED, GPIO_OUT);
	gpio_init(GPIO_LED_RP);
  gpio_set_dir(GPIO_LED_RP, GPIO_OUT);

  adc_init();
  adc_gpio_init(GPIO_SENSOR_A);
  adc_select_input(0);

  gpio_put(GPIO_HEATER, LOW);
  gpio_put(GPIO_LED, HIGH);
  gpio_put(GPIO_LED_RP, HIGH);
}

void i2c_lcd_init(void){
  	// I2C - LCD - inits
  i2c_init(i2c_default, 100 * 1000);
  gpio_set_function(GPIO_LCD_SDA, GPIO_FUNC_I2C);
  gpio_set_function(GPIO_LCD_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(GPIO_LCD_SDA);
  gpio_pull_up(GPIO_LCD_SCL);
  
  // I2C - LCD - Make the I2C pins available to picotool
  bi_decl(bi_2pins_with_func(GPIO_LCD_SDA, GPIO_LCD_SCL, GPIO_FUNC_I2C));
}

void load_flash(void){
  /* Variables to load:
     cycles -> uint8_t
     offsetA -> uint8_t
     offsetB -> uint8_t
     slopeA -> uint8_t
     slopeB -> uint8_t
     temperature[5] -> uint8_t x5
     cycleLife[5] -> uint16_t x5

     Used 20 bytes of memory flash
  */
  int i = 0;
  int j = 0;
  cycles = flash_target_contents[i++];
  offsetA = flash_target_contents[i++];
  offsetB = flash_target_contents[i++];
  slopeA = flash_target_contents[i++];
  slopeB = flash_target_contents[i++];

  for (j=0;j<CYCLES;j++){
    temperature[j] = flash_target_contents[i++];
    cycleLife[j] = flash_target_contents[i++] + (flash_target_contents[i++] * 256);
  }
}

void save_flash(void){
  /* Variables to load:
     cycles -> uint8_t
     offsetA -> uint8_t
     offsetB -> uint8_t
     slopeA -> uint8_t
     slopeB -> uint8_t
     temperature[5] -> uint8_t x5
     cycleLife[5] -> uint16_t x5

     Used 20 bytes of memory flash
  */
 	
  uint8_t  new_flash_target[FLASH_PAGE_SIZE];
  int i = 0;
  int j = 0;
  new_flash_target[i++] = cycles;
  new_flash_target[i++] = offsetA;
  new_flash_target[i++] = offsetB;
  new_flash_target[i++] = slopeA;
  new_flash_target[i++] = slopeB;

  for (j=0;j<CYCLES;j++){
    new_flash_target[i++] = temperature[j];
    new_flash_target[i++] = cycleLife[j] & 0xFF;
    new_flash_target[i++] = ((cycleLife[j] & 0xFF00) >> 8);
  }
  // disable interrupts for saving
  uint32_t ints = save_and_disable_interrupts();

  flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
  flash_range_program(FLASH_TARGET_OFFSET, new_flash_target, FLASH_PAGE_SIZE);
  restore_interrupts (ints);
}


void lcd_string_temperature(float temperatureIn, int x, int y){
  char temperatureString [5] = "";
  sprintf(temperatureString,"%.1f",temperatureIn);
  lcd_set_cursor(x,y);
  if(temperatureIn < 10) lcd_char('0');
  lcd_string(temperatureString);
  lcd_char(0);
  lcd_char(' ');
}

void lcd_string_time(uint16_t time , int x, int y){
  char timeString [6] = "";
  lcd_set_cursor(x,y);
  if (time < 60 ){  //    Set tiempo: 0:xx
    lcd_string("0:");
    if(time < 10){  //    Set tiempo: 0:0x
      sprintf(timeString,"0%d",time);
      lcd_string(timeString);
    }
    else{
      sprintf(timeString,"%d",time);
      lcd_string(timeString);
    }
  }
  else{//     Set tiempo: 0x:xx
    int temp = time/60;
    sprintf(timeString,"%d:",temp);
    lcd_string(timeString);
    if( (time - (temp * 60)) < 10 ){ //0X:0X
      sprintf(timeString,"0%d",(time - (temp * 60)));
      lcd_string(timeString);
    }
    else{ // 0X:XX
      sprintf(timeString,"%d",(time - (temp * 60)));
      lcd_string(timeString);
    }
  }
  if(time < 600) lcd_char(' ');
}
