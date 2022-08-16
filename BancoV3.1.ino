/*
  Laboratory Water Bath Control
*/

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

//                        V3.1  V2.1   V1.0
#define UPbutton          17  // A2  //  5
#define DOWNbutton        18  // A5  //  7
#define SETbutton         16  // A4  //
#define BACKbutton        19  // A1  //
#define ENCODER_PUSH      22
#define ENCODER_A         21
#define ENCODER_B         20
#define SENSOR            27  // A0  //  2
#define CALENTADOR        10   // 6   //  3
#define LED_INDICADOR     28  // 0
#define LED_RP            25
#define GPIO_LCD_SDA      0
#define GPIO_LCD_SCL      1

#define BRILLO            100
#define Xtemperatura      11
#define Ytemperatura      1
#define Xtiempo           2
#define Ytiempo           1
#define Xfuego            7
#define Yfuego            1

#define address           0
#define MINUTO            4424778 // 2min 4,34/60
#define tolerancia        0.5
#define FACTOR            0.1571245
#define OFFSET            -0.8726
#define N_AVERAGE         200

#define ARRIBA           1
#define ABAJO            2   
#define IZQ              3
#define DER              4
#define OFF              5
#define UP_COUNTER       2
#define DELAYBOTON       10
#define PARPADEO         80000

LiquidCrystal_I2C lcd(0x27,20,4);

/////////////// Vatiables globales ///////////////////////
String mensaje []={
  "  Bano de agua  ", // 1 
  "Arla Foods Ing. ", // 2
  "ENTER para cont.", // 3
  "  Ciclo         ", // 4
  "T        T      ", // 5
  "Ciclo           ", // 6
  "  Finalizado    ", // 7
  "     MANUAL     ", // 8
  "ERROR en sensor ", // 9
  "      OFF       ", // 10
  "", // 11
  "", // 12
  " Configuracion  ", // 13
  "Potencia        ", // 14
  "JC Instalaciones", // 15
  "HW:v3  SW:v3.1  "  // 16
};
byte grado[8]={
  0b00011000,
  0b00011000,
  0b00000011,
  0b00000100,
  0b00000100,
  0b00000100,
  0b00000011,
  0b00000000
};
byte izq[8]={
  0b00000011,
  0b00000111,
  0b00001111,
  0b00011111,
  0b00001111,
  0b00000111,
  0b00000011,
  0b00000000
};
byte der[8]={
  0b00011000,
  0b00011100,
  0b00011110,
  0b00011111,
  0b00011110,
  0b00011100,
  0b00011000,
  0b00000000
};
byte simboloCalor1[8]={    
  0b00001001,    
  0b00001001,    
  0b00010010,    
  0b00010010,    
  0b00001001,    
  0b00001001,    
  0b00011111,    
  0b00011111
};
byte simboloCalor2[8]={  
  0b00010010,    
  0b00010010,    
  0b00001001,    
  0b00001001,    
  0b00010010,    
  0b00010010,    
  0b00011111,    
  0b00011111
};
byte enie[8]={  
  0b00011110,    
  0b00000000,    
  0b00011110,    
  0b00010001,    
  0b00010001,    
  0b00010001,    
  0b00010001,    
  0b00000000
};

int msj = 10;
int menu = 0;
int preMenu = 0;
int ciclo = 1; 
float tempActual = 0;
int unsigned tiempo[]={10,10,10,10,10,10};
int  temperatura[] ={10,20,30,40,50,60,70};
int parpadeo = 0;
double loopCounter = 0; 
int positionCounter = 0;
int arribaCounter = 0;
int abajoCounter = 0;
int izqCounter = 0;
int derCounter = 0;
int retardo = 100;
int estado = 0;
int i = 0;

int counter = 0; // For encoder
int aState;      // For encoder
int aLastState;  // For encoder

  
 /////////////// Funciones ////////////////////////////
void imprimir_mensaje_LCD(int entrada, int x, int y,int flechas);
void imprimir_tiempo_LCD(int entrada, int x, int y);
void imprimir_temp_LCD(float entrada, int x, int y);
void load(void);
void save(void);
void confLCD(void);
void gpio_init(void);

  /////////////// VOID SETUP ////////////////////////////
