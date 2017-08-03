/*
 * Bu kod YTU IEEE Kapı otomasyon sistemi için yazılmıştır.
 * Son güncellenme tarihi: 3 Ağustos 2017
 * Son güncelleme değişiklikleri:
 *  Internal Id numarası eklendi.
 *  SD kart başlamadan güvenli modda kapı açma eklendi.
 *
 * Sistem Dosyaları:
 * ID.txt:
 *  Bu dosya kullanıcı Id'lerini saklar.
 * devId.txt:
 *  Bu dosya odaya başkalarının girişini kapatma yetkisi olan kişileri tutar
 * locker.txt:
 *  Bu dosya odaya başkalarının girişini kapatan kişiyi tutar
 *
 *  Sistem kullanımı:
 *
 *
 *  Kullanıcı ekleme:
 *    Master Kart 1 kez okutulur. Sistem master moda geçer. Master mod açıkken eklenmek istenen kartlar okutulur. Okutulan kartlar kaydedilir.
 *    İşlem bittiğinde master mod kapatılarak tekrar normal kullanıma dönülür.
 *  Kullanıcı silme:
 *    Master kart 1 kez okutulur. Sistem master moda geçer. Master mod açıkken sililmesini istenen kartlar 3 kez art arda okutulur. Okunan kartlar silinir.
 *    İşlem bittiğinde master mod kapatılarak tekrar normal kullanıma dönülür.
 *  Oturum açma:
 *    Kayıtlı kullanıcı kartını 1 kez okutarak oturum açar.Kapı 5 saniye boyunca açık kalır daha sonra kitlenir. Kayıtlı başka bir kişi kartını tekrar okutana
 *    kadar oturum açık kalır. Oturum açık iken kapıdaki butona basılarak kapı açılabilir. Oturum kapalıyken buton ile kapı açılmaz.
 *  Odayı diğer kullanıcıların girişine kapatma:
 *    Bu özelliği kullanmak için devID.txt dosyasına ID manuel olarak eklenmeli. Yetkili kişi kartını kapıya 6 kez okutursa kapıyı sadece bu kullanıcı açabilir.
 *    Oda, bu kullanıcı tekrar kartını okutana kadar açılmaz.
 *
 *  !!Geliştirici uyarıları!!:
 *    Master kart ID si masterID değişkenine yazılmalı.
 *    Kullanıcı eklerken veya silerken kart eklenme uyarısı geldiğinde master moddan çıkın ve tekrar girin.
 *    Sistem Sd kart okunana kadar başlamaz. Sd kart okunduğunda uyarı ışığı bir süre yanıp söner.
 *    devId dosyasına kişi eklemek için Sd kartın içine devId.txt dosyası açılmalı ve Id numaraları manuel olarak eklenmeli.
 *
 *
 *    Seslerin anlamları:
 *     *    Kısa beep     = 100ms HIGH, 100ms LOW
 *     **   Orta beep     = 300ms HIGH, 300ms LOW
 *     ***  Uzun beep     = 600ms HIGH, 400ms LOW
 *     **** Çok uzun beep = 1sn   HIGH, 600ms LOW
 *
 *    KOMUTLAR:                               SESLER:
 *   Master mod kapalı                        **
 *   Master mod aktif                         ** / **
 *   Oda açıldı                               * / *
 *   Oda kapandı                              * / * / *
 *   Buton ile açıldı                         * / *
 *   Buton ile açılmadı                       *** / *** / *** / ***
 *   izinsiz giriş                            *** / *** / *** / ***
 *   Oda girişleri kapandı                    **** / **** / ****
 *   Oda girişi yasak. Kapı açılmadı.         **** / **** / **** / ****
 *   Oda giriş yasağı kaldırıldı              **** / ****
 *   id eklendi                               ****
 *   id silmek için 2 kere daha okutun        *** / ***
 *   id silmek için 1 kez daha okutun         ***
 *   id silindi                               ** / ** / **
 *   
 *   
 *   GPL Licence:
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Written by Buğra Tufan, 3 August 2017
 *
 */

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

