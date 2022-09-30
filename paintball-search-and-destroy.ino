#include <WebServer.h>
#include <ESPmDNS.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <WebConfig.h>

#define redLED 32 //define the LED pins
#define yellowLED 12
#define powerpin 13
#define speakerpin 14

IPAddress local_IP(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

String params = "["
  "{"
  "'name':'expTime',"
  "'label':'Zeit bis zur Explusion (s)',"
  "'type':"+String(INPUTNUMBER)+","
  "'min':30,'max':1200,"
  "'default':'120'"
  "},"
  "{"
  "'name':'gTime',"
  "'label':'Spielzeit (m)',"
  "'type':"+String(INPUTNUMBER)+","
  "'min':5,'max':360,"
  "'default':'15'"
  "}"
  "]";
const byte rows = 4; //number of the keypad's rows and columns
const byte cols = 4;
char keyMap [rows] [cols] = { //define the cymbols on the buttons of the keypad

  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[rows] = {19, 18, 5, 17}; //connect to the row pinouts of the keypad
byte colPins[cols] = {16, 4, 0, 2}; //connect to the column pinouts of the keypad

Keypad myKeypad = Keypad( makeKeymap(keyMap), rowPins, colPins, rows, cols);

char* password = "7355608"; //create a password
int dur = 140;


//Laufzeitvariablen
int position = 0; //keypad position
int ledState = LOW;
int interval = 2000;
int previousMillis = 0;
unsigned long previousmillispower = 0;
unsigned long powerintervalmax = 4500;
bool powerOn = false;
int codepos = 0;
int toneActivation = 0;

WebServer server;
WebConfig conf;
LiquidCrystal_I2C lcd(0x27, 16, 2);

boolean initWiFi() {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP("paintball_boom","",1);   
    return false;
}

void handleRoot() {
  conf.handleFormRequest(&server);
  if (server.hasArg("SAVE")) {
    uint8_t cnt = conf.getCount();
    Serial.println("*********** Konfiguration ************");
    for (uint8_t i = 0; i<cnt; i++) {
      Serial.print(conf.getName(i));
      Serial.print(" = ");
      Serial.println(conf.values[i]);
    }
    if(conf.writeConfig()){
       if(conf.readConfig()){
          Serial.println("Config erfolgreich geschrieben und gelesen");
       }else{
        Serial.println("Fehler beim lesen");
       }
       
    }else{
      Serial.println("Fehler beim schreiben der Config");
    }
    
    if (conf.getBool("switch")) Serial.printf(" %i %i %5.2f \n",
                                conf.getInt("expTime"), 
                                conf.getInt("gameTime"));
  }
}

void setup() {
  Serial.begin(74880);
  Serial.println(params);
  conf.setDescription(params);
  conf.readConfig();
  Serial.println(conf.getInt("expTime"));
  initWiFi();
  char dns[30];
  sprintf(dns,"%s.local","paintball_boom");
  if (MDNS.begin(dns)) {
    Serial.println("MDNS responder gestartet");
  }
  server.on("/",handleRoot);
  server.begin(80);

  lcd.begin(21,22);
  lcd.backlight();
  pinMode(redLED, OUTPUT);  //set the LED as an output
  pinMode(yellowLED, OUTPUT);
  pinMode(powerpin,OUTPUT);
  pinMode(speakerpin,OUTPUT);
  lcd.setCursor(0, 0);
  lcd.print("*******");
  lcd.setCursor(0, 0);
 // pinMode(speakerpin, OUTPUT);
}

void countdown(int timesincestart) {
  lcd.clear();
  lcd.print("*******");
  lcd.setCursor(0, 1);
  lcd.print("BOMB ARMED");
  int fixCountdownTime = conf.getInt("expTime");
  while (true) {

    char whichKey;
    whichKey = keyget();
    if (whichKey == '*' || whichKey == '#') {
      position = 0;
    }
    if (whichKey == password[position]) {
      Serial.print("Now on position ");
      Serial.print(position);
      Serial.print("from");
      Serial.println(strlen(password));
      position ++;
    }
    if (position == strlen(password)) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("  BOMB DISARMED");
      lcd.setCursor(0, 1);
      lcd.print("    CT'S WIN");
      while(1){}
    }

    lcd.setCursor(0, 1);
    int timeleft = fixCountdownTime - (millis() / 1000 - timesincestart);
    lcd.print("BOMB ARMED ");
    if(timeleft < 10){
      lcd.print("00"+String(timeleft));
    }else{
      if(timeleft < 100){
        lcd.print("0"+String(timeleft));
      }else{
        lcd.print(timeleft);
      }
    }
    if (timeleft == 9) {
      lcd.setCursor(1, 12);
      lcd.print(" ");
    }
    if(timeleft == 120){
      interval = 1500;
    }
    if(timeleft == 60){
      interval = 1100;
    }
    if(timeleft == 30){
      interval = 1000;
    }

    if (timeleft == 23) {
      interval = 900;
    }

    if (timeleft == 15) {
      interval = 800;
    }

    if (timeleft == 10) {
      interval = 700;
    }

    if (timeleft == 8) {
      interval = 550;
    }

    if (timeleft == 6) {
      interval = 400;
    }
    if(timeleft == 4){
      interval = 300;
    }
    if(timeleft == 3){
      interval = 200;
    }

    int currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {

      previousMillis = currentMillis;

      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }

      digitalWrite(yellowLED, ledState);
      digitalWrite(speakerpin, HIGH);
      toneActivation = millis();
    }

    if(millis() - toneActivation >= dur){
      digitalWrite(speakerpin, LOW);
    }
    if (timeleft <= 0) {
      digitalWrite(yellowLED, LOW);
      digitalWrite(redLED, HIGH);

      
      lcd.setCursor(0, 0);
      lcd.print("TERRORISTS WIN      ");
      lcd.setCursor(0, 1);
      lcd.print("BOMB DETONATED     ");
      digitalWrite(powerpin,LOW);

      digitalWrite(speakerpin, HIGH);
      delay(2000);
      digitalWrite(speakerpin, LOW);
      while (1);
    }


  }
  return;
}

