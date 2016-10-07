#include <DFPlayer_Mini_Mp3.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <Keypad.h>
#include <Wire.h>
#include <EEPROM.h>


RTC_DS1307 rtc;
LiquidCrystal_I2C lcd(0x3F, 16, 2);

byte time_set;
byte count_call = 0;
byte current_call_h = 0;
byte current_call_m = 0;

String today;
boolean endCall = true;
byte numDay;
byte current_call = 0;

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the cymbols on the buttons of the keypads
char Keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {13, 12, 11, 10}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad keypad = Keypad( makeKeymap(Keys), rowPins, colPins, ROWS, COLS); 


void setup (){
  
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
  Wire.begin();
  rtc.begin();
  mp3_set_serial (Serial);
  delay(1);
  mp3_set_volume(25);

  //rtc.adjust(DateTime(__DATE__, __TIME__));
  //EEPROM.write(0, 0); // Убрать после сборки
 
  time_set = EEPROM.read(0);
  if (time_set == 0 || time_set == 255){ 
    time_set = set_SettingsCall();
    EEPROM.write(0, time_set);
  }
  lcd.clear();
}

String set_dayOfWeek(byte dayWeek){
  switch (dayWeek) {
    case 1:
      return "Mon";
      break;
    case 2:
      return "Tues";
      break;
    case 3:
      return "Wed";
      break;
    case 4:
      return "Thurs";
      break;
    case 5:
      return "Fri";
      break;
    case 6:
      return "Sat";
      break;
    case 7:
      return "Sun";
      break;
  }
}

byte set_SettingsCall(){
  for (byte d = 1; d < 7; d++){
    char key = NO_KEY;
    char count_callM[2];
    byte i = 0;
    boolean copy = false;
    if (d > 1){
      //Копирование настроек с Понедельника
      lcd.clear();
      lcd.print("Set Mon in ");
      lcd.print(set_dayOfWeek(d));
      lcd.setCursor(4 , 1);
      lcd.print("Yes/No"); 
      while (key != 'A' || key != 'C'){
      key = keypad.waitForKey();
        if (key == 'A'){
          //Копируем
          EEPROM.write(d, EEPROM.read(1));
          for (int x = 0, j = ((d-1) * 96) + 8; x < EEPROM.read(1); x++, j+=2) {
            EEPROM.write(j, EEPROM.read( j - (96 * (d-1)) ) );
            EEPROM.write(j + 1, EEPROM.read( (j - (96 * (d-1))) + 1 ) );            
          }
         copy = true;
         break;          
        }
        if (key == 'C'){
          copy = false;
          break;
        }
      }
    }
    if (copy){ continue; }
    lcd.clear();
    lcd.print("Settings: ");
    lcd.print(set_dayOfWeek(d));
    lcd.setCursor(0, 1);
    lcd.print("Number calls:");
    lcd.setCursor(14, 1);

    //key = keypad.waitForKey();
    while (key != 'A') {
      key = keypad.waitForKey();
      if (i == 0 && key == '0') { continue; } 
      if (key == 'B' && i > 0) {
          i--;
          count_callM[i] = ' ';
          lcd.setCursor(i + 14, 1); // 12 Заменить на кол-во букв прошлого принта
          lcd.print(" ");
          continue;   
      }
      if (i == 1 && String(count_callM[0]).toInt() > 4) { continue; }    
      if (key == 'D' || key == '*' || key == '#' || key == 'B'){ continue; }
      if (key == 'A'){
        if(i > 0) { 
          break; 
        } else { 
          key = NO_KEY; 
          continue; 
        }
      }
      if (key == 'C') { return 1; }
      if (i > 1) { continue; };
      count_callM[i] = key;
      lcd.setCursor(i + 14, 1);
      lcd.print(count_callM[i]);
      i++;
    } 

    count_call = (String(count_callM[0]) + String(count_callM[1])).toInt();
    EEPROM.write(d, count_call);
    
    for (int x = 0, j = ((d-1) * 96) + 8; x < count_call; x++, j+=2) {
      i = 0;
      char timeCall[] = "8888";
      key = NO_KEY;
      byte prevH;
      byte prevM;
      if (x > 0) {
        prevH = EEPROM.read(j-2);
        prevM = EEPROM.read(j-1);      
      }
      //Вывод на монитор настроек времени текущего звонка
      lcd.clear();
      lcd.print("Call: " + String(x+1));
      lcd.setCursor(9, 0);
      lcd.print("in ");
      lcd.print(set_dayOfWeek(d));
      //Запрос времени
      while (key != 'A') {
        lcd.setCursor(5, 1);
        lcd.print(timeCall[0]);
        lcd.setCursor(6, 1);
        lcd.print(timeCall[1]);
        lcd.setCursor(7, 1);
        lcd.print(":");
        lcd.setCursor(8, 1);
        lcd.print(timeCall[2]);
        lcd.setCursor(9, 1);
        lcd.print(timeCall[3]);
        
        key = keypad.waitForKey();   
        if (key == 'B' && i > 0) {
          i--;
          timeCall[i] = '8';
          continue;   
        }
        if (key == 'C'){
          return 0;
          break;
        }
        if (key == 'D' || key == '*' || key == '#' || key == 'B'){ continue; } 
        if (key == 'A'){
          if(i > 3) { 
            break; 
          } else { 
            key = NO_KEY; 
            continue; 
          }
        }     
        if (i > 3) { continue; };
        //Проверка правильности времени
        if (i == 0 && int(key) > 50) { continue; } // Число 2 равно числу 50 ASCII
        if (i == 1 && timeCall[0] == '2' && int(key) > 51) { continue; }  //С 00:00 ПРИДУМАТЬ
        if (i == 2 && int(key) > 53) { continue; }
  
        timeCall[i] = key;
        if (x > 0 && i > 0) {
          if ( prevH > (String(timeCall[0]) + String(timeCall[1])).toInt() ){
            i--;
            timeCall[0] = '8';
            timeCall[1] = '8';
            continue;   
          } else if ( prevH == (String(timeCall[0]) + String(timeCall[1])).toInt() ) {
            if (i == 2) {
              if ( prevM / 10 > (String(timeCall[2])).toInt() ) {
                timeCall[2] = '8';
                continue;
              }
            }         
            if (i > 2){ 
              if( prevM >= (String(timeCall[2]) + String(timeCall[3])).toInt() ) {
                timeCall[2] = '8';
                timeCall[3] = '8';
                i--;
                continue;   
              }        
            }       
          } 
        }
        i++;  
      }      
      //Сохранение в EEPROM Arduino
      byte tmpH = (String(timeCall[0]) + String(timeCall[1])).toInt();
      byte tmpM = (String(timeCall[2]) + String(timeCall[3])).toInt();
      EEPROM.write(j, tmpH);
      EEPROM.write(j + 1, tmpM);
    }  
  }
  return 1;  
}

