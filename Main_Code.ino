#include <SoftwareSerial.h>

// Настройка пинов
#define NUM_SENSORS 2          // Количество датчиков AMP-B025
const int soilMoisturePins[NUM_SENSORS] = {A0, A1}; // Аналоговые пины для датчиков
const int relayPins[NUM_SENSORS] = {5, 6};          // Цифровые пины для реле

// Настройка Bluetooth
SoftwareSerial BTSerial(10, 11); // RX, TX (пины Arduino)

// Переменные для хранения данных
int minMoisture = 300;        // Минимальная влажность почвы (по умолчанию)
int maxMoisture = 700;        // Максимальная влажность почвы (по умолчанию)

// Флаги для управления логикой
bool relayState[NUM_SENSORS] = {false}; // Состояние каждого реле
unsigned long lastMoistureLogTime = 0; // Время последнего вывода списка влажности

void setup() {
  // Инициализация Bluetooth для отладки
  BTSerial.begin(9600);       // Инициализация Bluetooth
  Serial.begin(9600);         // Для отладки через USB

  // Настройка пинов реле
  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW); // Все реле выключены по умолчанию
  }

  // Отправка приветственного сообщения через Bluetooth
  BTSerial.println("Система автоматического полива с 2 датчиками и 2 реле запущена!");
  BTSerial.println("Отправьте HELP для просмотра списка команд.");
}

void loop() {
  // Чтение данных через Bluetooth
  if (BTSerial.available()) {
    String command = BTSerial.readStringUntil('\n');
    command.trim(); // Убираем лишние пробелы и символы новой строки
    parseCommand(command); // Обработка команды
  }

  // Чтение данных с датчиков влажности почвы
  int moistureValues[NUM_SENSORS];
  for (int i = 0; i < NUM_SENSORS; i++) {
    moistureValues[i] = readMoisture(soilMoisturePins[i]); // Чтение усреднённого значения

    // Отладка: Вывод значений влажности
    Serial.print("Датчик ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(moistureValues[i]);

    // Логика управления реле
    if (moistureValues[i] <= minMoisture) {
      if (!relayState[i]) { // Включаем реле, только если оно выключено
        digitalWrite(relayPins[i], HIGH); // Включаем реле
        relayState[i] = true;
        BTSerial.print("Реле ");
        BTSerial.print(i + 1);
        BTSerial.println(" включено (влажность почвы ниже или равна минимальной)");
      }
    } else if (moistureValues[i] >= maxMoisture) {
      if (relayState[i]) { // Выключаем реле, только если оно включено
        digitalWrite(relayPins[i], LOW); // Выключаем реле
        relayState[i] = false;
        BTSerial.print("Реле ");
        BTSerial.print(i + 1);
        BTSerial.println(" выключено (влажность почвы выше или равна максимальной)");
      }
    }
  }

  // Вывод списка влажности раз в секунду
  unsigned long currentTime = millis();
  if (currentTime - lastMoistureLogTime >= 1000) { // Раз в секунду
    logMoisture(moistureValues);
    lastMoistureLogTime = currentTime;
  }

  delay(100); // Небольшая задержка для стабильности
}

// Функция для усреднения значений датчиков
int readMoisture(int pin) {
  int sum = 0;
  const int numReadings = 10; // Количество измерений для усреднения
  for (int i = 0; i < numReadings; i++) {
    sum += analogRead(pin);
    delay(10); // Небольшая задержка между измерениями
  }
  return sum / numReadings; // Возвращаем среднее значение
}

// Функция для обработки команд с Bluetooth
void parseCommand(String command) {
  if (command.startsWith("PMI=")) {
    minMoisture = command.substring(4).toInt();
    BTSerial.print("Установлено минимальное значение влажности почвы: ");
    BTSerial.println(minMoisture);
  } else if (command.startsWith("PMA=")) {
    maxMoisture = command.substring(4).toInt();
    BTSerial.print("Установлено максимальное значение влажности почвы: ");
    BTSerial.println(maxMoisture);
  } else if (command.equalsIgnoreCase("HELP")) {
    // Вывод справки по командам
    BTSerial.println("\n--- СПРАВКА ПО КОМАНДАМ ---");
    BTSerial.println("PMI=X: Установить минимальную влажность почвы (X в единицах ADC)");
    BTSerial.println("PMA=X: Установить максимальную влажность почвы (X в единицах ADC)");
    BTSerial.println("HELP: Вывести эту справку");
    BTSerial.println("-----------------------------");
  } else {
    BTSerial.println("Неизвестная команда. Отправьте HELP для просмотра списка команд.");
  }
}

// Функция для вывода списка влажности
void logMoisture(int values[]) {
  BTSerial.println("Текущие значения влажности почвы:");
  for (int i = 0; i < NUM_SENSORS; i++) {
    BTSerial.print("Датчик ");
    BTSerial.print(i + 1);
    BTSerial.print(": ");
    BTSerial.println(values[i]);
  }
}