char keyget() {
  char whichKey = myKeypad.getKey(); //define which key is pressed with getKey

  if (whichKey >= '0' && whichKey <= '9') {
    codepos++;
  }

  if (whichKey == '*' || whichKey == '#') {

    position = 0;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" ENTRY RESET!");
    delay(100);
    lcd.clear();
    lcd.print("*******");
    codepos = 0;
    lcd.setCursor(0, 0);
  }

  if (codepos >= 7) {
    entryReset();
    
  }

  lcd.setCursor(codepos - 1, 0);
  if (whichKey != NO_KEY) {

    if (whichKey == '1') {
      //tone(speakerpin, freqKey, dur);
      lcd.print(1);
    }

    if (whichKey == '2') {
      //tone(speakerpin, freqKey, dur);
      lcd.print(2);
    }

    if (whichKey == '3') {
      //tone(speakerpin, freqKey, dur);
      lcd.print(3);
    }

    if (whichKey == '4') {
      //tone(speakerpin, freqKey, dur);
      lcd.print(4);
    }

    if (whichKey == '5') {
      //tone(speakerpin, freqKey, dur);
      lcd.print(5);
    }

    if (whichKey == '6') {
      //tone(speakerpin, freqKey, dur);
      lcd.print(6);
    }

    if (whichKey == '7') {
      //tone(speakerpin, freqKey, dur);
      lcd.print(7);
    }

    if (whichKey == '8') {
      //tone(speakerpin, freqKey, dur);
      lcd.print(8);
    }

    if (whichKey == '9') {
      //tone(speakerpin, freqKey, dur);
      lcd.print(9);

    }

    if (whichKey == '0') {
      //tone(speakerpin, freqKey, dur);
      lcd.print(0);
    }
  }
  return whichKey;
}
void entryReset(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" ENTRY RESET!");
    delay(100);
    lcd.clear();
    lcd.print("*******");
    codepos = 0;
    lcd.setCursor(0, 0);
}

void loop() {
  // put your main code here, to run repeatedly:
  

  
  char whichKey;
  whichKey = keyget();
  if (whichKey == '*' || whichKey == '#') {
    Serial.println("Loop entry reset");
    position = 0;

    entryReset();
  }
  if (whichKey == password[position]) {

    position ++;
  }
  if (position == strlen(password)) {
    position = 0;
    int timesincestart = (millis()) / 1000;
    countdown(timesincestart);
  }
  if ((millis() - previousmillispower) > 400 && powerOn == true) 
    {
      previousmillispower = millis();
      digitalWrite(powerpin,LOW);
      powerOn = false;
    }
  if ((millis() - previousmillispower) > powerintervalmax && powerOn == false) 
    {
      previousmillispower = millis();
      digitalWrite(powerpin,HIGH);
      powerOn = true;
    }
}
