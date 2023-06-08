#include <WiFi.h>
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>       
#include <Arduino.h>
#include "freertos/queue.h"
#include <Bounce2.h>    
#include <ESP32Time.h>  
#include <HTTPClient.h>  
#include <ArduinoJson.h> 
#include <FS.h>
#include <LittleFS.h>
#include <esp_system.h>  

///////!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"""""""""""""""""""22221111111111111111111"
/*--------VERIFICAR DATOS----------*/
/*





//falta el codigo en el .ini de littlefs

//falta volver a la normalidad para no ver mensajes de depuracion

//ste codigo envia el conteo de el pin10 cuando para el star(in1), esta bien para matzuyas pero borrr para el resto

       SSID
       PASSWORD
       IP
       GATEWAY
       SUBNET
       DNS 1
       DNS 2

       URL DE LA API

       IP DEL SERVIDOR NTP

       CODIGO DE MAQUINA
      
       INTERVALO DE TIEMPO DE ENVIO DESDE LITTLEFS

       HOSTNAME DEL EQUIPO   EJEMPLO : DeviceTejRecR609





*/


/*--------ACTIVAR/DESACTIVAR DEPURACION----------*/
#define DEBUG_CMD "starship"  // Codigo de acceso
bool debugEnabled = false;  // Habilita/Deshabilita depuracion   //dejarlo en false
#define DEBUG_PRINT(x)  if(debugEnabled) { Serial.print(x); }
#define DEBUG_PRINTLN(x)  if(debugEnabled) { Serial.println(x); }


/*--------IMPUT DIGITAL----------*/ //Tarj. Matzuya M.  out:  33, start :  23,  conteo:   35
#define IN1 1                                           //PRODUCCION   
#define IN2 2                                           //PARO MANUAL
#define IN3 4                                           //HILO LATERAL
#define IN4 5                                           //HILO SUPERIOR
#define IN5 6                                           //LYCRA
#define IN6 7                                           //CAIDA DE TELA
#define IN7 8                                           //ANTIRREMOTADA
#define IN8 9                                           //FIN DE PIEZA O RPM
#define IN9 10                                          //PUERTA
#define IN10 11                                         //CONTEO

/*--------OUTPUT RELAY----------*/                                    
#define RL1 12                                          
#define RL2 13                                           
#define RL3 14 

/*--------LEDS INDICADORES----------*/
#define LEDWIFI 21 
#define LEDRTC 38 

/*--------I2C PARA DS3231----------*/
#define SDA_PIN 17
#define SCL_PIN 18

#define MAX_ATTEMPTS 10 //NUMERO DE INTENTOS PARA INICIAR DS3231 POR I2C


/*--------SERVIDOR NTP----------*/
const char* ntpServer = "131.107.32.217";         //Servidor NTP TDV 131.107.32.217 
const long gmtOffset_sec = -5*3600;                //Desplazamiento GMT
const int daylightOffset_sec = 0;                  //Compensacion de luz diurna
ESP32Time rtc_esp32;


RTC_DS3231 rtc_ds3231;

DateTime now; //variable global para definir now que luego se usara para alamcenar el tiempo


// Función que devuelve la fecha y la hora actual del RTC en formato (YYYYMMDDTHHMMSS)
String getDateTimeCodeDS3231() {

now = rtc_ds3231.now(); //Actualiza el tiempo en now desde DS3231 para usarlo en getDateTimeCodeDS3231() y getDateTimeDS3231()

  char buf[64];
  sprintf(buf, "%04d%02d%02d%02d%02d%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  return String(buf);
}

// Función que devuelve la fecha y la hora actual del RTC en formato ISO 8601 (YYYY-MM-DDTHH:MM:SS)
String getDateTimeDS3231() {
now = rtc_ds3231.now();
//DateTime now = rtc_ds3231.now();
  // Crear una cadena para almacenar la fecha y la hora formateada
  char buf[64];

  // Formatear la fecha y la hora y guardarla en la cadena creada
  sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

  // Devolver la cadena con la fecha y la hora formateada
  return String(buf);
}



/*--------ALARMA DE NO CONFIGURACION DE TIEMPO DS3231, INTERVALO PARDADEO LED----------*/ 
unsigned long previousMillis_led_rtc = 0; 
const long interval_led_rtc = 250; // Intervalo de parpadeo en milisegundos para alarma


/*--------TIEMPO DE ESPERA PARA EJECUTAR CONFIGURACION SERVIDOR NTP----------*/
unsigned long startTimeoutForNTP = 0; 
bool timeoutOver = false; // Para saber si ya pasó el tiempo de espera


/*--------FUNCIONES QUE SE EJECUTAN UNA SOLA VEZ----------*/
bool funcionYaEjecutada = false;
bool funcionYaEjecutada2 = false;

bool ledStatus = false;


/*--------FUNCION PARA ALARMA DE NO CONFIGURACION DE TIMEPO DS3231----------*/
void updateLedStatusRTC() {
  unsigned long currentMillis_led_rtc = millis(); // Obtiene el tiempo actual
  if (currentMillis_led_rtc - previousMillis_led_rtc >= interval_led_rtc) {
    previousMillis_led_rtc = currentMillis_led_rtc;
   if (digitalRead(LEDRTC) == LOW) {
       digitalWrite(LEDRTC, HIGH);
   } else {
    digitalWrite(LEDRTC, LOW);
    }
  }
}

/*--------FUNCION PARA EJECUTAR CONFIGURACION DE SERVIDOR NTP----------*/
void runSetupServerNTP() {
  if (!funcionYaEjecutada) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
	Serial.print("\n\n\nActualizando tiempo en ESP32 desde servidor NTP");
  startTimeoutForNTP = millis(); // Guarda el tiempo de inicio del programa
    funcionYaEjecutada = true;
  }
}

