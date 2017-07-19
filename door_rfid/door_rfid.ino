#define doorPin 2
#define LED_Pin 10
#define buttonPin A3
#define buzzer 8
#define SD_CS 9
#define RFID_CS 2
#define RFID_RST A2

#include <SD.h>

#include <SPI.h>
#include <RFID.h>

File SD_File;
RFID rfid(RFID_CS,RFID_RST); 

String masterID = "1317827188106";
String lockerID = "";
boolean masterAuthorization = false;
boolean doorState = false;
boolean newId = false; 
String lastID = "";
int idCounter = 0;
boolean doorLockedFromSomeone = false;

void setup() {
  pinMode(doorPin,OUTPUT);
  pinMode(LED_Pin,OUTPUT);
  pinMode(buzzer,OUTPUT);
  digitalWrite(doorPin,HIGH);
  Serial.begin(9600);
  saveLog("","Sd bekleniyor");
  SPI.begin(); 
  rfid.init();
  while(SD.begin(9)==0);
  saveLog("","Sistem baslatildi");
  for(byte a = 0; a<3; a++){
    digitalWrite(LED_Pin,HIGH);
    delay(100);  
    digitalWrite(LED_Pin,LOW);
    delay(100);  
  }
  String lockerData = lockerDataSd();
  if(lockerData.length() > 5){
    lockerID =  lockerData;  
    doorLockedFromSomeone = true;
    saveLog(lockerID,"Oda girisleri kapali");
  }

  

}

void loop() {
   String ID = readRFID();
   digitalWrite(LED_Pin, doorState);
   
   boolean buttonState = digitalRead(buttonPin);
   if(buttonState){
     if(doorState){
       unlockDoor();
     }
   }
  if(ID.length() == 0 ){
    newId = true;
  }
  if(ID.length()>0 && newId == true){
    newId = false;
    if(ID.equals(lastID)){
      idCounter++;
    }
    else{
      idCounter = 0;
    }
    lastID = ID;
    if( ID == masterID ){
      if(!masterAuthorization){
        saveLog(ID, "master mode aktif");
        alertSound(2,100,50);
      }
      else if(masterAuthorization){
        saveLog(ID, "master mode kapali");
        alertSound(3,100,50);
      }
      masterAuthorization = !masterAuthorization; 
    }
    else{
      if(masterAuthorization){
          if(!scanId(ID)){
            addId(ID);
          }
          else{
            switch(idCounter){
              case 0:
                saveLog(ID, "Bu kart zaten kayitli");
                saveLog(ID, "Silmek icin 2 kez daha okutun");
                alertSound(1,300,0);
                break;
              case 1:
                saveLog(ID, "Bu kart zaten kayitli");
                saveLog(ID,"Silmek icin 1 kez daha okutun");
                alertSound(1,300,0);
                break;
              case 2:
                saveLog(ID,"Silme istegi");
                deleteId(ID);
                break;
            }
            
          }
      }
      else if(scanId(ID)){
        if(!doorState){
          if(doorLockedFromSomeone){
            if(ID == lockerID){
              doorState = true;
              changeLockerState();
              doorLockedFromSomeone = false;
              saveLog(ID,"Oda izinleri tekrar acildi, Kapi acildi");
              alertSound(3,300,300);
              unlockDoor();
            }
            else{
              saveLog(ID,"Su an oda girisleri kapalidir.");
              alertSound(1,500,0);
            }
          }
          else{
            doorState = true;
            saveLog(ID,"Oda acildi");
            alertSound(1,100,100);
            unlockDoor();
          }
        }
        else{
          
          if(idCounter > 4 && scanDevId(ID)){
            doorState = false;
            doorLockedFromSomeone = true;
            lockerID = ID;
            saveLog(ID, "Oda girisleri kapatildi");
            saveLockerId(ID);
            alertSound(4,300,300);
            delay(1000);          
          }
          else{
            doorState = false;
            saveLog(ID, "Oda kilitlendi");
            alertSound(2,100,100);
            delay(1000);
            
          }
        }
      }
      else{
        if(idCounter>3 && idCounter<=7){
          saveLog(ID, "Kaybol burdan gozum gormesin");
          alertSound(3,500,300);
        }
        else if(idCounter>7){
          saveLog(ID, "Kaybol burdan gozum gormesin");
          alertSound(3,2000,500);
        }
        else{
          saveLog(ID, "Izinsiz giris");
          alertSound(5,100,100);
        }
      }
    }
  }
}

