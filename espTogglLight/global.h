ESP8266WebServer server(80);    // The Webserver
#define SWITCHPOSITIONS 12 //number of positions on position-switch
#define ANALOGMAX 970 //maximum value that is read from analog (should be 1023, can be less, not clear why)


struct ESPConfig {
  String ssid; //32 bytes maximum
  String password; //32 bytes maximum
  String APIkey;  //17 bytes (16 byte key plus termination zero)
} config;


const char* host = "www.toggl.com";
const int httpsPort = 443;

// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprint = "D1 27 31 46 99 DF FE C1 30 B2 00 7D 6C DE 62 02 86 AC 2D 54"; //got it using https://www.grc.com/fingerprints.htm

String userID;
String workspaceID[12]; //12 workspaces max
String workspaceName[12]; //12 workspaces max
String projectID[12]; //12 projects max
String projectName[12]; //12 projects max
String runningTimerID; //ID of currently running timer
bool isrunning = false; //true if timer is running (checked during setup)
bool startpressed; //button interrupt checking
bool stoppressed;
uint8_t switchposition;
bool switch_changed;
long projectstarttime; //millis time current project time tracking started (currently for display reference only)
String authentication; //authentication string, created during setup from API key using Base64 encoding (see API documentation)

void displaytime(long val); //function prototype