/*--------FUNCION PARA CONFIGURAR TIEMPO EN EL DS3231----------*/
void updateTimeDevice() {
  if (!funcionYaEjecutada2) {
    if (rtc_esp32.getYear() > 2022) { 
	 rtc_ds3231.adjust(DateTime(rtc_esp32.getYear(), rtc_esp32.getMonth()+1, rtc_esp32.getDay(), rtc_esp32.getHour(true), rtc_esp32.getMinute(), rtc_esp32.getSecond()));
     Serial.print("\n\nConfigurando DS3231 :  "); 
     Serial.print(rtc_esp32.getTime("%A, %B %d %Y %H:%M:%S"));
	 if(rtc_ds3231.now().year() > 2022){
    Serial.print("\n\nDS3231 Configurado desde ESP32"); 
	funcionYaEjecutada2 = true;
	timeoutOver = true; // Ya no necesitamos seguir comprobando
  digitalWrite(LEDRTC, LOW);
    if (ledStatus) {digitalWrite(LEDRTC, HIGH);} 
	return;	
	    }else{
        Serial.println("\n\nFalla al configurar el DS3231 desde ESP32");
      }
    }
  if(rtc_ds3231.now().year() > 2022){
    Serial.println("\n\nDS3231 Ya se encuentra Configurado"); 
	funcionYaEjecutada2 = true;	
	timeoutOver = true; // Ya no necesitamos seguir comprobando
  return;	
	}

updateLedStatusRTC(); //alarma si no se logra actualizar datos de servidor ntp
Serial.println("\n\nNo se pudo actualizar Tiempo de ESP32 y DS3231"); 

  }
}



/*--------WIFI----------*/
const char *wifi_ssid = "WF_TDV_PLC";                    //WF_TDV_PLC 
const char *wifi_password = "M4qPLC$*-+/";                //M4qPLC$*-+/
IPAddress local_IP(172, 16, 2, 210);                 //IPAddress local_IP(172, 16, 2, 201);
IPAddress gateway(172, 16, 2, 1);                   //IPAddress gateway(172, 16, 2, 1);
IPAddress subnet(255, 255, 254, 0);                    //IPAddress subnet(255, 255, 254, 0);
IPAddress primary_dns(131, 107, 32, 217);              //131, 107, 32, 217
IPAddress secondary_dns(131, 107, 32, 218);            //131, 107, 32, 218

unsigned long ultimoIntento = 0;   
int countReconectWifi = 0;
const int intervaloReconexion = 1000;

void reconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {   

    unsigned long tiempoActual = millis();
    if (tiempoActual - ultimoIntento >= intervaloReconexion) {
      ultimoIntento = tiempoActual;
      
      if (countReconectWifi == 0) {
        DEBUG_PRINT("\n\n\nReconectando a la red Wi-Fi  ");
        WiFi.disconnect();


         // Usar IP estática y DNS                                        
        if (!WiFi.config(local_IP, gateway, subnet, primary_dns, secondary_dns))   {  
        DEBUG_PRINT("\n\nError al configurar la IP estática y DNS");    
        }

        WiFi.begin(wifi_ssid, wifi_password);
      }
      
      DEBUG_PRINT(".");
      countReconectWifi++;

      if (countReconectWifi > 10) {
        
        DEBUG_PRINT("\n\nConexion WIFI Fallida Reintentando ");
        countReconectWifi = 0;
      }
    }
  } else {

    if (countReconectWifi > 0) {
    
      DEBUG_PRINT("\n\n           Conexion WIFI Exitosa   ->    IP Tarjeta :  ");
      DEBUG_PRINT(WiFi.localIP());
     
    }
    countReconectWifi = 0;

    runSetupServerNTP();

  }

}






/*--------VARIABLES GLOVALES----------*/

String url_api = "https://developer.textildelvalle.pe:444/TEJRectilineoParoBE/api/RectilineoEvento/Write"; //http://192.168.54.161/BackEnd2/api/Rectilineo/Write


const char* codDevice = "DeviceTejRecJ2";
const char* codMaquina = "J2";    
const char* codParoIN1 = "MP001";      //START/STOP
const char* codParoIN2 = "MP004";      //HILO SUPERIOR
const char* codParoIN3 = "MP003";      //HILO LATERAL
const char* codParoIN4 = "MP007";      //ANTIRREMOTADA
const char* codParoIN5 = "MP009";      //PUERTA
const char* codParoIN6 = "MP006";      //CAIDA DE TELA 
const char* codParoIN7 = "MP005";      //LYCRA
const char* codParoIN8 = "MP008";      //PUERTA LATERAL
const char* codParoIN9 = "RPM";        //RPM 
const char* codParoIN10 = "CONTADOR";  //CONTADOR DE UNIDADES

/*--------CONTADOR DE UNIDADES----------*/
volatile unsigned long count = 0;    

/*--------DEFINE EL INTERVALO DE TIEMPO DE ENVIO SI HAY DATOS GUARDADOS----------*/
int timeupdateSendHTTP = 15;
const unsigned long interval1 = timeupdateSendHTTP * 60 * 1000; 
unsigned long previousTime1 = 0;

/*--------DEFINE EL INTERVALO DE TIEMPO PARA VERIFICAR SI HAY ACTUALIZACIONES DE CODIGO DESDE SISTEMAS----------*/
int timeupdatewifi = 100; //tiempo en minutos para actualizar credenciales wifi
const unsigned long interval2 = timeupdatewifi * 60 * 1000; //Envio de datos para actualizar Wifi
unsigned long previousTime2 = 0;





DynamicJsonDocument json(2048);
DynamicJsonDocument apiResponse(256);
HTTPClient http;
QueueHandle_t queue;  //Cola

Bounce debouncer1 = Bounce();
Bounce debouncer2 = Bounce();
Bounce debouncer3 = Bounce();
Bounce debouncer4 = Bounce();
Bounce debouncer5 = Bounce();
Bounce debouncer6 = Bounce();
Bounce debouncer7 = Bounce();
Bounce debouncer8 = Bounce();
Bounce debouncer9 = Bounce();


struct InputInfo
 {
  uint8_t pin;
  int state;
 };


