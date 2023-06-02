#include <WiFi.h>       
#include <Arduino.h>
#include "freertos/queue.h"
#include <Bounce2.h>    
#include <ESP32Time.h>  
#include <HTTPClient.h>  
#include <ArduinoJson.h> 
#include <FS.h>
#include <LittleFS.h>
#include <esp_system.h>  


//fata el codigo en el .ini de littlefs

/*--------BERIFICAR DATOS----------*/
/*
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
bool debugEnabled = true;  // Habilita/Deshabilita depuracion   //dejarlo en false
#define DEBUG_PRINT(x)  if(debugEnabled) { Serial.print(x); }
#define DEBUG_PRINTLN(x)  if(debugEnabled) { Serial.println(x); }


/*--------DECLARANDO PINES----------*/
#define IN1 23                                           // entrada 1  START  23 //Tarj. Matzuya M.    out:  33, start :  23,  conteo:   35
#define IN2 25                                           // entrada 2  FALLA HILO SUPERIOR    25
#define IN3 32                                           // entrada 3  FALLA HILO LATERAL
#define IN4 33                                           // entrada 4  FALLA CAIDA DE TELA
#define IN5 34                                     // entrada 5  PRODUCCION (contador y rpm)
#define IN6 35                                           // entrada 6  FALLA LYCRA
#define RL1 18                                           // salida 1   RELE 1
#define RL2 19                                           // salida 2   RELE 2
#define RL3 21 


/*--------WIFI----------*/
const char *wifi_ssid = "starlink";                     //    WF_TDV_PLC 
const char *wifi_password = "70322511";                 //  M4qPLC$*-+/
IPAddress local_IP(192, 168, 239, 170);                  //IPAddress local_IP(172, 16, 2, 201);
IPAddress gateway(192, 168, 239, 94);                   //IPAddress gateway(172, 16, 2, 1);
IPAddress subnet(255, 255, 255, 0);                      //IPAddress subnet(255, 255, 254, 0);
IPAddress primary_dns(192, 168, 239, 94);               // Google DNS primario   //131, 107, 32, 217
//IPAddress secondary_dns(131, 107, 32, 218);            // Google DNS secundario   //131, 107, 32, 218

unsigned long ultimoIntento = 0;   
int contador_reconectarWiFi = 0;
const int intervaloReconexion = 1000;

void reconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {   

    unsigned long tiempoActual = millis();
    if (tiempoActual - ultimoIntento >= intervaloReconexion) {
      ultimoIntento = tiempoActual;
      
      if (contador_reconectarWiFi == 0) {
        DEBUG_PRINT("\n\n\nReconectando a la red Wi-Fi  ");
        WiFi.disconnect();


         // Usar IP estática y DNS                                        
        if (!WiFi.config(local_IP, gateway, subnet, primary_dns))   {   //(!WiFi.config(local_IP, gateway, subnet, primary_dns, secondary_dns)) - (!WiFi.config(local_IP, gateway, subnet))
        DEBUG_PRINT("\n\nError al configurar la IP estática y DNS");     //Serial.println("Error al configurar la IP estática y DNS"); -  Serial.println("Error al configurar la IP estática");
        }

        WiFi.begin(wifi_ssid, wifi_password);
      }
      
      DEBUG_PRINT(".");
      contador_reconectarWiFi++;

      if (contador_reconectarWiFi > 10) {
        
        DEBUG_PRINT("\n\nConexion WIFI Fallida Reintentando ");
        contador_reconectarWiFi = 0;
      }
    }
  } else {

    if (contador_reconectarWiFi > 0) {
    
      DEBUG_PRINT("\n\n           Conexion WIFI Exitosa   ->    IP Tarjeta :  ");
      DEBUG_PRINT(WiFi.localIP());
     
    }
    contador_reconectarWiFi = 0;
  }

}




/*--------SERVIDOR NTP----------*/
const char* ntpServer = "192.168.239.161";         //Servidor NTP TDV 131.107.32.217 
const long gmtOffset_sec = -5*3600;                //Desplazamiento GMT
const int daylightOffset_sec = 0;                  //Compensacion de luz diurna
ESP32Time rtc;


/*--------VARIABLES GLOVALES----------*/

String url_api = "http://192.168.239.161/BackEnd2/api/Rectilineo/Write"; //http://192.168.54.161/BackEnd2/api/Rectilineo/Write

const char* codigo_maquina = "R609";   
const char* codigo_paro_in1 = "MP001";  
const char* codigo_paro_in2 = "MP004"; 
const char* codigo_paro_in3 = "MP003";
const char* codigo_paro_in4 = "44444";  
const char* codigo_paro_in5 = "55555"; 
const char* codigo_paro_in6 = "66666";
const char* codigo_paro_in7 = "77777";  
const char* codigo_paro_in8 = "88888"; 
const char* codigo_paro_in9 = "99999";

volatile unsigned long count = 0;    //Variable para contador de unidades

const unsigned long INTERVAL1 = 1 * 60 * 1000; //Envio de datos desde littlefs cada 5 min
unsigned long previousTime1 = 0;

