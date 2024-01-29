#include <arduino-timer.h>
#include "Arduino_GigaDisplay_GFX.h"
#include <WiFi.h>

#define DEBUG 0

#if DEBUG == 1
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x)
#define debugln(x)
#endif

#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;        // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;
// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
// IPAddress server(93,184,216,34);   // IP address for example.com (no DNS)
char server[] = "google.com";         // host name for example.com (using DNS)

WiFiClient client;

GigaDisplay_GFX display; // create the object
#define screen_size_x 480
#define screen_size_y 800

#define BLACK         0x0000
#define TIMER_DELAY   2000

#define DEFAULT_ROWS  32

auto timer = timer_create_default(); // create a timer with default settings

static const byte rows = 32;
struct SharedData
{
  char pages_ptr[rows][256];
  uint16_t row_ndx;
}shared_data;

static uint8_t current_page = 0;

struct SharedData *shared_data_ptr = &shared_data;
static bool newData = false;

static int reprint = 1;

void displayDrawText(char *text) {
  if (display.getCursorY() > (screen_size_y - 10)) {
    display.fillScreen(BLACK);
    display.setCursor(0, 0);
  }
  display.print(text);
}

void displayDrawText(char *text, bool clear = false) {
  if (clear) {
    display.fillScreen(BLACK);
    display.setCursor(0, 0);
  }
  
  display.print(text);
}

char *decoratepages_ptr(char (*pages_ptr)[256], byte rows = 10, byte start = 0) {
  IPAddress ip = WiFi.localIP();
  char *ssid = WiFi.SSID();

  char *output_buf = (char *) malloc (1024 * sizeof(char));
  char i_bytes[5] = {0};
  char start_bytes[5] = {0};
  char rows_bytes[5] = {0};
  char row_ndx_bytes[5] = {0};
  byte iter_rows = rows;
  uint8_t page = current_page;

  start = 0;
  if (page > 0) {
    start = (page * rows) + 1;

    if (start >= shared_data_ptr->row_ndx) {
      current_page = 0;
      start = 0;
    }
  }

  debug("current_page: ");
  debugln(current_page);

  iter_rows = start + rows;

  // guard against over-indexing past the max threshold
  if (iter_rows > shared_data_ptr->row_ndx) {
    iter_rows = shared_data_ptr->row_ndx;
  }

  // Serial.print("start: ");
  // Serial.println(start);
  // Serial.print("iter_rows: ");
  // Serial.println(iter_rows);

  itoa(iter_rows, rows_bytes, 10);
  itoa(shared_data_ptr->row_ndx, row_ndx_bytes, 10);
  itoa(start, start_bytes, 10);

  // FIXME copy first bytes to buffer
  strcpy(output_buf, "SSID: ");
  
  strcat(output_buf, ssid);
  strcat(output_buf, "\n");
  strcat(output_buf, "IP: ");
  strcat(output_buf, ip.toString().c_str());
  strcat(output_buf, "\n\n");

  strcat(output_buf, "Rows: ");
  strcat(output_buf, start_bytes);
  strcat(output_buf, " - ");
  strcat(output_buf, rows_bytes);
  strcat(output_buf, " / ");
  strcat(output_buf, row_ndx_bytes);
  strcat(output_buf, "\n\n");

  for (int i = start; i < (iter_rows); i++) {
    itoa(i, i_bytes, 10);
    strcat(output_buf, "Row[");
    strcat(output_buf, i_bytes);
    strcat(output_buf, "]: ");
    strcat(output_buf, (char *) pages_ptr[i]);
    strcat(output_buf, "\n");

    // Serial.print("pages_ptr[");
    // Serial.print(i);
    // Serial.print("]: ");
    // Serial.println((char *) pages_ptr[i]);
  }

  if (shared_data_ptr->row_ndx > rows) {
    strcat(output_buf, "\n-- (N)ext page --\n");
  }

  return output_buf;
}

bool timer_callback(void *) {
  String output = "timer_callback = " + String(millis());
  debugln(output);

  // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // toggle the LED

  // if (display.getCursorY() > (screen_size_y - 10)) {
  //   display.fillScreen(BLACK);
  //   display.setCursor(0, 0);
  // }
  // display.println(output);

  reprint = 1;
  current_page++;

  return true; // repeat? true
}

void setup() {
  #if DEBUG == 1
  Serial.begin(115200);
  while (!Serial);
  #endif

  memset(shared_data_ptr, 0, sizeof(*shared_data_ptr));

  display.begin();
  display.setRotation(90);
  display.fillScreen(BLACK);
  display.setTextSize(3); //adjust text size
  display.setCursor(0, 0);
  display.println("booting...");
  display.print("connecting to http://");
  display.print(server);

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_SHIELD) {
    debugln("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    debug("Attempting to connect to SSID: ");
    debugln(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 3 seconds for connection:
    delay(3000);
  }
  debugln("Connected to wifi");
  printWifiStatus();
  // printWifiStatusToDisplay();

  debugln("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) {
    debugln("connected to server");
    // Make a HTTP request:
    client.println("GET /index.html HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("Connection: close");
    client.println();
  }

  debugln("System online");

  timer.every(TIMER_DELAY, timer_callback);
}

void loop(){
  timer.tick();

  static uint16_t ndx = 0;
  char endMarker = '\n';
  char i_bytes[5] = {0};

  // if there are incoming bytes available
  // from the server, read them and print them:
  while (client.available()) {
    char c = client.read();

    if (c != endMarker) {
      shared_data_ptr->pages_ptr[shared_data_ptr->row_ndx][ndx] = c;
      ndx++;
    } else {
      shared_data_ptr->pages_ptr[shared_data_ptr->row_ndx][ndx] = '\0';
      shared_data_ptr->row_ndx++;

      ndx = 0;

      newData = true;
    }
    
    // guard overflow
    if (ndx >= (std::numeric_limits<uint16_t>::max() - 1)) {
      ndx = 0;
    }
    if (shared_data_ptr->row_ndx >= (std::numeric_limits<byte>::max() - 1)) {
      shared_data_ptr->row_ndx = 0;
    }
  }

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    // NOTE: block on `newData`
    if (reprint == 1) {
      byte rows = 6;

      if (newData) {
        debugln("HTTP response received, rendering pages_ptr.");

        char *decorated = decoratepages_ptr(shared_data_ptr->pages_ptr, rows);
        displayDrawText(decorated, true);
        // Serial.println(decorated);

        free(decorated);
      } else {
        debugln("Disconnected from server, rendering existing page details.");

        char *decorated_cpy = decoratepages_ptr(shared_data_ptr->pages_ptr, rows);
        displayDrawText(decorated_cpy, true);
        free(decorated_cpy);
      }

      reprint = 0;
    }
    newData = false;

    client.stop();
  }
}

// void printWifiStatusToDisplay() {
//   IPAddress ip = WiFi.localIP();

//   char ssid[16];
//   strcat(ssid, "SSID: ");
//   strcat(ssid, WiFi.SSID());
//   displayDrawText(ssid);
//   // displayDrawText("IP: " + ip);
// }

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  debug("SSID: ");
  debugln(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  debug("IP Address: ");
  debugln(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  debug("signal strength (RSSI):");
  debug(rssi);
  debugln(" dBm");
}