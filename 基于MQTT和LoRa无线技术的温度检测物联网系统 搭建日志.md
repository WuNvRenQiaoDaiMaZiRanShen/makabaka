# 基于MQTT和LoRa无线技术的温度检测物联网系统 搭建日志

## 材料

| 名称           | 数量 | 备注                                      |
| -------------- | ---- | ----------------------------------------- |
| SX1278LoRa模块 | 2    | AI-thinker RA-02，工作在470Mhz频段        |
| LoRa接口板     | 2    | 将贴片的RA-02的引脚扩展出来，方便开发调试 |
| 排针           |      | 焊接在接口板上                            |
| 天线           |      | 480Mhz                                    |
| ESP32开发板    |      | 使用的AI-thinker的nodemcu                 |
| micro-usb线    |      | node-mcu内置usb转串口，通过usb线连接电脑  |
| 温度传感器     |      | 型号：DS18B02                             |
| 杜邦线         |      | 母对母型杜邦线                            |

## 焊接

* 组员前往科创中心，在老师的指导和帮助下完成焊接

* 焊接实况

  ![焊接实况](https://riyugo.com/i/2021/04/24/10dxghx.png)

## 接线

由于公对公杜邦连接线数量不足，无法借助面包板完成连接，最终直接利用母对母杜邦连接线连接模块针脚完成接线

* 电路连接示意图

  ![电路连接示意图](https://riyugo.com/i/2021/04/24/10dxqpy.png)

* 电路连接效果图

  

## 代码部分

两个单片机使用不同的代码

* 单片机其一

```
/*
  LoRa Node With and DS18B02
  MCU：ESP32-S
  Lora: ai-tinker RA-02 sx1278 470 
  Tempture Sensor:MCP9701A
  created 2020.8.25
  by Bloomlj
*/

#include <LoRaNow.h>
#include "SSD1306Wire.h"        
SSD1306Wire display(0x3c, 27, 26);

//vspi for lora radio module
#define MISO 19
#define MOSI 23
#define SCK 18
#define SS 5

#define DIO0 4

//for 18b20
#include <OneWire.h>
#include <DallasTemperature.h>

// GPIO where the DS18B20 is connected to
const int oneWireBus= 25 ; 

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);
double tmp;

void setup() {
  Serial.begin(115200);
  Serial.println("LoRaNow便携节点");
  pinMode(oneWireBus,INPUT);
  sensors.begin();
   LoRaNow.setFrequencyCN(); // Select the frequency 486.5 MHz - Used in China
  // LoRaNow.setFrequencyEU(); // Select the frequency 868.3 MHz - Used in Europe
  // LoRaNow.setFrequencyUS(); // Select the frequency 904.1 MHz - Used in USA, Canada and South America
  // LoRaNow.setFrequencyAU(); // Select the frequency 917.0 MHz - Used in Australia, Brazil and Chile

  // LoRaNow.setFrequency(frequency);
  // LoRaNow.setSpreadingFactor(sf);
  // LoRaNow.setPins(ss, dio0);

   LoRaNow.setPinsSPI(SCK, MISO, MOSI, SS, DIO0); // Only works with ESP32

  if (!LoRaNow.begin()) {
    Serial.println("LoRa加载失败。检查你的连接。");
    while (true);
  }

  LoRaNow.onMessage(onMessage);
  LoRaNow.onSleep(onSleep);
  LoRaNow.showStatus(Serial);
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
}

void loop() {
  sensors.requestTemperatures(); 
  tmp = sensors.getTempCByIndex(0);
  LoRaNow.loop();
}


void onMessage(uint8_t *buffer, size_t size)
{
  Serial.print("收到信息: ");
  Serial.write(buffer, size);
  Serial.println();
  Serial.println();
}

void onSleep()
{
  display.clear();
  Serial.println("休息中...");
  Serial.print("现在的温度是：");
  Serial.print(tmp);
  Serial.println("℃");
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 0, "Temperature:");
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 32, String(tmp)+"℃");
  display.display();
  delay(5000); // "kind of a sleep"
  Serial.println("发送信息：");
  LoRaNow.print("信息编号");
  LoRaNow.print(millis());
  LoRaNow.print("：");
  LoRaNow.print("现在的温度是：");
  //Serial.println("LoRaNow Message sended");
  LoRaNow.print(tmp);
  LoRaNow.print("℃");
  LoRaNow.send();
}
```

* 单片机其二

```
/*
  LoRaNow Simple Gateway with ESP32 setPins
  This code creates a webServer show the Message and use mqtt publish the data.
  created 27 04 2019
  by Luiz H. Cassettari
  modified 2020.09.19
  by bloomlj
*/

#include <LoRaNow.h>
#include <WiFi.h>
#include <WebServer.h>
#include <StreamString.h>
#include <PubSubClient.h>
#include "SSD1306Wire.h"        
SSD1306Wire display(0x3c, 27, 26);

//vspi
//#define MISO 19
//#define MOSI 23
//#define SCK 18
//#define SS 5

//hspi,only for ESP-32-Lora-Shield-Ra02
#define SCK 14
#define MISO 12
#define MOSI 13
#define SS 15

#define DIO0 4

const char *ssid = "test";
const char *password = "12345678";
const char *mqtt_server = "broker.hivemq.com";

WebServer server(80);
WiFiClient espClient;
PubSubClient mqttclient(espClient);

const char *script = "<script>function loop() {var resp = GET_NOW('loranow'); var area = document.getElementById('area').value; document.getElementById('area').value = area + resp; setTimeout('loop()', 1000);} function GET_NOW(get) { var xmlhttp; if (window.XMLHttpRequest) xmlhttp = new XMLHttpRequest(); else xmlhttp = new ActiveXObject('Microsoft.XMLHTTP'); xmlhttp.open('GET', get, false); xmlhttp.send(); return xmlhttp.responseText; }</script>";

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (60)
char msg[MSG_BUFFER_SIZE];

int value = 0;

void handleRoot()
{
  String str = "";
  str += "<html>";
  str += "<head>";
  str += "<title>Temperature monitor</title>";
  str += "<style type=\"text/css\">body{background: url(https://img.zcool.cn/community/015ea357afda5f0000018c1b2f966d.jpg@1280w_1l_2o_100sh.jpg) repeat-x center top fixed}</style>";
  str += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  str += script;
  str += "</head>";
  str += "<body onload='loop()'>";
  str += "<center>";
  str += "<textarea id='area' style='width:800px; height:400px;'></textarea>";
  str += "</center>";
  str += "</body>";
  str += "</html>";
  server.send(200, "text/html", str);
}

static StreamString string;

void handleLoRaNow()
{
  server.send(200, "text/plain", string);
  while (string.available()) // clear
  {
    string.read();
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("有新消息[");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    //digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    //digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void setup(void)
{

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  //if (ssid != "")
    WiFi.begin(ssid, password);
  WiFi.begin();
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("已经成功连接到：");
  Serial.println(ssid);
  Serial.print("IP地址: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/loranow", handleLoRaNow);
  server.begin();
  Serial.println("已启用HTTP服务器");

  LoRaNow.setFrequencyCN(); // Select the frequency 486.5 MHz - Used in China
  // LoRaNow.setFrequencyEU(); // Select the frequency 868.3 MHz - Used in Europe
  // LoRaNow.setFrequencyUS(); // Select the frequency 904.1 MHz - Used in USA, Canada and South America
  // LoRaNow.setFrequencyAU(); // Select the frequency 917.0 MHz - Used in Australia, Brazil and Chile

  // LoRaNow.setFrequency(frequency);
  // LoRaNow.setSpreadingFactor(sf);
  // LoRaNow.setPins(ss, dio0);

  LoRaNow.setPinsSPI(SCK, MISO, MOSI, SS, DIO0); // Only works with ESP32

  if (!LoRaNow.begin())
  {
    Serial.println("LoRa加载失败。检查你的连接。");
    while (true)
      ;
  }

  LoRaNow.onMessage(onMessage);
  LoRaNow.gateway();
  //mqtt
  mqttclient.setServer(mqtt_server, 1883);
  mqttclient.setCallback(callback);
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
}

void loop(void)
{
  LoRaNow.loop();
  server.handleClient();
  mqttloop();
}

void onMessage(uint8_t *buffer, size_t size)
{
  int i=35,j=0,k=0;
  char tmp[6];
  unsigned long id = LoRaNow.id();
  byte count = LoRaNow.count();
  display.clear();
  Serial.print("节点ID: ");
  Serial.println(id, HEX);
  Serial.print("接收次数: ");
  Serial.println(count);
  Serial.print("信息内容: ");
  Serial.write(buffer, size);
  while ('0'>=buffer[i]||'9'<=buffer[i])i++;
  j=i+5;
  for(i;i<=j;i++,k++) tmp[k]=buffer[i];
  tmp[k-1]='\0';
  Serial.println();
  Serial.println();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 0, "Temperature:");
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 32, tmp);
  display.display();
   //此处通过mqtt发送信息。
   snprintf (msg, MSG_BUFFER_SIZE, "#%d#%s", count,buffer);
   Serial.print("发布信息: ");
   Serial.println(msg);
   mqttclient.publish("C2TLOutTopic", msg);

  if (string.available() > 512)
  {
    while (string.available())
    {
      string.read();
    }
  }

  string.print("节点ID: ");
  string.println(id, HEX);
  string.print("接收次数: ");
  string.println(count);
  string.print("信息内容: ");
  string.write(buffer, size);
  string.println();
  string.println();

  // Send data to the node
  LoRaNow.clear();
  LoRaNow.print("LoRaNow 网关信息");
  LoRaNow.print(millis());
  LoRaNow.send();
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttclient.connected()) {
    Serial.println("尝试与MQTT服务器进行连接");
    // Create a random mqttclient ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqttclient.connect(clientId.c_str())) {
      Serial.println("连接成功");
      // Once connected, publish an announcement...
      mqttclient.publish("C2TLOutTopic", "hello world");
      // ... and resubscribe
      mqttclient.subscribe("C2TLInTopic");
    } else {
      Serial.print("失败, 错误代码：");
      Serial.print(mqttclient.state());
      Serial.println("。5秒后再次尝试。");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void mqttloop() {

  if (!mqttclient.connected()) {
    reconnect();
  }
  mqttclient.loop();

//  unsigned long now = millis();
//  if (now - lastMsg > 2000) {
//    lastMsg = now;
//    ++value;
//    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
//    Serial.print("Publish message: ");
//    Serial.println(msg);
//    mqttclient.publish("C2TLOutTopic", msg);
//  }
}
```