int timeupdatewifi = 1;
const unsigned long INTERVAL2 = timeupdatewifi * 60 * 1000; //Envio de datos para actualizar Wifi
unsigned long previousTime2 = 0;





DynamicJsonDocument json(2048);
DynamicJsonDocument api_response(256);
HTTPClient http;
QueueHandle_t queue;  //Cola

Bounce debouncer1 = Bounce();
Bounce debouncer2 = Bounce();
Bounce debouncer3 = Bounce();


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

    vTaskDelay(1); // Agrega un pequeño retardo para liberar el procesador
  
  }
}


/*--------INDICADORES LED----------*/
void updateLedStatusWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(RL2, LOW);  // Apagar LED
  } else {
    digitalWrite(RL2, HIGH); // Encender LED
  }
}

void updateLedStatusInput() {
  if (digitalRead(IN1) == HIGH || digitalRead(IN2) == HIGH || digitalRead(IN3) == HIGH)
   { digitalWrite(RL1, HIGH);
   } else {
    digitalWrite(RL1, LOW);
   }
}


/*--------CONTADOR DE UNIDADES----------*/
void IRAM_ATTR countInterrupt() {
  count++;   // Incrementar el contador cada vez que se detecte un cambio en el pin de entrada
}


/*--------ALMACENAMIENTO FLASH----------*/
void displayLittleFSSpace() {
  
  // Calcular el porcentaje de espacio utilizado
  float percentageUsed = (float)LittleFS.usedBytes() / (float)LittleFS.totalBytes() * 100;

  // Mostrar el porcentaje de espacio utilizado con dos decimales
  Serial.print("\n     Espacio Usado de Memoria: ");
  Serial.printf("%.2f", percentageUsed);
  Serial.println(" %");

}