String readRFID(){
  String ID = "";
  if(rfid.isCard()){
    if (rfid.readCardSerial()) {
      for( byte a = 0; a<5; a++){
        ID += String(rfid.serNum[a]);  
      }
    }
  }
  rfid.halt();
  return ID;
}

void unlockDoor(){
  digitalWrite(doorPin,LOW);
  for(byte a = 0; a<25; a++){
    digitalWrite(LED_Pin,HIGH);
    delay(50);  
    digitalWrite(LED_Pin,LOW);
    delay(50);  
  }
  digitalWrite(doorPin,HIGH);
}
void saveLog( String ID , String logString ){
  Serial.print(ID);
  Serial.println(" "+logString);
}
boolean addId( String ID ){
  SD_File = SD.open("ID.txt", FILE_WRITE);
  if (SD_File) {
    SD_File.println(ID);
    saveLog("ID", "eklendi");
    alertSound(1,100,0);
    SD_File.close();
    return true;
  } 
  else {
    saveLog("" , "Id dosya acilmadi");
    return false;
  }
}

boolean scanId( String ID ){
  SD_File = SD.open("ID.txt");
  if (SD_File) {
    while (SD_File.available()) {
      String line = SD_File.readStringUntil('\n');
      line.replace("\r", "");
      line.replace("\n", "");
        if( (ID ).equals(line) ){
          SD_File.close();
          return true;
        }
    }
    SD_File.close();
  }
  else {
    saveLog("" , "Id dosya acilmadi");
    }
  return false;
}


boolean scanDevId( String ID ){
  SD_File = SD.open("devID.txt");
  if (SD_File) {
    while (SD_File.available()) {
      String line = SD_File.readStringUntil('\n');
      line.replace("\r", "");
      line.replace("\n", "");
        if( (ID ).equals(line) ){
          SD_File.close();
          return true;
        }
    }
    SD_File.close();
  }
  else {
    saveLog("" , "devID dosya acilmadi");
    }
  return false;
}


boolean deleteId( String ID ){
  String newData = "";
  SD_File = SD.open("ID.txt");
  if (SD_File) {
    while (SD_File.available()) {
      String line = SD_File.readStringUntil('\n');
      line.replace("\r", "");
      line.replace("\n", "");
        if( !ID.equals(line) ){
          newData = line+"\n";
        }
    }
    SD_File.close();
    SD.remove("ID.txt");
    SD_File = SD.open("ID.txt", FILE_WRITE);
    if (SD_File) {
      SD_File.print(newData);
      saveLog("ID", "silindi");
      alertSound(3,100,40);
      SD_File.close();
      return true;
    } 
    else {
      saveLog("" , "Id dosya acilmadi");
    }
  }
  return false;
}

String lockerDataSd(){
  SD_File = SD.open("LOCKER.TXT");
  String line = "";
  if (SD_File) {
    while(SD_File.available()) {
      line += String(SD_File.readStringUntil('\n'));
      line.replace("\r", "");
      line.replace("\n", "");
    }
    SD_File.close();
    return line;
  }
  else {
    saveLog("" , "locker dosya acilmadi");
    }
  return line;
}
boolean saveLockerId(String ID){
   SD.remove("LOCKER.TXT");
    SD_File = SD.open("LOCKER.TXT", FILE_WRITE);
    if (SD_File) {
      SD_File.print(ID);
      SD_File.close();
      return true;
    } 
    else {
      saveLog("" , "locker dosya acilmadi");
    }
    return false;
}

boolean changeLockerState(){
  SD.remove("LOCKER.TXT");
    SD_File = SD.open("LOCKER.TXT", FILE_WRITE);
    if (SD_File) {
      SD_File.print("0\n");
      SD_File.close();
      return true;
    } 
    else {
      saveLog("" , "locker dosya acilmadi");
    }
    return false;
}

void alertSound(byte amount, int delayTime_H,int delayTime_L){
  for( byte a = 0; a<amount; a++){
    digitalWrite(buzzer,HIGH);
    delay(delayTime_H);
    digitalWrite(buzzer,LOW);
    delay(delayTime_L);
  }
  delay(100);
}

