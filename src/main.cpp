#include <WiFi.h>
#include <Wire.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include "freertos/queue.h"
#include <Bounce2.h>
#include <ESP32Time.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <sqlite3.h> // Biblioteca SQLite
#include <SPI.h>
#include <SD.h>

//pio run --target uploadfs

/*--------------------------------- Definiciones y Configuración ---------------------------------*/

// Pines
#define ledWifiOutput 21 // LED indicador de conexión WiFi
#define relay1_Alarm 12  // Relé para alarma de temperatura de máquina

// Configuración del servidor NTP para sincronización de tiempo
const char* ntpServer = "time.google.com"; // Servidor NTP
const long gmtOffset_sec = -5 * 3600;  // GMT-5 para Perú
const int daylightOffset_sec = 0;
ESP32Time rtc_esp32;

// Configuración de WiFi
const char *wifi_ssid = "MOVISTAR_1C30";
const char *wifi_password = "PatfPhEeJuT53p8A2Hr5";
IPAddress local_IP(192, 168, 1, 150);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primary_dns(8, 8, 8, 8);
IPAddress secondary_dns(8, 8, 4, 4);

// Variables para reconexión WiFi
unsigned long ultimoIntento = 0;
int countReconectWifi = 0;
const int intervaloReconexion = 1000;

// Indicador para ejecutar configuraciones una sola vez
bool funcionYaEjecutada = false;

// Intervalo para guardar en memoria los datos de `tempMachine`
int timeupdatewifi = 1000;
const unsigned long interval2 = timeupdatewifi;
unsigned long previousTime2 = 0;

// Variables para la temperatura de la máquina y alarma
float tempMachine;
float setTemperature = 0.0;

// Inicialización de servidores y bases de datos
AsyncWebServer server(80);
unsigned long ota_progress_millis = 0;
char *zErrMsg = 0; // Mensaje de error para SQLite
sqlite3 *db;

// Límite de registros en la tabla antes de crear una nueva base de datos y tabla
const int tableRecordLimit = 3600;  //10000

/*-------------------------- Funciones de Configuración --------------------------*/

// Configuración inicial del servidor NTP para sincronizar el tiempo en ESP32
void runSetupServerNTP() {
  if (!funcionYaEjecutada) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("Actualizando tiempo en ESP32 desde servidor NTP");
    delay(2000);

    Serial.print("La hora actualizada es: ");
    Serial.println(rtc_esp32.getTime("%A, %B %d %Y %H:%M:%S"));
    funcionYaEjecutada = true;
  }
}

// Maneja la reconexión al WiFi en caso de desconexión
void reconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long tiempoActual = millis();
    if (tiempoActual - ultimoIntento >= intervaloReconexion) {
      ultimoIntento = tiempoActual;

      if (countReconectWifi == 0) {
        Serial.print("Reconectando a la red Wi-Fi...");
        WiFi.disconnect();

        if (!WiFi.config(local_IP, gateway, subnet, primary_dns, secondary_dns)) {
          Serial.print("Error al configurar la IP estática y DNS");
        }

        WiFi.begin(wifi_ssid, wifi_password);
      }

      Serial.print(".");
      countReconectWifi++;

      if (countReconectWifi > 10) {
        Serial.print("Conexión WiFi fallida, reintentando...");
        countReconectWifi = 0;
      }
    }
  } else {
    if (countReconectWifi > 0) {
      Serial.println("\nConexión WiFi exitosa");
      Serial.print("IP: "); Serial.println(WiFi.localIP());
      Serial.print("SSID: "); Serial.println(WiFi.SSID());
      Serial.print("BSSID: "); Serial.println(WiFi.BSSIDstr());
      Serial.print("Máscara de subred: "); Serial.println(WiFi.subnetMask());
      Serial.print("Puerta de enlace: "); Serial.println(WiFi.gatewayIP());
      Serial.print("DNS 1: "); Serial.println(WiFi.dnsIP(0));
      Serial.print("DNS 2: "); Serial.println(WiFi.dnsIP(1));
      Serial.print("RSSI: "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
    }
    countReconectWifi = 0;

    runSetupServerNTP();
  }
}

// Inicializa el sistema de archivos LittleFS
void initializeLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println("Error al inicializar LittleFS, formateando...");
    if (LittleFS.format()) {
      Serial.println("Formateo de LittleFS completado");
    } else {
      Serial.println("Error al formatear LittleFS");
      return;
    }
    if (!LittleFS.begin()) {
      Serial.println("Error al inicializar LittleFS después de formatear");
      return;
    }
  } else {
    Serial.println("LittleFS inicializado correctamente");
  }
}