void initializeLittleFS() {

if (!LittleFS.begin()) {  //!
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

        int response_code = http.POST(line);

        if (response_code < 0) {
          DEBUG_PRINT("\n\nError al intentar enviar Post Request desde LittleFS ");
          http.end();
          return;
        }

        if (response_code != 200) {
          DEBUG_PRINT("\n\nError en response al intentar enviar desde LittleFS :  ");
          DEBUG_PRINTLN(response_code);
          http.end();
          return;
        }

        if (response_code == 200) {
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


 



 WiFi.setHostname("DeviceTejRecR609");


 configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //ntp

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
 pinMode(IN5, INPUT_PULLUP); //pin count 5 se utiliza para interrupcion
 pinMode(IN6, INPUT);  //Interrupcion
 pinMode(RL1, OUTPUT);
 digitalWrite(RL1, LOW);
 pinMode(RL2, OUTPUT);
 digitalWrite(RL2, LOW);
 pinMode(RL3, OUTPUT);
 digitalWrite(RL3, LOW);


 attachInterrupt(digitalPinToInterrupt(IN6), countInterrupt, FALLING);  // Establece interrupción en el pin de entrada 5
 

  
 debouncer1.attach(IN1);
 debouncer1.interval(5); // intervalo en ms

 debouncer2.attach(IN2);
 debouncer2.interval(5); // intervalo en ms

 debouncer3.attach(IN3);
 debouncer3.interval(5); // intervalo en ms


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
 if (debouncer3.read() == HIGH) 
   {
   InputInfo initialInfo3 = {IN3, HIGH};
   xQueueSend(queue, &initialInfo3, 0); //xQueueSend(queue, &initialInfo2, portMAX_DELAY);
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
  updateLedStatusInput();






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
     static String  codigo_static_in1 = "";

        if(info.state == HIGH) 

        {  codigo_static_in1 = rtc.getTime("%Y%m%d%H%M%S");  // Actualizar el código cuando PIN1 esté en alto
 
  //JSON  A ENVIAR POR POST
    json.clear();                                                        
    json["CodMaquina"] = codigo_maquina,                  
    json["CodParo"] = codigo_paro_in1,                    
    json["CodUnicoParo"] = codigo_static_in1,          
    json["FechaHoraIniParo"] = rtc.getTime("%FT%T"),         
    json["FechaHoraFinParo"] = "",            
    json["Rpm"] = 0,                                      

   //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int response_code = http.POST(json.as<String>());

   if (response_code < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (response_code != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(response_code);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (response_code == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(api_response, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = api_response["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
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
    json["CodMaquina"] = codigo_maquina,                 
    json["CodParo"] = codigo_paro_in1,                   
    json["CodUnicoParo"] = codigo_static_in1,          
    json["FechaHoraIniParo"] = "",          
    json["FechaHoraFinParo"] = rtc.getTime("%FT%T"),            
    json["Rpm"] = count,                                      

    //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int response_code = http.POST(json.as<String>());

   if (response_code < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (response_code != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(response_code);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (response_code == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(api_response, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = api_response["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();

      }

      codigo_static_in1 = "";
      count = 0;
   
    }
     
  }

  
 /*-------------------------DIGITAL IMPUT 2---------------------------*/
  if(info.pin == IN2)

  {
     static String  codigo_static_in2 = "";

        if(info.state == HIGH) 

        {  codigo_static_in2 = rtc.getTime("%Y%m%d%H%M%S");  // Actualizar el código cuando PIN1 esté en alto
 
  //JSON  A ENVIAR POR POST
    json.clear();                                                                
    json["CodMaquina"] = codigo_maquina,                   
    json["CodParo"] = codigo_paro_in2,                     
    json["CodUnicoParo"] = codigo_static_in2,          
    json["FechaHoraIniParo"] = rtc.getTime("%FT%T"),          
    json["FechaHoraFinParo"] = "",             
    json["Rpm"] = 0,                                  

   //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int response_code = http.POST(json.as<String>());

   if (response_code < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (response_code != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(response_code);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (response_code == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(api_response, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = api_response["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();

    }

  } 
        
 else
        
 {

  //JSON  A ENVIAR POR POST
    json.clear();                                                              
    json["CodMaquina"] = codigo_maquina,                  
    json["CodParo"] = codigo_paro_in2,                     
    json["CodUnicoParo"] = codigo_static_in2,         
    json["FechaHoraIniParo"] = "",         
    json["FechaHoraFinParo"] = rtc.getTime("%FT%T"),            
    json["Rpm"] = 0,                                      

   //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int response_code = http.POST(json.as<String>());

   if (response_code < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (response_code != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(response_code);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (response_code == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(api_response, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = api_response["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();

    }

     codigo_static_in2 = "";

  }
     
 }


 /*-------------------------DIGITAL IMPUT 3---------------------------*/
  if(info.pin == IN3)

  {
     static String  codigo_static_in3 = "";

        if(info.state == HIGH) 

        {  codigo_static_in3 = rtc.getTime("%Y%m%d%H%M%S");  // Actualizar el código cuando PIN1 esté en alto
        
 
  //JSON  A ENVIAR POR POST
    json.clear();                                                                
    json["CodMaquina"] = codigo_maquina,                   
    json["CodParo"] = codigo_paro_in3,                   
    json["CodUnicoParo"] = codigo_static_in3,          
    json["FechaHoraIniParo"] = rtc.getTime("%FT%T"),          
    json["FechaHoraFinParo"] = "",            
    json["Rpm"] = 0,                               

    //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int response_code = http.POST(json.as<String>());

   if (response_code < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (response_code != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(response_code);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (response_code == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(api_response, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = api_response["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();

    }

  } 
        
 else
        
 {

  //JSON  A ENVIAR POR POST
    json.clear();                                                              
    json["CodMaquina"] = codigo_maquina,                   
    json["CodParo"] = codigo_paro_in3,                    
    json["CodUnicoParo"] = codigo_static_in3,          
    json["FechaHoraIniParo"] = "",          
    json["FechaHoraFinParo"] = rtc.getTime("%FT%T"),            
    json["Rpm"] = 0,                                     

    //HTTP POST
   http.begin(url_api);   
   http.addHeader("Content-Type", "application/json"); 

   int response_code = http.POST(json.as<String>());

   if (response_code < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request");
    http.end();
   }

   if (response_code != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar :  ");
    DEBUG_PRINTLN(response_code);
    http.end();
    appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text
   }

   if (response_code == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(api_response, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = api_response["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();

        }

         codigo_static_in3 = "";
      }
      
    }

  }

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
  Serial.print(rtc.getTime("%Y%m%d%H%M%S"));

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



/*-------------------------SE EJECUTA CADA X INTERVALO DE TIEMPO PARA ENVIAR DESDE LITTLEFS---------------------------*/  
 unsigned long currentTime1 = millis();
  if (currentTime1 - previousTime1 >= INTERVAL1) 
  {  previousTime1 = currentTime1;

   sendDataFromLittleFS(); 

  }





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
  if (currentTime2 - previousTime2 >= INTERVAL2) 
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

   int response_code = http.POST(json.as<String>());

   if (response_code < 0)
   {
    DEBUG_PRINT("\n\n         Error al intentar enviar Post Request wifi");
    http.end();
   }

   if (response_code != 200)
   {
    DEBUG_PRINT("\n\n         Error en response al intentar enviar credenciales wifi:  ");
    DEBUG_PRINTLN(response_code);
    http.end();
    //appendDataToFile(json.as<String>());  //agrega el dato que no se envio al archivo data.text

   }

   if (response_code == 200)
   {
    String responseBody = http.getString();
    DEBUG_PRINT("\n\nDatos wifi enviados :  ");
    DEBUG_PRINTLN(json.as<String>());
 
    //deserializeJson(api_response, responseBody);    //deserializa una cadena JSON
    //delay(2000);
    //String codigoresponse = api_response["codUnicoParo"]; // retorna  por (codigoresponse)  el {"CodUnicoParo":"20230322085824"}
    //Serial.println(codigoresponse);
    //Serial.println(responseBody);"

    http.end();
    

    }



 }
 
 */



}