const PROGMEM String defaultInternalID  = "1317827188106";//SD kart arızaları için Internal ID numarası. Odayı güvenli moda açar.
const PROGMEM String masterID = "ADD MASTER_CARD_ID HERE";//Master Card ID'si (Master Kartı değiştirmek için sadece bu id'nin değişmesi yeterli
String lockerID = "";//Oda girişini başkalarına kitleyen kişi
boolean masterAuthorization = false;// Master kartın okunduğunu ve kişi ekleme çıkarma moduna girdiğini gösterir
boolean doorState = false;// Kapının bir kişi tarafından açıldığını ve kapının kartsız açılabildiğini gösterir
boolean newId = false; //Kartın sürekli okunmaması için gereken bir değişken
String lastID = "";// Okunan karttan önceki kartın numarasıdır. Kartın art arda okunup okunmadığını anlamak için vardır.
int idCounter = 0;// Bir kartın art arda kaç defa okunduğunu sayar
boolean doorLockedFromSomeone = false;// Oda girişlerinin herkese kapandığını gösterir

void setup() {
  pinMode(doorPin,OUTPUT);
  pinMode(LED_Pin,OUTPUT);
  pinMode(buzzer,OUTPUT);
  digitalWrite(doorPin,HIGH);
  Serial.begin(9600);
  saveLog("","Sd bekleniyor");
  SPI.begin();
  rfid.init();
  while(SD.begin(9)==0){
    String ID = readRFID();
    if(ID.length() == 0 ){
      if(ID == defaultInternalID){
        saveLog(ID,"Sd kart baslatilamadi.");
        saveLog(ID,"Oda guvenli modda acıldı");
        alertSound(2, 100, 100);
        unlockDoor();
      }
    }
  }
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
   String ID = readRFID();// Kartı okutan kişinin ID'si
   digitalWrite(LED_Pin, doorState);

   boolean buttonState = digitalRead(buttonPin);
   if(buttonState){
     if(doorState){
       unlockDoor();
       saveLog(ID,"Kapi acildi");
       alertSound(2, 100, 100);
     }
     else{
       saveLog(ID,"Oda kapali");
       alertSound(4, 600, 400);
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
        alertSound(2,300,300);
      }
      else if(masterAuthorization){
        saveLog(ID, "master mode kapali");
        alertSound(1,300,300);
      }
      masterAuthorization = !masterAuthorization;
    }
    else{
      if( ID == defaultInternalID){
        unlockDoor();
        saveLog(ID,"Oda Guvenli Modda acıldı");
        alertSound(2,100,100);
      }
      else if(masterAuthorization){
          if(!scanId(ID)){
            addId(ID);
            saveLog("ID", "ekleme isteği");
            alertSound(1,1000,0);
            idCounter = 0;
          }
          else{
            switch(idCounter){
              case 0:
                saveLog(ID, "Bu kart zaten kayitli");
                saveLog(ID, "Silmek icin 2 kez daha okutun");
                alertSound(2,600,400);
                break;
              case 1:
                saveLog(ID, "Bu kart zaten kayitli");
                saveLog(ID,"Silmek icin 1 kez daha okutun");
                alertSound(1,600,0);
                break;
              case 2:
                saveLog(ID,"Silme istegi");
                idCounter = 0;
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
              idCounter = 0;
              alertSound(2,1000,1000);
              unlockDoor();
            }
            else{
              saveLog(ID,"Su an oda girisleri kapalidir.");
              alertSound(4,1000,1000);
            }
          }
          else{
            doorState = true;
            saveLog(ID,"Oda acildi");
            alertSound(2,100,100);
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
            alertSound(3,1000, 1000);
            delay(1000);
          }
          else{
            doorState = false;
            saveLog(ID, "Oda kilitlendi");
            alertSound(3,100,100);
            delay(1000);

          }
        }
      }
      else{
        if(idCounter>3 && idCounter<=7){
          saveLog(ID, "Kaybol burdan gozum gormesin");
          alertSound(4,1000,400);
        }
        else if(idCounter>7){
          saveLog(ID, "Burayi terk et");
          alertSound(4,2000,500);
        }
        else{
          saveLog(ID, "Izinsiz giris");
          alertSound(4,600,400);
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
      alertSound(3,300,300);
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
    digitalWrite(LED_Pin,HIGH);
    delay(delayTime_H);
    digitalWrite(buzzer,LOW);
    digitalWrite(LED_Pin,LOW);
    delay(delayTime_L);
  }
}