// Muestra el espacio utilizado y libre en LittleFS
void displayLittleFSSpace() {
  Serial.print("Almacenamiento flash total: ");
  Serial.println(ESP.getFlashChipSize());
  Serial.print("Almacenamiento flash usado: ");
  Serial.println(ESP.getSketchSize());

  Serial.print("Espacio total BD: ");
  Serial.print(LittleFS.totalBytes() / (1024.0 * 1024.0));
  Serial.println(" MB");

  Serial.print("Espacio usado en BD: ");
  Serial.print(LittleFS.usedBytes() / (1024.0 * 1024.0));
  Serial.println(" MB");

  float percentageUsed = (float)LittleFS.usedBytes() / (float)LittleFS.totalBytes() * 100;
  Serial.print("Espacio usado de memoria: ");
  Serial.printf("%.2f", percentageUsed);
  Serial.println(" %");

  size_t freeHeap = ESP.getFreeHeap();
  Serial.print("Memoria libre: ");
  Serial.print(freeHeap);
  Serial.println(" bytes");

  Serial.print("Velocidad del procesador: ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");
}

// Carga la temperatura configurada desde un archivo JSON en LittleFS
void loadSetTemperature() {
  if (!LittleFS.exists("/setTemp.json")) {
    Serial.println("El archivo setTemp.json no existe");
    return;
  }
  File file = LittleFS.open("/setTemp.json", "r");
  if (!file) {
    Serial.println("Error al abrir setTemp.json para lectura");
    return;
  }
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.print("Error al deserializar JSON: ");
    Serial.println(error.c_str());
  } else {
    setTemperature = doc["setTemperature"].as<float>();
  }
  file.close();
}

// Guarda la temperatura configurada en un archivo JSON en LittleFS
void saveSetTemperature() {
  File file = LittleFS.open("/setTemp.json", "w");
  if (!file) {
    Serial.println("Error al abrir setTemp.json para escritura");
    return;
  }
  DynamicJsonDocument doc(1024);
  doc["setTemperature"] = setTemperature;
  if (serializeJson(doc, file) == 0) {
    Serial.println("Error al escribir JSON en setTemp.json");
  }
  file.close();
}

/*-------------------------- Funciones de SQLite3 --------------------------*/

// Callback para mostrar resultados de una consulta SQL en la consola serial
static int callback(void *data, int argc, char **argv, char **azColName) {
  Serial.printf("%s: ", (const char *)data);
  for (int i = 0; i < argc; i++) {
    Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  Serial.printf("\n");
  return 0;
}

// Ejecuta una consulta SQL en la base de datos SQLite
int db_exec(sqlite3 *db, const char *sql) {
  Serial.println(sql);
  long start = micros();
  int rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
  if (rc != SQLITE_OK) {
    Serial.printf("Error SQL: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  } else {
    Serial.printf("Operación realizada exitosamente\n");
  }
  Serial.print("Tiempo transcurrido: ");
  Serial.println(micros() - start);
  return rc;
}

// Abre una base de datos SQLite y configura el modo WAL
int openDb(const char *filename, sqlite3 **db) {
  int rc = sqlite3_open(filename, db);
  if (rc) {
    Serial.printf("No se puede abrir la base de datos: %s\n", sqlite3_errmsg(*db));
    return rc;
  } else {
    Serial.printf("Base de datos abierta exitosamente: %s\n", filename);
    db_exec(*db, "PRAGMA journal_mode = WAL;");
  }
  return rc;
}

// Obtiene el valor máximo de ID en la tabla tempData
int getMaxId() {
  const char *sqlMaxId = "SELECT MAX(id) FROM tempData;";
  sqlite3_stmt *stmt;
  int maxId = 0;

  if (sqlite3_prepare_v2(db, sqlMaxId, -1, &stmt, NULL) == SQLITE_OK) {
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      maxId = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
  } else {
    Serial.println("Error al obtener el valor máximo de id.");
  }
  return maxId;
}

// Crea la tabla tempData si no existe
void createTable() {
  const char* sqlCreateTable = "CREATE TABLE IF NOT EXISTS tempData ("
                               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                               "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
                               "temperature REAL);";
  
  if (db_exec(db, sqlCreateTable) == SQLITE_OK) {
    Serial.println("Tabla creada o existente: tempData");
  } else {
    Serial.println("Error al crear la tabla tempData.");
  }
}

// Inserta un nuevo registro en la tabla db_list de la base de datos central
void insertNewDatabase(String dbName, String path) {
  sqlite3 *centralDb;
  
  // Abrir la base de datos central
  if (sqlite3_open("/sd/central.db", &centralDb)) {
    Serial.println("Error al abrir la base de datos central");
    return;
  }

  // Crear la tabla db_list si no existe
  const char* sqlCreateDbListTable = "CREATE TABLE IF NOT EXISTS db_list ("
                                     "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                     "name TEXT, "
                                     "path TEXT);";
  db_exec(centralDb, sqlCreateDbListTable);

  // Insertar el nombre y la ruta de la nueva base de datos en la tabla db_list
  String sql = "INSERT INTO db_list (name, path) VALUES ('" + dbName + "', '" + path + "');";
  char *zErrMsg = 0;
  int rc = sqlite3_exec(centralDb, sql.c_str(), NULL, 0, &zErrMsg);
  if (rc != SQLITE_OK) {
    Serial.print("Error al insertar la nueva base de datos en db_list: ");
    Serial.println(zErrMsg);
    sqlite3_free(zErrMsg);  // Liberar la memoria asignada para el mensaje de error
  }

  // Cerrar la base de datos central
  sqlite3_close(centralDb);
}

// Función para obtener el nombre de la última base de datos desde la tabla db_list
String getLastDatabaseName() {
  sqlite3 *centralDb;
  String lastDbPath;

  // Abrir la base de datos central
  if (sqlite3_open("/sd/central.db", &centralDb)) {
    Serial.println("Error al abrir la base de datos central");
    return lastDbPath;
  }

  // Consulta para obtener el nombre y la ruta de la última base de datos
  const char *sqlLastDb = "SELECT path FROM db_list ORDER BY id DESC LIMIT 1;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(centralDb, sqlLastDb, -1, &stmt, NULL) == SQLITE_OK) {
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      lastDbPath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    }
    sqlite3_finalize(stmt);
  } else {
    Serial.println("Error al obtener el nombre de la última base de datos.");
  }

  // Cerrar la base de datos central
  sqlite3_close(centralDb);

  return lastDbPath;
}