void setup() {
  gpio_init();
  delay(300);
  load();
  confLCD();
  aLastState = digitalRead(ENCODER_A);
}

/////////////// VOID LOOP ///////////////////////////////
void loop() {
  if (parpadeo == PARPADEO ){
    digitalWrite(LED_RP,!digitalRead(LED_RP));
  }

  if (preMenu != menu){
    Serial.print("menu: ");
    Serial.print(menu);
    Serial.print(" Ciclo: ");
    Serial.print(ciclo);
    Serial.print(" Tiempo: ");
    Serial.print(tiempo[ciclo-1]);
    Serial.print("min Temperatura: ");
    Serial.print(temperatura[ciclo-1]);
    Serial.println("°C");
    preMenu = menu;
    
  }

  //////////////// DELAYS PULSADORES /////////////////////
  int boton = 0;
  retardo = 200;

  // Encoder 
  aState = digitalRead(ENCODER_A);
  if (aState != aLastState){
    if (digitalRead(ENCODER_B) != aState) 
      counter ++;
    else
      counter --;
    Serial.println(counter);
    if (counter >= UP_COUNTER){
      counter = 0;
      boton = ARRIBA;
    }
    else if (counter <= UP_COUNTER*-1){
      counter = 0;
      boton = ABAJO;
    }
  }
  aLastState = aState; // Guardamos el ultimo valor

  if ( digitalRead(UPbutton) == LOW ){
    arribaCounter ++; 
    if ( digitalRead(DOWNbutton) == LOW ){
      arribaCounter = 0;
      abajoCounter = 0;
      // Blanqueo rapido de variables de Temperatura y Tiempo de ciclo
      for ( int i = 0; i <= 5 ; i++) {
        tiempo[i] = i*10;
        temperatura[i]=10*i;
      }       
    }
    if ( arribaCounter >= DELAYBOTON ){
      boton = ARRIBA;
      if (arribaCounter >= (DELAYBOTON*2)) retardo = 50;
      if (arribaCounter >= (DELAYBOTON*5)) retardo = 2;
    }
  }
  else arribaCounter = 0;

  if ( digitalRead(DOWNbutton) == LOW ){
    abajoCounter ++;
    if ( abajoCounter >= DELAYBOTON ){
      boton = ABAJO;
      if (abajoCounter >= (DELAYBOTON*2)) retardo = 50;
      if (abajoCounter >= (DELAYBOTON*5)) retardo = 2;
    }
  }
  else abajoCounter = 0;

  if ( digitalRead(BACKbutton) == LOW ){
    izqCounter ++;
    if ( izqCounter == DELAYBOTON*20){
      boton = IZQ;
      delay(retardo);
    }
  }
  else izqCounter = 0;

  if ( digitalRead(SETbutton) == LOW || digitalRead(ENCODER_PUSH) == LOW ){
    derCounter ++;
    if ( derCounter == DELAYBOTON*20 ){
      boton = DER; 
      Serial.println("SET_BUTTON");
      delay(retardo);                
    }
  }
  else derCounter = 0;

  //////////// CORTES MAX y MIN ///////////////////
  if ( temperatura[ciclo] < 0 ) temperatura[ciclo] = 0;
  if ( temperatura[ciclo] >= 99 ) temperatura[ciclo] = 99;
  if ( tiempo[ciclo] <= 0 ) tiempo[ciclo] = 0;
  if ( tiempo[ciclo] >= 1000 ) tiempo[ciclo] = 1000;

  if ( menu == 7 ){ /////////// CICLO FASE 2 ///////////////////
    ledParpadeando();
    loopCounter ++;
    if (tiempo[ciclo] == 0 ){
      ciclo++;
      if( ciclo > 5 ) menu = 8;
      else menu = 5;
    }
    else{
      if ( loopCounter >= MINUTO ){
        loopCounter = 0;
        // menu = 5; // vuelvo por configurar la pantalla de nuevo act. 20/11/2018
        if ( tiempo[ciclo] == 600){ 
          lcd.setCursor(Xtiempo+4,Ytiempo);
          lcd.print(" ");
        }
        tiempo[ciclo]--;
        imprimir_tiempo_LCD(tiempo[ciclo],Xtiempo,Ytiempo);
        if( tiempo[ciclo] == 0 ){
          ciclo++;
          if( ciclo > 5 ) menu = 8;
          else menu = 5;
        }
      }                    
    }
    if ( parpadeo == 0 ){
      tempActual = 0; // Reset promedio
      if ( digitalRead(CALENTADOR) ){
        lcd.setCursor(Xfuego,Yfuego);
        lcd.write(4);
      }
      else {
        lcd.setCursor(Xfuego,Yfuego);
        lcd.print(" ");
      }
      if( tiempo[ciclo] < 600 ) lcd.setCursor(Xtiempo+1,Ytiempo);
      else lcd.setCursor(Xtiempo+2,Ytiempo);
      lcd.print(":");
    }
    if( parpadeo <= PARPADEO){
      if(parpadeo % (PARPADEO / N_AVERAGE) == 0 ){
        float aux = analogRead(SENSOR);
        tempActual = tempActual + aux;
      }
    }
    if ( parpadeo == PARPADEO ){
      tempActual = (tempActual / (N_AVERAGE-1))*FACTOR +OFFSET;
      Serial.print("\nTemperatura: ");
      Serial.print(tempActual);
      Serial.print(" Tension: ");
      Serial.println(tempActual*2);
      imprimir_temp_LCD(tempActual,Xtemperatura,Ytemperatura);
      if ( tempActual >= temperatura[ciclo] ){
        digitalWrite(CALENTADOR,LOW);
      } 
      if ( tempActual <= temperatura[ciclo]-tolerancia ){
        lcd.setCursor(Xfuego,Yfuego);
        lcd.write(5);
        digitalWrite(CALENTADOR,HIGH);  
      }
      if ( tiempo[ciclo] < 600) lcd.setCursor(Xtiempo+1,Ytiempo);
      else lcd.setCursor(Xtiempo+2,Ytiempo);
      lcd.print(" ");
    }
    parpadeo++;
    if ( parpadeo >= PARPADEO*2 ){
      parpadeo = 0;  
      if ( msj != 6 ){
        imprimir_mensaje_LCD(6,0,0,0);
        lcd.setCursor(7,0);
        lcd.print(ciclo);
        imprimir_temp_LCD(temperatura[ciclo],11,0);  
        digitalWrite(LED_INDICADOR,LOW);       
      }
    }
  } // end of menu 7

  if ( menu == 6 ){ ////////////// CICLO FASE 1 ////////////////
    digitalWrite(LED_INDICADOR,HIGH);
    if (parpadeo == 0){
      tempActual = 0;                         
      lcd.setCursor(Xfuego,Yfuego);
      lcd.write(5);
    }
    if( parpadeo <= PARPADEO){
      if(parpadeo % (PARPADEO/N_AVERAGE) == 0){
        float aux = analogRead(SENSOR);
        tempActual = tempActual + aux;
      }
    }
     if (parpadeo == PARPADEO){
      tempActual = (tempActual / (N_AVERAGE+1))*FACTOR +OFFSET;
      Serial.print("\nTemperatura: ");
      Serial.print(tempActual);
      Serial.print(" Tension: ");
      Serial.println(tempActual*2);
      imprimir_temp_LCD(tempActual,Xtemperatura,Ytemperatura);                          
      lcd.setCursor(Xfuego,Yfuego);
      lcd.write(4);
      if ( tempActual >= temperatura[ciclo] || tiempo[ciclo] == 0 ) menu = 7;
      else digitalWrite(CALENTADOR,HIGH);  
    }
    parpadeo++;
    if(parpadeo >= PARPADEO*2 ){
      if ( msj != 6 ){
        imprimir_mensaje_LCD(6,0,0,0);
        lcd.setCursor(7,0);
        lcd.print(ciclo);
        imprimir_temp_LCD(temperatura[ciclo],11,0);  
        digitalWrite(LED_INDICADOR,LOW);       
      }
      parpadeo = 0;
    }
  } // end of menu 6

  if ( (menu == 10) || (menu == 1)  ){ /////////// CICLO MANUAL /////////////////
    loopCounter++;
    if( loopCounter == PARPADEO){
      imprimir_mensaje_LCD(3,0,1,0);
      digitalWrite(LED_INDICADOR, !digitalRead(LED_INDICADOR));
      loopCounter = 0;
    }
    if ( boton != LOW  ){
      if( menu == 1){
        menu = 2;
        tempActual= 0;
      }
      else{
        if (ciclo == 5) menu = 8;
        else{ 
          menu = 5;
          ciclo ++;
        }
        //confLCD();
      }
    }
  } // end of menu 10 || 1 CICLO MANUAL

  if ( menu == 5 ){ ////////////// COMIENZA CICLO ////////////////
    confLCD();
    if( temperatura[ciclo] == 0 ){
      imprimir_mensaje_LCD(8,0,0,0);
      lcd.setCursor(12,0);
      lcd.print(ciclo);
      menu = 10;
    }
    else{
      imprimir_mensaje_LCD(6,0,0,0);
      lcd.setCursor(7,0);
      lcd.print(ciclo);
      imprimir_temp_LCD(temperatura[ciclo],11,0);
      imprimir_mensaje_LCD(5,0,1,0);
      imprimir_temp_LCD(tempActual,Xtemperatura,Ytemperatura);
      imprimir_tiempo_LCD(tiempo[ciclo],Xtiempo,Ytiempo);
      menu = 6;
      parpadeo = 0;
    }    
  } // end of menu 5

  if ( menu == 12){
    menu = 2;
  }
    
  if ( menu == 11){
    imprimir_mensaje_LCD(13,0,0,0);
    lcd.setCursor(15,0);
    lcd.write(3);
    menu = 12;
  }
  // cambios de ciclo en funcionamiento 
  if( menu == 6 || menu== 7 ){
    if(boton == IZQ ){
      if( ciclo > 1 ){
        ciclo = ciclo -1;
        menu = 5;
      }
    }
    if(boton == DER ) {
      if(ciclo<5 && tiempo[ciclo+1]!=0){
        ciclo = ciclo +1;
        menu = 5;
      }
    }
    if (boton == ARRIBA || boton == ABAJO){
      menu = 5;
    }
  }
  if ( menu == 3){////////////// SET TEMPERATURA ////////////////
    if( boton == LOW && temperatura[ciclo] > 0 ){
      if( parpadeo == 0 ){
        imprimir_temp_LCD(temperatura[ciclo],Xtemperatura,Ytemperatura);
      }
      if( parpadeo == PARPADEO){
        lcd.setCursor(Xtemperatura,Ytemperatura);
        lcd.print("    ");
      }
      parpadeo++;
      if( parpadeo >= PARPADEO*2 ) parpadeo = 0;
    }
    else parpadeo = 0;      
    if ( boton == ARRIBA ){ 
      if ( temperatura[ciclo] == 0 ){
        imprimir_mensaje_LCD(5,0,1,0);
        imprimir_tiempo_LCD(tiempo[ciclo],Xtiempo,Ytiempo);
      }  
      temperatura[ciclo]++;
      imprimir_temp_LCD(temperatura[ciclo],Xtemperatura,Ytemperatura);
      delay(retardo);
    }
    if ( boton == ABAJO ){
      if (temperatura[ciclo]> 0){
        temperatura[ciclo]--;
        imprimir_temp_LCD(temperatura[ciclo],Xtemperatura,Ytemperatura);
        delay(retardo);
      }
    }
    if ( temperatura[ciclo] == 0 ) imprimir_mensaje_LCD(8,0,1,0);
    if ( boton == DER ){
      if ( ciclo == 5 ){
        ciclo = 1;
        menu = 5;
        save();
      }
      else{
        ciclo ++;
        menu = 2;
      }
    }
    if ( boton == IZQ ){
      imprimir_temp_LCD(temperatura[ciclo],Xtemperatura,Ytemperatura);
      menu = 4;
    }
  } // end of menu 3 SET TEMPERATURA

  if ( menu == 4 ){////////////// SET TIEMPO ////////////////
    if( boton == LOW && tiempo[ciclo]>0 ){
      if( parpadeo == 0 ){
        imprimir_temp_LCD(temperatura[ciclo],Xtemperatura,Ytemperatura);
        imprimir_tiempo_LCD(tiempo[ciclo],Xtiempo,Ytiempo);
      }     
      if( parpadeo == PARPADEO ){
        lcd.setCursor(Xtiempo,Ytiempo);
        if(tiempo[ciclo] < 600) lcd.print(" :  ");
        else lcd.print("  :  ");  
      }
      parpadeo++;  
      if( parpadeo >= PARPADEO*2 ) parpadeo =0;
    }
    else  parpadeo = 0;        
    if ( boton == ARRIBA ){
      if ( tiempo[ciclo] == 0 ){
        imprimir_mensaje_LCD(5,0,1,0);
        imprimir_temp_LCD(temperatura[ciclo],Xtemperatura,Ytemperatura);
      }
      tiempo[ciclo]++;
      imprimir_tiempo_LCD(tiempo[ciclo],Xtiempo,Ytiempo);
      delay(retardo);
    }
    if( boton == ABAJO){
      if ( tiempo[ciclo] > 0 ){
        tiempo[ciclo]--;
        imprimir_tiempo_LCD(tiempo[ciclo],Xtiempo,Ytiempo);
        delay(retardo);
      }
    }
    if ( tiempo[ciclo] == 0 ) imprimir_mensaje_LCD(10,0,1,0);
    if ( boton == IZQ ){
      if ( ciclo > 1 ){
        ciclo --;
        menu = 2;
      }
      else menu = 11;
    }
    if( boton == DER ){
      if( tiempo[ciclo] > 0){
        imprimir_tiempo_LCD(tiempo[ciclo],Xtiempo,Ytiempo);
        menu = 3;               
      }
      else {
        if (ciclo == 5){
          menu = 5;
          ciclo = 1;
          save();                     
        }
        else {
          ciclo++;
          menu = 2;
        }
      }
    }
  } // end of menu 4 SET TIEMPO

  if( menu == 2 ){/////////// IMPRIME CICLO  ////////////////

    imprimir_mensaje_LCD(4,0,0,1);
    lcd.setCursor(13,0);
    lcd.print(ciclo);
    imprimir_mensaje_LCD(5,0,1,0);
    imprimir_tiempo_LCD(tiempo[ciclo],Xtiempo,Ytiempo);
    imprimir_temp_LCD(temperatura[ciclo],Xtemperatura,Ytemperatura);
    menu = 4;
    if (ciclo > 5) menu = 5;
  }

  if ( menu == 0 ){ //////////// INICIO /////////////////////
    imprimir_mensaje_LCD(1,0,0,0); //bienvenida
    lcd.setCursor(4,0); // coloca la ñ
    lcd.write(6);
    menu = 1;
    digitalWrite(LED_INDICADOR,HIGH);          
  }

  if ( menu == 8 ){ ////////////// FINALIZADO ////////////////
    digitalWrite( CALENTADOR, LOW );
    imprimir_mensaje_LCD(0,0,0,0);
    lcd.setCursor(4,0);
    lcd.write(6);
    delay(PARPADEO);
    menu = 9;             
  }

  if ( menu == 9 ){ ///////////////// STAND BY ////////////////// 
    ledParpadeando();
  }

  if ( boton == OFF ) menu = 8;

} ///////////////// FIN VOID LOOP /////////////////////////