void set_nextCall(byte dayWeek, DateTime now){
  if (dayWeek == 7){
    endCall = true;
    return;
  }
  numDay = dayWeek;
  byte call_count_mem = EEPROM.read(dayWeek);
  byte tmpTimeH;
  byte tmpTimeM;
  current_call_h = 0;
  current_call_m = 0;
    
  for (int i = 0, j = ((dayWeek - 1) * 96) + 8; i != call_count_mem; i++, j+=2) {
     tmpTimeH = EEPROM.read(j);
     tmpTimeM = EEPROM.read(j+1);
     if ( tmpTimeH == now.hour() && tmpTimeM == now.minute()) { continue; }      
     if ( tmpTimeH >= now.hour() ) {
        if (tmpTimeH == now.hour()) {
          if(tmpTimeM > now.minute()) {
            current_call_h = tmpTimeH;
            current_call_m = tmpTimeM;
            current_call = i+1;
            break;
          } else continue;
        }
        if ( tmpTimeM < now.minute() ){
          current_call_h = tmpTimeH - 1;
          current_call_m = tmpTimeM + 60;          
        } else {
          current_call_h = tmpTimeH;
          current_call_m = tmpTimeM;  
        }

        if (tmpTimeM == 0){
          current_call_m = 60;
        }
        current_call = i+1;
        break;
     }
  }

  
  endCall = false;
 
  if (current_call_h == 0 && current_call_m == 0){
    //Если уроки кончились
    endCall = true;
  } else {
    if(current_call > (call_count_mem / 2)){     // count = 8
      current_call = current_call - call_count_mem / 2;
    }    
  }
  EEPROM.write(1000, current_call);
}
void loop () {

  DateTime now = rtc.now();

  byte cur_dayWeek = now.dayOfTheWeek();
  today = set_dayOfWeek(cur_dayWeek); 
  
  if (time_set == 0){
    time_set = set_SettingsCall();
    lcd.clear();
    set_nextCall(cur_dayWeek, now);
  } 
    
  char key = keypad.getKey();
  if(key == 'D'){
    time_set = 0;
    delay(2000);
  }
  if(key == '*'){
    rtc.adjust(now.unixtime() + 60);
    set_nextCall(cur_dayWeek, now);
  }
  if(key == '#'){
    rtc.adjust(now.unixtime() - 60);
    set_nextCall(cur_dayWeek, now);
  } 
  lcd.setCursor(0, 0);
  lcd.print(now.hour()/10); 
  lcd.print(now.hour()%10); 
  lcd.print(":");
  lcd.setCursor(3, 0);
  lcd.print(now.minute()/10);
  lcd.print(now.minute()%10);
  lcd.print(":");
  lcd.setCursor(6, 0);
  lcd.print(now.second()/10);
  lcd.print(now.second()%10);
  lcd.setCursor(10, 0);
  lcd.print(today);
  lcd.setCursor(0, 1);

  byte timeToCallH = current_call_h - now.hour();
  byte timeToCallM = current_call_m - now.minute(); 

  if (timeToCallH > 25 && !endCall) { set_nextCall(cur_dayWeek, now); } 

  if(timeToCallH == 0 && timeToCallM == 0 && !endCall){
    current_call = EEPROM.read(1000);
    mp3_play (current_call);   
    if ( cur_dayWeek == 1 && current_call == 1 ){
      delay (30000);
      mp3_play(90);
    }    
    delay(10);    
    set_nextCall(cur_dayWeek, now);
  } 
  
  if (endCall){
    if (cur_dayWeek - numDay != 0){
      set_nextCall(cur_dayWeek, now); 
    }
    lcd.print("Today lesson end");
    lcd.print("      ");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("To call:  ");
    lcd.setCursor(10, 1);
    lcd.print(timeToCallH/10);
    lcd.print(timeToCallH%10);
    lcd.setCursor(12, 1);
    lcd.print(":");
    lcd.print(timeToCallM/10);
    lcd.print(timeToCallM%10);
    lcd.print(" ");
  }
  delay(1000);
} 
