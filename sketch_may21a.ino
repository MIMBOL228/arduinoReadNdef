/*
  Reading NDEG tags (Text and URL) from NFC Tags
  Чтение NDEF (Текст и ссылки) из метки

  Author: MIMBOL (https://github.com/mimbol228)
*/

#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         9                       // Пин rfid модуля RST
#define SS_PIN          10                      // Пин rfid модуля SS

MFRC522 rfid(SS_PIN, RST_PIN);                  // Объект rfid модуля

uint8_t globalDataBlock[140];                   // Массив байтов от метки

void setup() {
  Serial.begin(9600);                           // Инициализация Serial

  SPI.begin();                                  // Инициализация SPI

  rfid.PCD_Init();                              // Инициализация модуля
  rfid.PCD_SetAntennaGain(rfid.RxGain_min);     // Установка усиления антенны
  rfid.PCD_AntennaOff();                        // Перезагружаем антенну
  rfid.PCD_AntennaOn();                         // Включаем антенну
}

void loop() {
  static uint32_t rebootTimer = millis();       // Важный костыль против зависания модуля!
  if (millis() - rebootTimer >= 1000) {         // Таймер с периодом 1000 мс
    rebootTimer = millis();                     // Обновляем таймер
    digitalWrite(RST_PIN, HIGH);                // Сбрасываем модуль
    delayMicroseconds(2);                       // Ждем 2 мкс
    digitalWrite(RST_PIN, LOW);                 // Отпускаем сброс
    rfid.PCD_Init();                            // Инициализируем заного
  }

  if (!rfid.PICC_IsNewCardPresent()) return;    // Если новая метка не поднесена - вернуться в начало loop
  if (!rfid.PICC_ReadCardSerial()) return;      // Если метка не читается - вернуться в начало loop
  
  for(int g = 5; g < 40; g++){                  // Проходимся по секторам поочерёдно от пятого по 39й
    uint8_t dataBlock[18];                      // Буфер для чтения
    uint8_t size = sizeof(dataBlock);           // Размер буфера
    rfid.MIFARE_Read(g, dataBlock, &size);      // Читаем сектор
    for (uint8_t i = 0; i < 4; i++) {           // Проходимся по блокам поочерёдно
      globalDataBlock[(g)*4+i] = dataBlock[i];  // И кладём данные в общий массив
    }
  }

  int end = searchIndex(0xFE);                  // Ищем конец NDEF
  int start = 23;                               // Старт NDEF обычно в 23м байте, но есть исключения

  switch(globalDataBlock[21]){                  // Анализируем тип тега по 21му байту
    case 0x54:
      Serial.println("Type: Text");
      start += 2;
      break;
    case 0x55:
      Serial.println("Type: Link");
      break;
    case 0x00:
      Serial.println("Type: Empty");
      break;
    default:
      Serial.println("Type: Unkown");
      break;
  }

  for (int i = start; i < end; i++) {           // Проходимся по всему тегу и читаем байтики в сериал
    Serial.write(globalDataBlock[i]);
  }

  Serial.println("");
  Serial.println("----------------------------");

  rfid.PICC_HaltA();                            // Заканчиваем работу с меткой
  rfid.PCD_StopCrypto1();
}

int searchIndex(uint8_t value){ // Линейный поиск в массиве globalDataBlock
  for (int i=0; i< sizeof(globalDataBlock); i++) 
    if (globalDataBlock[i] == value) 
      return i;

  return -1;
}