///////////////// Funciones  //////////////////////////////

////////////// INICIALIZAR ENTRADAS Y SALIDAS ///////////////////
 void gpio_init(void){
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();
  pinMode( UPbutton, INPUT_PULLUP );
  pinMode( DOWNbutton, INPUT_PULLUP );
  pinMode( SETbutton, INPUT_PULLUP );
  pinMode( BACKbutton, INPUT_PULLUP );
  pinMode( LED_INDICADOR, OUTPUT );
  pinMode( LED_RP, OUTPUT );
  pinMode( CALENTADOR, OUTPUT );
  pinMode( ENCODER_PUSH, INPUT_PULLUP );
  pinMode( ENCODER_A, INPUT_PULLUP );
  pinMode( ENCODER_B, INPUT_PULLUP );
  digitalWrite( LED_INDICADOR, HIGH );
  digitalWrite( CALENTADOR, LOW );
 }

/////////// IMPRIME MENSAJE POR UNICA VEZ EN LCD //////////////////

void imprimir_mensaje_LCD(int entrada, int x, int y, int flechas){
  if (entrada != msj){
    lcd.setCursor(x,y);
    if (entrada == 0) {
      imprimir_mensaje_LCD(1, 0, 0, 0);
      imprimir_mensaje_LCD(7, 0, 1 ,0);
    }
    else {
      lcd.print(mensaje[entrada-1]);
      if (flechas != 0){
        lcd.setCursor(0,y);
        lcd.write(2);
        lcd.setCursor(15,y);
        lcd.write(3);
      }
    }
  }
  msj = entrada;
  return;
}