/*--------TAREA EN CORE 0 (LOOP-2)----------*/
void Task1code( void * pvParameters ) {
  for (;;) {
   
  if (debouncer1.update()) {
    int value = debouncer1.read();
    InputInfo info = {IN1, value};
    xQueueSend(queue, &info, 0); //xQueueSend(queue, &info, portMAX_DELAY);
  }
  if (debouncer2.update()) {
    int value = debouncer2.read();
    InputInfo info = {IN2, value};
    xQueueSend(queue, &info, 0); //xQueueSend(queue, &info, portMAX_DELAY);
  }
  if (debouncer3.update()) {
    int value = debouncer3.read();
    InputInfo info = {IN3, value};
    xQueueSend(queue, &info, 0); //xQueueSend(queue, &info, portMAX_DELAY);
  }
  if (debouncer4.update()) {
    int value = debouncer4.read();
    InputInfo info = {IN4, value};
    xQueueSend(queue, &info, 0); //xQueueSend(queue, &info, portMAX_DELAY);
  }
  if (debouncer5.update()) {
    int value = debouncer5.read();
    InputInfo info = {IN5, value};
    xQueueSend(queue, &info, 0); //xQueueSend(queue, &info, portMAX_DELAY);
  }
  if (debouncer6.update()) {
    int value = debouncer6.read();
    InputInfo info = {IN6, value};
    xQueueSend(queue, &info, 0); //xQueueSend(queue, &info, portMAX_DELAY);
  }
  if (debouncer7.update()) {
    int value = debouncer7.read();
    InputInfo info = {IN7, value};
    xQueueSend(queue, &info, 0); //xQueueSend(queue, &info, portMAX_DELAY);
  }
  if (debouncer8.update()) {
    int value = debouncer8.read();
    InputInfo info = {IN8, value};
    xQueueSend(queue, &info, 0); //xQueueSend(queue, &info, portMAX_DELAY);
  }
  if (debouncer9.update()) {
    int value = debouncer9.read();
    InputInfo info = {IN9, value};
    xQueueSend(queue, &info, 0); //xQueueSend(queue, &info, portMAX_DELAY);
  }




    vTaskDelay(1); // Agrega un pequeño retardo para liberar el procesador
  
  }
}


/*--------INDICADORES BALIZA RELE----------*/
void updateReleStatusInput() {

  if (digitalRead(IN1) == HIGH)
   { digitalWrite(RL1, HIGH);
   } else {
    digitalWrite(RL1, LOW);
   }

  if (digitalRead(IN3) == LOW || digitalRead(IN4) == LOW || digitalRead(IN5) == LOW || digitalRead(IN6) == LOW || digitalRead(IN7) == LOW || digitalRead(IN9) == LOW)
   { digitalWrite(RL2, HIGH);
   } else {
    digitalWrite(RL2, LOW);
   }

  if (digitalRead(IN2) == HIGH)
   { digitalWrite(RL3, HIGH);
   } else {
    digitalWrite(RL3, LOW);
   }

}


/*--------LED SE ENCIENDE SI EL DEVICE ESTA CONECTADO A WIFI----------*/
void updateLedStatusWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LEDWIFI, LOW);  // Apagar LED
  } else {
    digitalWrite(LEDWIFI, HIGH); // Encender LED
  }
}





/*--------CONTADOR DE UNIDADES----------*/
void IRAM_ATTR countInterrupt() {
  count++;   // Incrementar el contador cada vez que se detecte un cambio en el pin de entrada
}


/*--------ALMACENAMIENTO FLASH----------*/
void displayLittleFSSpace() {

  //prueba para ver espacio : inicio
  Serial.print("     Almacenamiento flash total (bytes): ");
  Serial.println(ESP.getFlashChipSize());
  Serial.print("     Almacenamiento flash usado (bytes): ");
  Serial.println(ESP.getSketchSize());

  Serial.print("     Espacio total en LittleFS: ");
  Serial.print(LittleFS.totalBytes() / (1024.0 * 1024.0));
  Serial.println(" MB");

  Serial.print("     Espacio usado en LittleFS: ");
  Serial.print(LittleFS.usedBytes() / (1024.0 * 1024.0));
  Serial.println(" MB");

  //prueba : fin

  
  
  // Calcular el porcentaje de espacio utilizado
  float percentageUsed = (float)LittleFS.usedBytes() / (float)LittleFS.totalBytes() * 100;

  // Mostrar el porcentaje de espacio utilizado con dos decimales
  Serial.print("\n     Espacio Usado de Memoria: ");
  Serial.printf("%.2f", percentageUsed);
  Serial.println(" %");

}

void initializeLittleFS() {

if (!LittleFS.begin()) {  //!  FORMATEAR MEMORIA QUINATR SIGNO LUEGO VOLVER A COLOCAR
  Serial.println("\n     Error al Inicializar LittleFS, Formateando...");
  if (LittleFS.format()) {
    Serial.println("\n     Formateo de LittleFS Completado");
  } else {
    Serial.println("\n     Error al Formatear LittleFS");
    return;
  }
  if (!LittleFS.begin()) {
    Serial.println("\n     Error al Inicializar LittleFS Después de Formatear");
    return;
  }
} else {
  Serial.println("\n     LittleFS Inicializado Correctamente"); // Agrega este mensaje de éxito
}
}

void ensureFileExists() {
  if (!LittleFS.exists("/data.txt")) {
    File file = LittleFS.open("/data.txt", "a");  //w
    if (!file) {
      Serial.println("\n     Error al Crear el Archivo data.txt");
    } else {
      Serial.println("\n     Archivo data.txt Creado Exitosamente");
      file.close();
    }
  } else {
    Serial.println("\n     Se Encontro el Archivo data.txt ");
  }
}

void appendDataToFile(const String &data) {
  File file = LittleFS.open("/data.txt", "a");
  if (!file) {
    DEBUG_PRINT("\nError al abrir el archivo para escritura");
  } else {
    file.println(data);
    file.close();
    DEBUG_PRINT("\nArchivo guardado correctamente en memoria");
  }
}