// Definición de la función createDatabaseAndTable
void createDatabaseAndTable() {
  // Generar nombre para la nueva base de datos
  String newDbName = "tempDB_" + rtc_esp32.getTime("%Y%m%d_%H%M%S") + ".db";
  String newDbPath = "/sd/" + newDbName;

  // Cerrar la base de datos actual si está abierta
  if (db) {
    sqlite3_close(db);
    Serial.println("Base de datos anterior cerrada.");
  }
  
  // Crear la nueva base de datos
  if (openDb(newDbPath.c_str(), &db) == SQLITE_OK) {
    Serial.print("Nueva base de datos creada: ");
    Serial.println(newDbName);

    // Registrar el nombre de la base de datos en la base de datos central
    insertNewDatabase(newDbName, newDbPath);

    // Crear la tabla en la nueva base de datos
    createTable();
  } else {
    Serial.println("Error al crear la nueva base de datos.");
  }
}



// Función para crear una nueva base de datos o abrir la última utilizada
void createOrOpenDatabase() {
  // Obtener el nombre de la última base de datos
  String lastDbPath = getLastDatabaseName();

  if (!lastDbPath.isEmpty()) {
    // Intentar abrir la última base de datos
    if (openDb(lastDbPath.c_str(), &db) == SQLITE_OK) {
      Serial.print("Base de datos abierta: ");
      Serial.println(lastDbPath);

      // Crear la tabla si no existe
      createTable();
    } else {
      Serial.println("Error al abrir la última base de datos. Creando nueva base de datos.");
      createDatabaseAndTable();  // Llamada a la función declarada
    }
  } else {
    // Si no se pudo obtener el nombre, crear una nueva base de datos
    createDatabaseAndTable();  // Llamada a la función declarada
  }
}


// Inserta un registro de temperatura en la tabla tempData
void insertData(float temperature) {
  char sqlInsert[128];
  sprintf(sqlInsert, "INSERT INTO tempData (temperature) VALUES (%f);", temperature);
  
  if (db_exec(db, sqlInsert) == SQLITE_OK) {
    Serial.print("Valor de tempMachine insertado en tempData: ");
    Serial.println(temperature);
    Serial.print("Timestamp: ");
    Serial.println(rtc_esp32.getTime("%A, %B %d %Y %H:%M:%S"));
  }
  
  // Verificar si se alcanzó el límite de registros
  if (getMaxId() >= tableRecordLimit) {
    Serial.println("Límite de registros alcanzado. Creando nueva base de datos y tabla.");
    createDatabaseAndTable();
  }
}

// Borra todos los registros de la tabla TempData
void deleteAllData() {
  const char *sqlDelete = "DELETE FROM tempData;";
  db_exec(db, sqlDelete);
  Serial.println("Todos los datos eliminados de tempData");
}

/*-------------------------- Funciones de ElegantOTA --------------------------*/

// Maneja el inicio de la actualización OTA
void onOTAStart() {
  Serial.println("¡Comenzó la actualización OTA!");
}

// Muestra el progreso de la actualización OTA
void onOTAProgress(size_t current, size_t final) {
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("Progreso de OTA: %u bytes, Final: %u bytes\n", current, final);
  }
}