////////////// IMPRIME TIEMPO EN LCD ///////////////////
 void imprimir_tiempo_LCD(int tiempo, int x, int y){
  if (tiempo < 60 ){  //    Set tiempo: 0:xx
    lcd.setCursor(x,y);
    lcd.print("0:");
    if(tiempo <10){  //    Set tiempo: 0:0x
      lcd.setCursor(x+2,y);
      lcd.print("0");
      lcd.print(tiempo);
    }
    else{
      lcd.setCursor(x+2,y);
      lcd.print(tiempo);
    }
  }
  else{             // Set tiempo: 0x:xx
    int temp = tiempo /60;
    lcd.setCursor(x,y);
    lcd.print(temp);
    lcd.print(":");
    if( (tiempo - (temp * 60)) < 10 ){
      lcd.print("0");
      lcd.print(tiempo - (temp * 60));
    }
    else{
      lcd.print(tiempo-temp*60);
    }  
  }
  if(tiempo < 600) lcd.print(" ");
  return;
}

////////////// IMPRIME TEMPERATURA Y UNIDAD EN LCD /////////////
void imprimir_temp_LCD(float entrada, int x, int y){
  lcd.setCursor(x,y);
  if(entrada < 10) lcd.print("0");
  lcd.print(entrada,1);
  lcd.write(1);
  lcd.print(" ");
  return;
}