/*LEE CONTENIDO DE DATA.TEXT
void readAllDataFromFile() {
  File file = LittleFS.open("/data.txt", "r");
  if (!file) {
    DEBUG_PRINT("\n     Error al abrir el archivo para lectura");
  } else {
    DEBUG_PRINT("\n     Archivo abierto correctamente");
    while (file.available()) {
      Serial.write(file.read());
    }
    file.close();
  }
}
*/

void copyFileExceptSentLine(const String &sentLine) {
  File sourceFile = LittleFS.open("/data.txt", "r");
  File tempFile = LittleFS.open("/temp.txt", "w");

  if (!sourceFile || !tempFile) {
    DEBUG_PRINT("\n     Error al abrir archivos para actualizar datos no enviados");
    return;
  }

  String line;
  while (sourceFile.available()) {
    line = sourceFile.readStringUntil('\n');
    if (line != sentLine) {
      tempFile.println(line);
    }
  }

  sourceFile.close();
  tempFile.close();

}

void replaceOriginalFileWithTemp() {
  if (LittleFS.remove("/data.txt")) {
    if (LittleFS.rename("/temp.txt", "/data.txt")) {
      DEBUG_PRINT("\nArchivo de LittleFS actualizado correctamente");
    } else {
      DEBUG_PRINT("\nError al renombrar el archivo temporal");
    }
  } else {
    DEBUG_PRINT("\nError al eliminar el archivo original de LittleFS");
  }
}

void sendDataFromLittleFS() {

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  
  File file = LittleFS.open("/data.txt", "r"); 
  if (!file) {
    DEBUG_PRINT("\n\n\n\nError al intentar enviar desde LittleFS nose pudo abrir el archivo data.text para lectura");
   // file.close(); // Cierra el archivo
  } else {
    if (file.available()) { // Verifica si hay datos en el archivo
      String line; // Variable para almacenar cada línea leída
      while (file.available()) { // Mientras haya datos en el archivo
        line = file.readStringUntil('\n'); // Lee una línea hasta el carácter de nueva línea
        DEBUG_PRINT("\n\n\n\nAbriendo dato almacenado desde LittleFS");
        //DEBUG_PRINTLN(line);

        file.close(); // Cierra el archivo

        //HTTP POST
        http.begin(url_api);
        http.addHeader("Content-Type", "application/json"); 

        int responseCode = http.POST(line);

        if (responseCode < 0) {
          DEBUG_PRINT("\n\nError al intentar enviar Post Request desde LittleFS ");
          http.end();
          return;
        }

        if (responseCode != 200) {
          DEBUG_PRINT("\n\nError en response al intentar enviar desde LittleFS :  ");
          DEBUG_PRINTLN(responseCode);
          http.end();
          return;
        }

        if (responseCode == 200) {
          String responseBody = http.getString();

          DEBUG_PRINT("\n\nDatos enviados desde LittleFS :  ");
          DEBUG_PRINTLN(line);

          http.end();

          // Copia el contenido del archivo original a un archivo temporal
          // excepto la línea enviada
          copyFileExceptSentLine(line);
          // Reemplaza el archivo original con el archivo temporal
          replaceOriginalFileWithTemp();
        }
    
        // Abre el archivo nuevamente para la siguiente lectura
        file = LittleFS.open("/data.txt", "r");
      }
      file.close(); 
    } else {
      file.close(); 
      DEBUG_PRINT("\nNo hay datos para enviar desde LittleFS");
    }
  }
}



/*--------PRUEBA - RELE CONTADOR ACTIVACIONES CADA X TIEMPO ----------*/
int count_prueba = 0; // Declarar una variable de contador