// Maneja el fin de la actualización OTA
void onOTAEnd(bool success) {
  if (success) {
    Serial.println("¡La actualización OTA finalizó exitosamente!");
  } else {
    Serial.println("¡Hubo un error durante la actualización OTA!");
  }
}

/*-------------------------- Funciones de Control y Utilidad --------------------------*/

// Actualiza el estado del LED según la conexión WiFi
void updateLedStatusWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(ledWifiOutput, LOW);  // Apagar LED
  } else {
    digitalWrite(ledWifiOutput, HIGH); // Encender LED
  }
}

// Devuelve la temperatura de la máquina como cadena de texto
String readTempMachine() {
  return String(tempMachine);
}

// Devuelve la temperatura configurada como cadena de texto
String readsetTempAlarm() {
  return String(setTemperature);
}

/*-------------------------- Función para eliminar todas las bases de datos --------------------------*/

void deleteAllDatabases() {
  File root = SD.open("/");
  if (!root) {
    Serial.println("Error al abrir el directorio raíz de la tarjeta SD");
    return;
  }

  if (!root.isDirectory()) {
    Serial.println("La raíz de la tarjeta SD no es un directorio");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    if (fileName.endsWith(".db")) { // Solo eliminar archivos con extensión .db
      Serial.print("Eliminando base de datos: ");
      Serial.println(fileName);

      // Asegurarse de que el archivo se cierre antes de eliminarlo
      file.close();

      // Intentar eliminar el archivo
      if (SD.remove(fileName)) {
        Serial.println("Archivo eliminado exitosamente");
      } else {
        Serial.println("Error al eliminar el archivo");
      }
    }
    file = root.openNextFile();
  }
}

/*--------------------------------- Configuración Inicial en setup() ---------------------------------*/

void setup() {
  Serial.begin(115200);
  delay(1000);

  esp_task_wdt_init(30, true);
  esp_task_wdt_add(NULL);

  WiFi.mode(WIFI_STA);
  reconnectWiFi();
  runSetupServerNTP();

  ElegantOTA.setAuth("admin", "admin");
  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.on("/tempMachine", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readTempMachine().c_str());
  });

  server.on("/setTemp", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value", true)) {
      setTemperature = request->getParam("value", true)->value().toFloat();
      saveSetTemperature();
      request->send(200, "text/plain", "Set temperature updated");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  server.on("/setTempAlarm", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readsetTempAlarm().c_str());
  });

  server.on("/database", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SD, "/temperature.db", "application/octet-stream");        
  });

  server.on("/deleteAllData", HTTP_GET, [](AsyncWebServerRequest *request) {
    deleteAllData();
    request->send(200, "text/plain", "All data deleted");
  });

  server.on("/deleteAllDatabases", HTTP_GET, [](AsyncWebServerRequest *request) {
    deleteAllDatabases();
    request->send(200, "text/plain", "Todas las bases de datos han sido eliminadas.");
  });

  server.serveStatic("/", SD, "/");
  server.serveStatic("/", LittleFS, "/").setCacheControl("max-age=600");
  server.begin();
  Serial.println("Servidor HTTP iniciado");

  initializeLittleFS();
  displayLittleFSSpace();
  loadSetTemperature();

  pinMode(ledWifiOutput, OUTPUT);
  digitalWrite(ledWifiOutput, LOW);

  pinMode(relay1_Alarm, OUTPUT);
  digitalWrite(relay1_Alarm, HIGH);

  SPI.begin(12, 13, 11, 10);
  if (!SD.begin(10)) {
    Serial.println("Error al inicializar la tarjeta SD");
    return;
  }
  sqlite3_initialize();

  // Modificación: Intentar abrir la última base de datos o crear una nueva
  createOrOpenDatabase();
}

/*--------------------------------- Bucle Principal en loop() ---------------------------------*/

void loop() {
  reconnectWiFi();
  updateLedStatusWifi();
  ElegantOTA.loop();

  tempMachine = round((25.0 + ((float)rand() / RAND_MAX) * 5.0) * 100.0) / 100.0;

  if (tempMachine >= setTemperature) {
    digitalWrite(relay1_Alarm, LOW); // Activar alarma
  } else {
    digitalWrite(relay1_Alarm, HIGH); // Desactivar alarma
  }

  unsigned long currentTime2 = millis();
  if (currentTime2 - previousTime2 >= interval2) {
    previousTime2 = currentTime2;

    if (WiFi.status() == WL_CONNECTED) {
      insertData(tempMachine);
      Serial.println(rtc_esp32.getTime("%A, %B %d %Y %H:%M:%S"));  // Imprime la hora actualizada
    }
  }

  esp_task_wdt_reset();
}