////////////////// PARPADEO DEL LED INDICADOR /////////////////
void ledParpadeando(void){
  if(parpadeo % (PARPADEO/20) == 0){
    int outputValue = map(parpadeo, 0, 8000, 0, 255);
    if( parpadeo <= PARPADEO ){
      analogWrite(LED_INDICADOR,outputValue);
    }
    else analogWrite(LED_INDICADOR,(255-outputValue));
  }        
  return;
}

/////////////////////// LOAD /////////////////////////////////
void load(void){
  Serial.print("Load");
  EEPROM.begin(256);
  int unsigned auxiliar[]={0,0,0,0,0,0,0,0,0,0,0,0};
  for (int i = 0; i <= 11;  i++){
    auxiliar[i]=EEPROM.read(i+address);
  }
  for(int i=0;i<=5;i++){
    tiempo[i]=auxiliar[2*i]*256+auxiliar[2*i+1];
  }
  for (int i = 0 ; i <= 5 ; i++ ){
    temperatura[i] = EEPROM.read(i+address+14);
  }  
}

////////////////// SAVE //////////////////////////////////////
void save(void){              
  int unsigned auxiliar[]={0,0,0,0,0,0,0,0,0,0,0,0};
  for(int i=0;i<=5;i++){
  auxiliar[2*i]=tiempo[i]/256;
  auxiliar[2*i+1]=tiempo[i]-(256*auxiliar[2*i]);
  }
  for (int i = 0; i <= 11; i++ ){
    EEPROM.write(i+address,auxiliar[i]);      
  }
  for (int i = 0; i <= 5 ; i++ ){
    EEPROM.write(i+address+14,temperatura[i]);
  }
  EEPROM.commit();
}

////////////////// SAVE //////////////////////////////////////
void confLCD(void){
  lcd.init();
  // Print a message to the LCD.
  delay(1);
  lcd.backlight();
  lcd.createChar(1,grado);
  lcd.createChar(2,izq);
  lcd.createChar(3,der);
  lcd.createChar(4,simboloCalor1);
  lcd.createChar(5,simboloCalor2);
  lcd.createChar(6,enie);  
  delay(1);                       
}

void errorSensor(void){
  if ( tempActual > 150 && ( menu != 1 )) {                
    digitalWrite(CALENTADOR,LOW);
    imprimir_mensaje_LCD(9,0,0,0);
    imprimir_mensaje_LCD(3,0,1,0);
    menu = 1;
  }
}

/* 
  Librerías de RP PICO:
  https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
*/