void setup()
{

 Serial.begin(115200);
 
 configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

 Wire.begin(SDA_PIN, SCL_PIN);//iniciando I2C

   int attempts = 0;
  while (!rtc_ds3231.begin() && attempts < MAX_ATTEMPTS) {
    attempts++;
    Serial.println("\n\n Intentando iniciar el modulo DS3231");
    delay(1000);
   }
  
  if (attempts == MAX_ATTEMPTS) {
    Serial.println("\n\n No se pudo encontrar el modulo DS3231 valido");
    updateLedStatusRTC(); //alarma si no se logra actualizar datos de servidor ntp
     //deberia activarse la alarma updateLedStatusRTC() por que no detectaria el año valido
  }



 WiFi.setHostname(codDevice);


 

 queue = xQueueCreate(100, sizeof(InputInfo));  //Cola

 
 Serial.print("\n\n\n     Host: ");
 Serial.println(WiFi.getHostname());

 initializeLittleFS();
 ensureFileExists();
 displayLittleFSSpace();


 pinMode(IN1, INPUT);
 pinMode(IN2, INPUT);
 pinMode(IN3, INPUT);
 pinMode(IN4, INPUT);
 pinMode(IN5, INPUT);
 pinMode(IN6, INPUT);
 pinMode(IN7, INPUT);
 pinMode(IN8, INPUT);
 pinMode(IN9, INPUT);
 


 pinMode(RL1, OUTPUT);
 digitalWrite(RL1, LOW);
 pinMode(RL2, OUTPUT);
 digitalWrite(RL2, LOW);
 pinMode(RL3, OUTPUT);
 digitalWrite(RL3, LOW);


 pinMode(LEDWIFI, OUTPUT);
 digitalWrite(LEDWIFI, LOW);
 pinMode(LEDRTC, OUTPUT);
 digitalWrite(LEDRTC, LOW);


 attachInterrupt(digitalPinToInterrupt(IN10), countInterrupt, FALLING);  // Establece interrupción en el pin de entrada 10
 


/*--------DETECCION DE PERDIDA DE ENERGIA DEL DS3231----------*/
 if (rtc_ds3231.lostPower()) {  //detecta perdida de energia en el modulo DS3231
    digitalWrite(LEDRTC, HIGH);
    ledStatus = true; // Cambia el estado del LED
  }




  
 debouncer1.attach(IN1);
 debouncer1.interval(600); // intervalo en ms

 debouncer2.attach(IN2);
 debouncer2.interval(900); // intervalo en ms

 debouncer3.attach(IN3);
 debouncer3.interval(900); // intervalo en ms
 
 debouncer4.attach(IN4);
 debouncer4.interval(600); // intervalo en ms
 
 debouncer5.attach(IN5);
 debouncer5.interval(600); // intervalo en ms
 
 debouncer6.attach(IN6);
 debouncer6.interval(1000); // intervalo en ms   //aumentar si es preciso ya que da muchos activaciones caida de tela
 
 debouncer7.attach(IN7);
 debouncer7.interval(600); // intervalo en ms
 
 debouncer8.attach(IN8);
 debouncer8.interval(600); // intervalo en ms
 
 debouncer9.attach(IN9);
 debouncer9.interval(600); // intervalo en ms


 // Comprueba y registra el estado inicial de las entradas digitales
 debouncer1.update();
 if (debouncer1.read() == HIGH) 
   {
   InputInfo initialInfo1 = {IN1, HIGH};
   xQueueSend(queue, &initialInfo1, 0); //xQueueSend(queue, &initialInfo1, portMAX_DELAY);
   }
 
 debouncer2.update();
 if (debouncer2.read() == HIGH)
   {
   InputInfo initialInfo2 = {IN2, HIGH};
   xQueueSend(queue, &initialInfo2, 0); //xQueueSend(queue, &initialInfo2, portMAX_DELAY);
   }

   debouncer3.update();
 if (debouncer3.read() == LOW) 
   {
   InputInfo initialInfo3 = {IN3, LOW};
   xQueueSend(queue, &initialInfo3, 0); //xQueueSend(queue, &initialInfo2, portMAX_DELAY);
   }

   debouncer4.update();
 if (debouncer4.read() == LOW) 
   {
   InputInfo initialInfo4 = {IN4, LOW};
   xQueueSend(queue, &initialInfo4, 0); //xQueueSend(queue, &initialInfo2, portMAX_DELAY);
   }

   debouncer5.update();
 if (debouncer5.read() == LOW) 
   {
   InputInfo initialInfo5 = {IN5, LOW};
   xQueueSend(queue, &initialInfo5, 0); //xQueueSend(queue, &initialInfo2, portMAX_DELAY);
   }

   debouncer6.update();
 if (debouncer6.read() == LOW) 
   {
   InputInfo initialInfo6 = {IN6, LOW};
   xQueueSend(queue, &initialInfo6, 0); //xQueueSend(queue, &initialInfo2, portMAX_DELAY);
   }

   debouncer7.update();
 if (debouncer7.read() == LOW) 
   {
   InputInfo initialInfo7 = {IN7, LOW};
   xQueueSend(queue, &initialInfo7, 0); //xQueueSend(queue, &initialInfo2, portMAX_DELAY);
   }

   debouncer8.update();
 if (debouncer8.read() == LOW) 
   {
   InputInfo initialInfo8 = {IN8, LOW};
   xQueueSend(queue, &initialInfo8, 0); //xQueueSend(queue, &initialInfo2, portMAX_DELAY);
   }

   debouncer9.update();
 if (debouncer9.read() == LOW) 
   {
   InputInfo initialInfo9 = {IN9, LOW};
   xQueueSend(queue, &initialInfo9, 0); //xQueueSend(queue, &initialInfo2, portMAX_DELAY);
   }



 xTaskCreatePinnedToCore(
                   Task1code,
                   "Task1",
                   10000,
                   NULL,
                   1,
                   NULL,
                   0);





Serial.print("\n     Servidor NTP : ");
Serial.print(ntpServer);

Serial.print("\n\n\n\n         Codigo de Depuracion : ");


}



