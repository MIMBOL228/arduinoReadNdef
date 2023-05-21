#include <SPI.h>
#include <MFRC522.h>
#define RST_PIN         9        // Пин rfid модуля RST
#define SS_PIN          10       // Пин rfid модуля SS

MFRC522 rfid(SS_PIN, RST_PIN);   // Объект rfid модуля
MFRC522::MIFARE_Key key;         // Объект ключа
MFRC522::StatusCode status;      // Объект статуса
uint8_t globalDataBlock[92];

void setup() {
  Serial.begin(9600);              // Инициализация Serial
  SPI.begin();                     // Инициализация SPI
  rfid.PCD_Init();                 // Инициализация модуля
  rfid.PCD_SetAntennaGain(rfid.RxGain_min);  // Установка усиления антенны
  rfid.PCD_AntennaOff();           // Перезагружаем антенну
  rfid.PCD_AntennaOn();            // Включаем антенну
  for (byte i = 0; i < 6; i++) {   // Наполняем ключ
    key.keyByte[i] = 0xFF;         // Ключ по умолчанию 0xFFFFFFFFFFFF
  }
}
void loop() {
  // Занимаемся чем угодно
  static uint32_t rebootTimer = millis(); // Важный костыль против зависания модуля!
  if (millis() - rebootTimer >= 1000) {   // Таймер с периодом 1000 мс
    rebootTimer = millis();               // Обновляем таймер
    digitalWrite(RST_PIN, HIGH);          // Сбрасываем модуль
    delayMicroseconds(2);                 // Ждем 2 мкс
    digitalWrite(RST_PIN, LOW);           // Отпускаем сброс
    rfid.PCD_Init();                      // Инициализируем заного
  }

  if (!rfid.PICC_IsNewCardPresent()) return;  // Если новая метка не поднесена - вернуться в начало loop
  if (!rfid.PICC_ReadCardSerial()) return;    // Если метка не читается - вернуться в начало loop
  
  for(int g = 5; g < 28; g++){
    uint8_t dataBlock[18];                          // Буфер для чтения
    uint8_t size = sizeof(dataBlock);               // Размер буфера
    rfid.MIFARE_Read(g, dataBlock, &size); // Читаем 6 блок в буфер
    for (uint8_t i = 0; i < 4; i++) {
      globalDataBlock[(g)*4+i] = dataBlock[i];
    }
  }

  int end = searchIndex(0xFE);
  int start = 23;
  switch(globalDataBlock[21]){
    case 0x54:
      Serial.println("Type: Text");
      start += 2;
      break;
    case 0x55:
      Serial.println("Type: Link");
      break;
    default:
      Serial.println("Type: Unkown");
      break;
  }
  for (int i = start; i < end; i++) {
    Serial.write(globalDataBlock[i]);
  }
  Serial.println("");
  Serial.println("----------------------------");

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
int searchIndex(uint8_t value){
  for (int i=0; i< 92; i++) 
    if (globalDataBlock[i] == value) 
      return i;
  return -1;
}