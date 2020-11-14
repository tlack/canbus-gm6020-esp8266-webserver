// demo: CAN-BUS Shield, receive data with check mode
// send data coming to fast, such as less than 10ms, you can use this way
// loovee, 2014-6-13

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SPI.h>
#include "mcp_can.h"

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = D0;
const int LED        = LED_BUILTIN;
boolean ledON        = 1;
const int CAN_CTRL_ID = 0x1FF;
const int MOTOR_ID = 4;

// how sensitive is the position sensor? tends to drift +/- 2 to me..
#define POS_THRESH 1
#define WEB_FREQ 100
#define GOAL_TIME 30*1000
#define GOAL_CURRENT 1024

#define HOSTNAME "canbot0"
#define SSID "YOUR-WIFI"
#define PASS "YOUR-PASSWORD"

MCP_CAN CAN(SPI_CS_PIN);
ESP8266WebServer server(80);

uint8_t has_goal = 0; 
int16_t goal_pos = -0xFFFF;
uint8_t last_goal_attained = 0;
uint32_t goal_start = 0;
uint32_t last_state_update = 0;
uint16_t last_pos;
int16_t last_rpm;
int16_t last_amps;
uint8_t last_temp;
uint32_t last_web;
uint32_t loop_cnt = 0;

void setup()
{
    Serial.begin(115200);
    pinMode(LED,OUTPUT);

    Serial.println("starting wifi..");
    WiFi.begin(SSID, PASS);
    while (WiFi.status() != WL_CONNECTED) delay(100);
    Serial.print("wifi up: "); Serial.println(WiFi.localIP());
    if (MDNS.begin(HOSTNAME)) {
      Serial.print("mdns up: "); Serial.println(HOSTNAME);
    } else Serial.println("mdns error");
    Serial.println("starting canbus..");
    while (CAN_OK != CAN.begin(CAN_1000KBPS)) delay(100);
    Serial.println("canbus up");

    setup_webserver();
}

void serve_root() {
  String html = "<!doctype html>"
  "<script>"
  "var START_DELAY=50;"
  "function emit(x) { console.log('e',x); return x; }"
  "function $(x,y) { z=typeof(x) == 'string' ? document.querySelector(x) : x; z.innerHTML = y; }"
  "function $$(x, y) { z=document.querySelectorAll(x); for(var i=0; i<z.length; i++){y(z[i], x)} }"
  "function upd() { var r=this.responseText; $('#state', emit(r)); fresh(); }"
  "function fresh() { x = new XMLHttpRequest(); x.onload=upd; x.open('GET', '/get'); x.send(); }"
  "setTimeout(fresh,START_DELAY);"
  "</script>"
  "<div id=h>" HOSTNAME "</div>"
  "<div id=state></div>"
  "<div id=ui></div>";  
  server.send(200, "text/html", html);
}

void serve_get() {
  char buf[256];
  snprintf(buf, sizeof(buf), 
    "{\"ga\":%d,\"p\":%d,\"r\":%d,\"a\":%d,\"t\":%d}", 
    last_goal_attained, 
    last_pos, last_rpm, last_amps, last_temp);
  server.send(200, "text/html", buf);
}

void serve_set() {
  char buf[256];
  if (server.args() == 0) {
    server.send(500, "text/html", "no args");
    return;
  } 
  int16_t goal;
  char argbuf[256] = {0};
  server.arg(0).toCharArray(argbuf, sizeof(argbuf));
  if (sscanf(argbuf, "%d", &goal) != 1){
    server.send(500, "text/html", "bad goal");
    return;
  }
  goal_pos = goal;
  has_goal = 1;
  last_goal_attained = 0;
  goal_start = millis();
  snprintf(buf, sizeof(buf), "{\"gp\":%d}", goal_pos);
  server.send(200, "text/html", buf);
}

void setup_webserver() {
  server.on("/", serve_root);
  server.on("/get", serve_get);
  server.on("/set", serve_set);
  server.begin();
  Serial.print("http up: http://"); Serial.println(WiFi.localIP());
}

void exec(int16_t c) {
  uint8_t buf[8] = {0};
  int ofs = (MOTOR_ID - 1) * 2;
  buf[ofs] = c >> 8;
  buf[ofs + 1] = c;
  // Serial.print("exec:"); Serial.print(c); Serial.print(" -> "); for(int i=0; i<8; i++) { Serial.print(buf[i], HEX); Serial.print(" "); } Serial.println();
  CAN.sendMsgBuf(0x1FF, 0, 8, (unsigned char*)buf); 
}

void fwd() {
  Serial.println("+");
  exec(GOAL_CURRENT);
}
void back() {
  Serial.println("-");
  exec(-1 * GOAL_CURRENT);
}

void exec_goal() {
  if (abs(last_pos - goal_pos) < POS_THRESH) {
    Serial.print("got goal!! ");
    Serial.println(last_pos);
    has_goal = 0;
    last_goal_attained = 1;
  }
  if (abs(millis() - goal_start) > GOAL_TIME) {
    Serial.print("goal expired: ");
    Serial.println(goal_pos);
    has_goal = 0;
    last_goal_attained = 0;
  }
  if (last_pos < goal_pos) fwd(); 
  else back();
}

void loop() {
    unsigned char len = 0;
    unsigned char buf[8];

    if(CAN_MSGAVAIL == CAN.checkReceive())            // check if data coming
    {
        CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf

        unsigned long canId = CAN.getCanId();

        uint16_t pos = buf[0] << 8 | buf[1];
        int16_t rpm = buf[2] << 8 | buf[3];
        int16_t amps = buf[4] << 8 | buf[5];
        uint8_t temp = buf[6]; 

        last_pos = pos;
        last_rpm = rpm;
        last_amps = amps;
        last_temp = temp;

        if (has_goal) exec_goal();
        
        last_state_update = millis();
        // Serial.print(".");
    }
    if (millis() - last_web > WEB_FREQ) {
        server.handleClient();
        MDNS.update();
        last_web = millis();
    }
}

//END FILE