void loop()
{
  reconnectWiFi();

  updateLedStatusWifi();
  updateReleStatusInput();




  

  if (!timeoutOver) // Si aún no ha pasado el tiempo de espera
  {
  if (WiFi.status() == WL_CONNECTED) 
  { 
    if (millis() - startTimeoutForNTP >= 15000)  
    {
      updateTimeDevice();
    }
  }
  }
  






  /*--------ACTIVAR/DESACTIVAR DEPURACION----------*/

  if (Serial.available()) {
    
    String cod = Serial.readStringUntil('\n'); // Lee la línea desde la interfaz serie

    if (cod == DEBUG_CMD) {
      debugEnabled = !debugEnabled;  // Cambia el estado de debugEnabled
      Serial.println(debugEnabled ? "         DEPURACION HABILITADA" : "\n\n\n         DEPURACION DESHABILITADA");
    }
  }


/*-------------------------ENVIAR ESTADOS A BD---------------------------*/      

    InputInfo info;

    if (uxQueueMessagesWaiting(queue) > 0) 
{

    if(xQueueReceive(queue, &info, 0))
 {

 /*-------------------------DIGITAL IMPUT 1---------------------------*/
  if(info.pin == IN1)

  {
     static String  codStaticIN1 = "";

        if(info.state == HIGH) 

        {  codStaticIN1 = getDateTimeCodeDS3231();  //rtc_esp32.getTime("%Y%m%d%H%M%S");  // Actualizar el código cuando PIN1 esté en alto
 
  //JSON  A ENVIAR POR POST
    json.clear();                                                        
    json["CodMaquina"] = codMaquina,                  
    json["CodParo"] = codParoIN1,                    
    json["CodUnicoParo"] = codStaticIN1,          
    json["FechaHoraIniParo"] = getDateTimeDS3231(),   //rtc_esp32.getTime("%FT%T"),         
    json["FechaHoraFinParo"] = "",          
    json["Rpm"] = 0,                                      

   //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int responseCode = http.POST(json.as<String>());

   if (responseCode < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (responseCode != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(responseCode);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (responseCode == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(apiResponse, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = apiResponse["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();
    

    }
      count = 0;
  } 
        
 else
        
 {

  //JSON  A ENVIAR POR POST
    json.clear();                                                              
    json["CodMaquina"] = codMaquina,                 
    json["CodParo"] = codParoIN1,                   
    json["CodUnicoParo"] = codStaticIN1,          
    json["FechaHoraIniParo"] = "",     
    json["FechaHoraFinParo"] = getDateTimeDS3231(),   //rtc_esp32.getTime("%FT%T"),            
    json["Rpm"] = count,                                      

    //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int responseCode = http.POST(json.as<String>());

   if (responseCode < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (responseCode != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(responseCode);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (responseCode == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(apiResponse, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = apiResponse["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();

      }

      codStaticIN1 = "";
      count = 0;
   
    }
     
  }

  
 /*-------------------------DIGITAL IMPUT 2---------------------------*/
  if(info.pin == IN2)

  {
     static String  codStaticIN2 = "";

        if(info.state == HIGH) 

        {  codStaticIN2 = getDateTimeCodeDS3231();  //rtc_esp32.getTime("%Y%m%d%H%M%S");  // Actualizar el código cuando PIN1 esté en alto
 
  //JSON  A ENVIAR POR POST
    json.clear();                                                        
    json["CodMaquina"] = codMaquina,                  
    json["CodParo"] = codParoIN2,                    
    json["CodUnicoParo"] = codStaticIN2,          
    json["FechaHoraIniParo"] = getDateTimeDS3231(),   //rtc_esp32.getTime("%FT%T"),         
    json["FechaHoraFinParo"] = "",          
    json["Rpm"] = 0,                                      

   //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int responseCode = http.POST(json.as<String>());

   if (responseCode < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (responseCode != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(responseCode);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (responseCode == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(apiResponse, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = apiResponse["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();
    

    }
     
  } 
        
 else
        
 {

  //JSON  A ENVIAR POR POST
    json.clear();                                                              
    json["CodMaquina"] = codMaquina,                 
    json["CodParo"] = codParoIN2,                   
    json["CodUnicoParo"] = codStaticIN2,          
    json["FechaHoraIniParo"] = "",     
    json["FechaHoraFinParo"] = getDateTimeDS3231(),   //rtc_esp32.getTime("%FT%T"),            
    json["Rpm"] = 0,                                      

    //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int responseCode = http.POST(json.as<String>());

   if (responseCode < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (responseCode != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(responseCode);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (responseCode == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(apiResponse, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = apiResponse["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();

      }

      codStaticIN2 = "";
   
    }
     
  }

  

 /*-------------------------DIGITAL IMPUT 3---------------------------*/
/* NO ACTIVO EN   J2
  if(info.pin == IN3)

  {
     static String  codStaticIN3 = "";

        if(info.state == LOW) 

        {  codStaticIN3 = getDateTimeCodeDS3231();  //rtc_esp32.getTime("%Y%m%d%H%M%S");  // Actualizar el código cuando PIN1 esté en alto
 
  //JSON  A ENVIAR POR POST
    json.clear();                                                        
    json["CodMaquina"] = codMaquina,                  
    json["CodParo"] = codParoIN3,                    
    json["CodUnicoParo"] = codStaticIN3,          
    json["FechaHoraIniParo"] = getDateTimeDS3231(),   //rtc_esp32.getTime("%FT%T"),         
    json["FechaHoraFinParo"] = "",          
    json["Rpm"] = 0,                                      

   //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int responseCode = http.POST(json.as<String>());

   if (responseCode < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (responseCode != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(responseCode);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (responseCode == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(apiResponse, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = apiResponse["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();
    

    }
     
  } 
        
 else
        
 {

  //JSON  A ENVIAR POR POST
    json.clear();                                                              
    json["CodMaquina"] = codMaquina,                 
    json["CodParo"] = codParoIN3,                   
    json["CodUnicoParo"] = codStaticIN3,          
    json["FechaHoraIniParo"] = "",     
    json["FechaHoraFinParo"] = getDateTimeDS3231(),   //rtc_esp32.getTime("%FT%T"),            
    json["Rpm"] = 0,                                      

    //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int responseCode = http.POST(json.as<String>());

   if (responseCode < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (responseCode != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(responseCode);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (responseCode == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(apiResponse, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = apiResponse["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();

      }

      codStaticIN3 = "";
   
    }
     
  }
*/
  

 /*-------------------------DIGITAL IMPUT 4---------------------------*/
  if(info.pin == IN4)

  {
     static String  codStaticIN4 = "";

        if(info.state == LOW) 

        {  codStaticIN4 = getDateTimeCodeDS3231();  //rtc_esp32.getTime("%Y%m%d%H%M%S");  // Actualizar el código cuando PIN1 esté en alto
 
  //JSON  A ENVIAR POR POST
    json.clear();                                                        
    json["CodMaquina"] = codMaquina,                  
    json["CodParo"] = codParoIN4,                    
    json["CodUnicoParo"] = codStaticIN4,          
    json["FechaHoraIniParo"] = getDateTimeDS3231(),   //rtc_esp32.getTime("%FT%T"),         
    json["FechaHoraFinParo"] = "",          
    json["Rpm"] = 0,                                      

   //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int responseCode = http.POST(json.as<String>());

   if (responseCode < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (responseCode != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(responseCode);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (responseCode == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(apiResponse, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = apiResponse["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();
    

    }
     
  } 
        
 else
        
 {

  //JSON  A ENVIAR POR POST
    json.clear();                                                              
    json["CodMaquina"] = codMaquina,                 
    json["CodParo"] = codParoIN4,                   
    json["CodUnicoParo"] = codStaticIN4,          
    json["FechaHoraIniParo"] = "",     
    json["FechaHoraFinParo"] = getDateTimeDS3231(),   //rtc_esp32.getTime("%FT%T"),            
    json["Rpm"] = 0,                                      

    //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int responseCode = http.POST(json.as<String>());

   if (responseCode < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (responseCode != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(responseCode);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (responseCode == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(apiResponse, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = apiResponse["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();

      }

      codStaticIN4 = "";
   
    }
     
  }

  

 /*-------------------------DIGITAL IMPUT 5---------------------------*/
 /* NO ACTIVO EN   J2
  if(info.pin == IN5)

  {
     static String  codStaticIN5 = "";

        if(info.state == LOW) 

        {  codStaticIN5 = getDateTimeCodeDS3231();  //rtc_esp32.getTime("%Y%m%d%H%M%S");  // Actualizar el código cuando PIN1 esté en alto
 
  //JSON  A ENVIAR POR POST
    json.clear();                                                        
    json["CodMaquina"] = codMaquina,                  
    json["CodParo"] = codParoIN5,                    
    json["CodUnicoParo"] = codStaticIN5,          
    json["FechaHoraIniParo"] = getDateTimeDS3231(),   //rtc_esp32.getTime("%FT%T"),         
    json["FechaHoraFinParo"] = "",          
    json["Rpm"] = 0,                                      

   //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int responseCode = http.POST(json.as<String>());

   if (responseCode < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (responseCode != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(responseCode);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (responseCode == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(apiResponse, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = apiResponse["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();
    

    }
     
  } 
        
 else
        
 {

  //JSON  A ENVIAR POR POST
    json.clear();                                                              
    json["CodMaquina"] = codMaquina,                 
    json["CodParo"] = codParoIN5,                   
    json["CodUnicoParo"] = codStaticIN5,          
    json["FechaHoraIniParo"] = "",     
    json["FechaHoraFinParo"] = getDateTimeDS3231(),   //rtc_esp32.getTime("%FT%T"),            
    json["Rpm"] = 0,                                      

    //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int responseCode = http.POST(json.as<String>());

   if (responseCode < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (responseCode != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(responseCode);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (responseCode == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(apiResponse, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = apiResponse["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();

      }

      codStaticIN5 = "";
   
    }
     
  }
*/
  

 /*-------------------------DIGITAL IMPUT 6---------------------------*/
  if(info.pin == IN6)

  {
     static String  codStaticIN6 = "";

        if(info.state == LOW) 

        {  codStaticIN6 = getDateTimeCodeDS3231();  //rtc_esp32.getTime("%Y%m%d%H%M%S");  // Actualizar el código cuando PIN1 esté en alto
 
  //JSON  A ENVIAR POR POST
    json.clear();                                                        
    json["CodMaquina"] = codMaquina,                  
    json["CodParo"] = codParoIN6,                    
    json["CodUnicoParo"] = codStaticIN6,          
    json["FechaHoraIniParo"] = getDateTimeDS3231(),   //rtc_esp32.getTime("%FT%T"),         
    json["FechaHoraFinParo"] = "",          
    json["Rpm"] = 0,                                      

   //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int responseCode = http.POST(json.as<String>());

   if (responseCode < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (responseCode != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(responseCode);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (responseCode == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(apiResponse, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = apiResponse["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();
    

    }
     
  } 
        
 else
        
 {

  //JSON  A ENVIAR POR POST
    json.clear();                                                              
    json["CodMaquina"] = codMaquina,                 
    json["CodParo"] = codParoIN6,                   
    json["CodUnicoParo"] = codStaticIN6,          
    json["FechaHoraIniParo"] = "",     
    json["FechaHoraFinParo"] = getDateTimeDS3231(),   //rtc_esp32.getTime("%FT%T"),            
    json["Rpm"] = 0,                                      

    //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int responseCode = http.POST(json.as<String>());

   if (responseCode < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (responseCode != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(responseCode);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (responseCode == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(apiResponse, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = apiResponse["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();

      }

      codStaticIN6 = "";
   
    }
     
  }

  

 /*-------------------------DIGITAL IMPUT 7---------------------------*/
 /* NO ACTIVO EN   J2
  if(info.pin == IN7)

  {
     static String  codStaticIN7 = "";

        if(info.state == LOW) 

        {  codStaticIN7 = getDateTimeCodeDS3231();  //rtc_esp32.getTime("%Y%m%d%H%M%S");  // Actualizar el código cuando PIN1 esté en alto
 
  //JSON  A ENVIAR POR POST
    json.clear();                                                        
    json["CodMaquina"] = codMaquina,                  
    json["CodParo"] = codParoIN7,                    
    json["CodUnicoParo"] = codStaticIN7,          
    json["FechaHoraIniParo"] = getDateTimeDS3231(),   //rtc_esp32.getTime("%FT%T"),         
    json["FechaHoraFinParo"] = "",          
    json["Rpm"] = 0,                                      

   //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int responseCode = http.POST(json.as<String>());

   if (responseCode < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (responseCode != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(responseCode);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (responseCode == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(apiResponse, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = apiResponse["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();
    

    }
     
  } 
        
 else
        
 {

  //JSON  A ENVIAR POR POST
    json.clear();                                                              
    json["CodMaquina"] = codMaquina,                 
    json["CodParo"] = codParoIN7,                   
    json["CodUnicoParo"] = codStaticIN7,          
    json["FechaHoraIniParo"] = "",     
    json["FechaHoraFinParo"] = getDateTimeDS3231(),   //rtc_esp32.getTime("%FT%T"),            
    json["Rpm"] = 0,                                      

    //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int responseCode = http.POST(json.as<String>());

   if (responseCode < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (responseCode != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(responseCode);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (responseCode == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(apiResponse, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = apiResponse["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();

      }

      codStaticIN7 = "";
   
    }
     
  }
*/
  

 /*-------------------------DIGITAL IMPUT 8---------------------------*/
 /* NO ACTIVO EN   J2
  if(info.pin == IN8)

  {
     static String  codStaticIN8 = "";

        if(info.state == LOW) 

        {  codStaticIN8 = getDateTimeCodeDS3231();  //rtc_esp32.getTime("%Y%m%d%H%M%S");  // Actualizar el código cuando PIN1 esté en alto
 
  //JSON  A ENVIAR POR POST
    json.clear();                                                        
    json["CodMaquina"] = codMaquina,                  
    json["CodParo"] = codParoIN8,                    
    json["CodUnicoParo"] = codStaticIN8,          
    json["FechaHoraIniParo"] = getDateTimeDS3231(),   //rtc_esp32.getTime("%FT%T"),         
    json["FechaHoraFinParo"] = "",          
    json["Rpm"] = 0,                                      

   //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int responseCode = http.POST(json.as<String>());

   if (responseCode < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (responseCode != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(responseCode);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (responseCode == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(apiResponse, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = apiResponse["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();
    

    }
     
  } 
        
 else
        
 {

  //JSON  A ENVIAR POR POST
    json.clear();                                                              
    json["CodMaquina"] = codMaquina,                 
    json["CodParo"] = codParoIN8,                   
    json["CodUnicoParo"] = codStaticIN8,          
    json["FechaHoraIniParo"] = "",     
    json["FechaHoraFinParo"] = getDateTimeDS3231(),   //rtc_esp32.getTime("%FT%T"),            
    json["Rpm"] = 0,                                      

    //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int responseCode = http.POST(json.as<String>());

   if (responseCode < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (responseCode != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(responseCode);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (responseCode == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(apiResponse, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = apiResponse["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();

      }

      codStaticIN8 = "";
   
    }
     
  }
*/
  

 /*-------------------------DIGITAL IMPUT 9---------------------------*/
  if(info.pin == IN9)

  {
     static String  codStaticIN9 = "";

        if(info.state == LOW) 

        {  codStaticIN9 = getDateTimeCodeDS3231();  //rtc_esp32.getTime("%Y%m%d%H%M%S");  // Actualizar el código cuando PIN1 esté en alto
 
  //JSON  A ENVIAR POR POST
    json.clear();                                                        
    json["CodMaquina"] = codMaquina,                  
    json["CodParo"] = codParoIN9,                    
    json["CodUnicoParo"] = codStaticIN9,          
    json["FechaHoraIniParo"] = getDateTimeDS3231(),   //rtc_esp32.getTime("%FT%T"),         
    json["FechaHoraFinParo"] = "",          
    json["Rpm"] = 0,                                      

   //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int responseCode = http.POST(json.as<String>());

   if (responseCode < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (responseCode != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(responseCode);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (responseCode == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(apiResponse, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = apiResponse["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();
    

    }
     
  } 
        
 else
        
 {

  //JSON  A ENVIAR POR POST
    json.clear();                                                              
    json["CodMaquina"] = codMaquina,                 
    json["CodParo"] = codParoIN9,                   
    json["CodUnicoParo"] = codStaticIN9,          
    json["FechaHoraIniParo"] = "",     
    json["FechaHoraFinParo"] = getDateTimeDS3231(),   //rtc_esp32.getTime("%FT%T"),            
    json["Rpm"] = 0,                                      

    //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int responseCode = http.POST(json.as<String>());

   if (responseCode < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (responseCode != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(responseCode);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (responseCode == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(apiResponse, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = apiResponse["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();

      }

      codStaticIN9 = "";
   
    }
     
  }

  


























  }

}




/*-------------------------SE EJECUTA CADA X INTERVALO DE TIEMPO PARA ENVIAR DESDE LITTLEFS---------------------------*/  
 unsigned long currentTime1 = millis();
  if (currentTime1 - previousTime1 >= interval1) 
  {  previousTime1 = currentTime1;
   sendDataFromLittleFS(); 
  }






/*--------SERIALS PRINTS PARA MOSTARA EN CONSOLA----------*/
/*
  
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Máscara de subred: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Puerta de enlace: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("Dirección IP del servidor DNS 1: ");
  Serial.println(WiFi.dnsIP(0));
  Serial.print("Dirección IP del servidor DNS 2: ");
  Serial.println(WiFi.dnsIP(1));

  Serial.print("Intensidad de la señal Wi-Fi (RSSI): ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");

  Serial.print("\n\n\nEnviando JSON.......");
  Serial.println(json.as<String>());

  Serial.print("\n         Tiempo Servidor NTP  -> ");
  Serial.print(rtc_esp32.getTime("%Y%m%d%H%M%S"));

  size_t freeHeap = ESP.getFreeHeap();
  Serial.print("Memoria libre: ");
  Serial.print(freeHeap);
  Serial.println(" bytes");
  
  Serial.print("Velocidad del procesador: ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");

  Serial.print("Almacenamiento flash total (bytes): ");
  Serial.println(ESP.getFlashChipSize());
  Serial.print("Almacenamiento flash usado (bytes): ");
  Serial.println(ESP.getSketchSize());

  Serial.print("Espacio total en LittleFS: ");
  Serial.print(LittleFS.totalBytes() / (1024.0 * 1024.0));
  Serial.println(" MB");

  Serial.print("Espacio usado en LittleFS: ");
  Serial.print(LittleFS.usedBytes() / (1024.0 * 1024.0));
  Serial.println(" MB");

  Serial.print("Espacio libre en LittleFS: ");
  Serial.print((LittleFS.totalBytes() - LittleFS.usedBytes()) / (1024.0 * 1024.0));
  Serial.println(" MB");
  

*/




  /*--------PRUEBA - RELE CONTADOR ACTIVACIONES CADA X TIEMPO ----------*/
/*
 if (count_prueba < 300) {    //Numero de activaciones 
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 500) {   //Tiempo de duracion de cada activacion
    previousMillis = currentMillis;
    digitalWrite(RL3, !digitalRead(RL3));
     count_prueba++;}
 }

*/






/*--------PRUEBA - ENVIO DE CREDENCIALES WIFI POST ----------*/
/*
 unsigned long currentTime2 = millis();
  if (currentTime2 - previousTime2 >= interval2) 
  {  previousTime2 = currentTime2;

    if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  
  //JSON  A ENVIAR POR POST
    json.clear();                                                        
    json["CodMaquina"] = wifi_ssid,                  
    json["CodParo"] = wifi_password,                    
    json["CodUnicoParo"] = local_IP,          
    json["FechaHoraIniParo"] = "",         
    json["FechaHoraFinParo"] = "",            
    json["Rpm"] = timeupdatewifi,                                      

   //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int responseCode = http.POST(json.as<String>());

   if (responseCode < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request wifi");
    http.end();
   }

   if (responseCode != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar credenciales wifi:  ");
    DEBUG_PRINTLN(responseCode);
    http.end();
    //appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text

   }

   if (responseCode == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos wifi enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(apiResponse, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = apiResponse["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();
    

    }



 }
 
 */



}
