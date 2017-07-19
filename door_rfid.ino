/*
 * Bu kod YTU IEEE Kapı otomasyon sistemi için yazılmıştır.
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
 *   !!Geliştirici uyarıları!!:
 *   
 *    Kullanıcı eklerken veya silerken kart eklenme uyarısı geldiğinde master moddan çıkın ve tekrar girin. 
 *    Sistem Sd kart okunana kadar başlamaz. Sd kart okunduğunda uyarı ışığı bir süre yanıp söner.
 *    
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


String masterID = "1317827188106";//Master Card ID'si (Master Kartı değiştirmek için sadece bu id'nin değişmesi yeterli
String lockerID = "";//Oda girişini başkalarına kitleyen kişi
boolean masterAuthorization = false;// Master kartın okunduğunu ve kişi ekleme çıkarma moduna girdiğini gösterir
boolean doorState = false;// Kapının bir kişi tarafından açıldığını ve kapının kartsız açılabildiğini gösterir
boolean newId = false; //Kartın sürekli okunmaması için gereken bir değişken
String lastID = "";// Okunan karttan önceki kartın numarasıdır. Kartın art arda okunup okunmadığını anlamak için vardır.
int idCounter = 0;// Bir kartın art arda kaç deva okunduğunu sayar
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
   String ID = readRFID();// Kartı okutan kişinin ID'si
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
            saveLog("ID", "ekleme isteği");
            alertSound(1,100,0);
            idCounter = 0;
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
          saveLog(ID, "Burayi terk et");
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
    digitalWrite(LED_Pin,HIGH);
    delay(delayTime_H);
    digitalWrite(buzzer,LOW);
    digitalWrite(LED_Pin,LOW);
    delay(delayTime_L);
  }
  delay(100);
}

