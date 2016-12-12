#include <DFPlayer_Mini_Mp3.h>        //   Подключения библиотек.
#include <SoftwareSerial.h>           //   Mp3 модуль.
#include <LiquidCrystal_I2C.h>        //   1602 дисплей.
#include <RTClib.h>                   //   Модуль реального времени.
#include <Keypad.h>                   //   Матричные кнопки.
//#include <Wire.h>
#include <EEPROM.h>                   //   Работа с ПЗУ Arduino.

const byte ROWS = 4;
const byte COLS = 4;
char Keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6};        //  Подключения пинов для строк.
byte colPins[COLS] = {13, 12, 11, 10};    //  Подключения пинов для столбцов.
Keypad keypad = Keypad( makeKeymap(Keys), rowPins, colPins, ROWS, COLS);  // Инициализация клавиш.

byte time_set;
byte count_call = 0;
byte current_call_h = 0;
byte current_call_m = 0;

String today;
boolean endCall = true;
byte numDay;
byte current_call = 0;

RTC_DS1307 rtc; 
LiquidCrystal_I2C lcd(0x3F, 16, 2);

void setup (){
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
  Wire.begin();
  rtc.begin();
  mp3_set_serial (Serial);
  delay(1);
  mp3_set_volume(25);
  time_set = EEPROM.read(0);
  //rtc.adjust(DateTime(__DATE__, __TIME__));
}

String set_dayOfWeek(byte &dayWeek){
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
      return "Воскресенье";
      break;
    default:
      return "Ошибка";
  }
}

byte set_SettingsCall(){
  for (byte d = 1; d < 7; d++){ // Цикл кол-во дней в недели.
    char key = NO_KEY;
    boolean copy = false;
    if (d > 1){
      //Копирование настроек с Понедельника
      lcd.clear();
      lcd.print("Set Mon in ");
      lcd.print(set_dayOfWeek(d));
      lcd.setCursor(4 , 1);
      lcd.print("Да/Нет"); 
      while (key != 'A' || key != 'C'){
      key = keypad.waitForKey();
        if (key == 'A'){
          //Копируем
          EEPROM.write(d, EEPROM.read(1)); // Записываем кол-во звонков.
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
    lcd.setCursor(14, 1);             //        Копировать число символов отсюда.
    char count_callM[2];
    byte i = 0;
    while (key != 'A') {
      key = keypad.waitForKey();
      if (i == 0 && key == '0') { continue; } 
      if (key == 'B' && i > 0) {
          i--;
          count_callM[i] = ' ';
          lcd.setCursor(i + 14, 1);   //        сюда
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
        
        if (i > 3) { continue; };  
        if (key == 'B' && i > 0) {
          i--;
          timeCall[i] = '8'; 
        }
        if (key == 'D' || key == '*' || key == '#' || key == 'B'){ continue; } 
        if (key == 'C'){
          return 0;
          break;
        }
        if (key == 'A'){
          if(i > 3) { 
            break; 
          } else { 
            key = NO_KEY; 
            continue; 
          }
        }     

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
  numDay = dayWeek;
  if (dayWeek == 7){
    endCall = true;
    return;
  } 
  byte call_count_mem = EEPROM.read(dayWeek);
  byte tmpTimeH;
  byte tmpTimeM;
  current_call_h = 0;
  current_call_m = 0;
  endCall = false;
    
  for (int i = 0, j = ((dayWeek - 1) * 96) + 8; i != call_count_mem; i++, j+=2) {
     tmpTimeH = EEPROM.read(j);
     tmpTimeM = EEPROM.read(j+1);
     if ( tmpTimeH == now.hour() && tmpTimeM == now.minute()) { continue; }     // 8,00  10,00 
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
  
  if (current_call_h == 0 && current_call_m == 0){
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
  
  if (time_set == 0 || time_set == 255){
    time_set = set_SettingsCall();
    lcd.clear();
    set_nextCall(cur_dayWeek, now);
  }   
  char key = keypad.getKey();
  if (key != NO_KEY){
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
    if(key == '0'){
      set_nextCall(cur_dayWeek, now);     
    }
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
  
  if (endCall){
    if (cur_dayWeek - numDay != 0){
      set_nextCall(cur_dayWeek, now); 
    }
    lcd.print("Today lesson end");
    lcd.print("      ");
  } else {
    if (timeToCallH > 25 || timeToCallM > 60) { set_nextCall(cur_dayWeek, now); }
    if(timeToCallH == 0 && timeToCallM == 0){
      current_call = EEPROM.read(1000);
      mp3_play (current_call);   
      if ( cur_dayWeek == 1 && current_call == 1 ){
        delay (30000);
        mp3_play(90);
      }    
      delay(10);    
      set_nextCall(cur_dayWeek, now);
    } 
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
