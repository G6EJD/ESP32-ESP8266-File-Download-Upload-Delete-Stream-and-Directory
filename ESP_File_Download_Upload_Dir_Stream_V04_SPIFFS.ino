/*  Version 4
 *  
 *  ESP32/ESP8266 example of downloading and uploading a file from or to the device's Filing System, including Directory and Streaming of files.
 *  
 This software, the ideas and concepts is Copyright (c) David Bird 2019. All rights to this software are reserved.
 
 Any redistribution or reproduction of any part or all of the contents in any form is prohibited other than the following:
 1. You may print or download to a local hard disk extracts for your personal and non-commercial use only.
 2. You may copy the content to individual third parties for their personal use, but only if you acknowledge the author David Bird as the source of the material.
 3. You may not, except with my express written permission, distribute or commercially exploit the content.
 4. You may not transmit it or store it in any other website or other form of electronic retrieval system for commercial purposes.

 The above copyright ('as annotated') notice and this permission notice shall be included in all copies or substantial portions of the Software and where the
 software use is visible to an end-user.
 
 THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT. FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY 
 OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 See more at http://www.dsbird.org.uk
 *
*/
#ifdef ESP8266
  #include <ESP8266WiFi.h>       // Built-in
  #include <ESP8266WebServer.h>  // Built-in
#else
  #include <WiFi.h>              // Built-in
  #include <WebServer.h>
  #include "SPIFFS.h"
#endif

#define ServerVersion "1.0"
String  webpage = "";
bool    SPIFFS_present = false;

const char ssid[]     = "your SSID";
const char password[] = "your PASSWORD";

#include "FS.h"
#include "CSS.h"
#include <SPI.h>

#ifdef ESP8266
  ESP8266WebServer server(80);
#else
  WebServer server(80);
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void setup(void){
  Serial.begin(115200);

  WiFi.begin(ssid,password);
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(250); Serial.print('.');
  }
  Serial.println("\nConnected to "+WiFi.SSID()+" Use IP address: "+WiFi.localIP().toString()); // Report which SSID and IP is in use
  #ifdef ESP8266
  if (!SPIFFS.begin()) {
  #else
  if (!SPIFFS.begin(true)) {
  #endif  
    Serial.println("SPIFFS initialisation failed...");
    SPIFFS_present = false; 
  }
  else
  {
    Serial.println(F("SPIFFS initialised... file access enabled..."));
    SPIFFS_present = true; 
  }
  //----------------------------------------------------------------------   
  ///////////////////////////// Server Commands 
  server.on("/",         HomePage);
  server.on("/download", File_Download);
  server.on("/upload",   File_Upload);
  server.on("/fupload",  HTTP_POST,[](){ server.send(200);}, handleFileUpload);
  server.on("/stream",   File_Stream);
  server.on("/delete",   File_Delete);
  server.on("/dir",      SPIFFS_dir);
  
  ///////////////////////////// End of Request commands
  server.begin();
  Serial.println("HTTP server started");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void loop(void){
  server.handleClient(); // Listen for client connections
}

// All supporting functions from here...
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void HomePage(){
  SendHTML_Header();
  webpage += F("<a href='/download'><button>Download</button></a>");
  webpage += F("<a href='/upload'><button>Upload</button></a>");
  webpage += F("<a href='/stream'><button>Stream</button></a>");
  webpage += F("<a href='/delete'><button>Delete</button></a>");
  webpage += F("<a href='/dir'><button>Directory</button></a>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop(); // Stop is needed because no content length was sent
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Download(){ // This gets called twice, the first pass selects the input, the second pass then processes the command line arguments
  if (server.args() > 0 ) { // Arguments were received
    if (server.hasArg("download")) DownloadFile(server.arg(0));
  }
  else SelectInput("Enter filename to download","download","download");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void DownloadFile(String filename){
  if (SPIFFS_present) { 
    File download = SPIFFS.open("/"+filename,  "r");
    if (download) {
      server.sendHeader("Content-Type", "text/text");
      server.sendHeader("Content-Disposition", "attachment; filename="+filename);
      server.sendHeader("Connection", "close");
      server.streamFile(download, "application/octet-stream");
      download.close();
    } else ReportFileNotPresent("download"); 
  } else ReportSPIFFSNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Upload(){
  append_page_header();
  webpage += F("<h3>Select File to Upload</h3>"); 
  webpage += F("<FORM action='/fupload' method='post' enctype='multipart/form-data'>");
  webpage += F("<input class='buttons' style='width:40%' type='file' name='fupload' id = 'fupload' value=''><br>");
  webpage += F("<br><button class='buttons' style='width:10%' type='submit'>Upload File</button><br>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  server.send(200, "text/html",webpage);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
File UploadFile; 
void handleFileUpload(){ // upload a new file to the Filing system
  HTTPUpload& uploadfile = server.upload(); // See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/srcv
                                            // For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
  if(uploadfile.status == UPLOAD_FILE_START)
  {
    String filename = uploadfile.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("Upload File Name: "); Serial.println(filename);
    SPIFFS.remove(filename);                  // Remove a previous version, otherwise data is appended the file again
    UploadFile = SPIFFS.open(filename, "w");  // Open the file for writing in SPIFFS (create it, if doesn't exist)
  }
  else if (uploadfile.status == UPLOAD_FILE_WRITE)
  {
    if(UploadFile) UploadFile.write(uploadfile.buf, uploadfile.currentSize); // Write the received bytes to the file
  } 
  else if (uploadfile.status == UPLOAD_FILE_END)
  {
    if(UploadFile)          // If the file was successfully created
    {                                    
      UploadFile.close();   // Close the file again
      Serial.print("Upload Size: "); Serial.println(uploadfile.totalSize);
      webpage = "";
      append_page_header();
      webpage += F("<h3>File was successfully uploaded</h3>"); 
      webpage += F("<h2>Uploaded File Name: "); webpage += uploadfile.filename+"</h2>";
      webpage += F("<h2>File Size: "); webpage += file_size(uploadfile.totalSize) + "</h2><br>"; 
      append_page_footer();
      server.send(200,"text/html",webpage);
    } 
    else
    {
      ReportCouldNotCreateFile("upload");
    }
  }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#ifdef ESP32
void SPIFFS_dir(){ 
  if (SPIFFS_present) { 
    File root = SPIFFS.open("/");
    if (root) {
      root.rewindDirectory();
      SendHTML_Header();
      webpage += F("<h3 class='rcorners_m'>SD Card Contents</h3><br>");
      webpage += F("<table align='center'>");
      webpage += F("<tr><th>Name/Type</th><th style='width:20%'>Type File/Dir</th><th>File Size</th></tr>");
      printDirectory("/",0);
      webpage += F("</table>");
      SendHTML_Content();
      root.close();
    }
    else 
    {
      SendHTML_Header();
      webpage += F("<h3>No Files Found</h3>");
    }
    append_page_footer();
    SendHTML_Content();
    SendHTML_Stop();   // Stop is needed because no content length was sent
  } else ReportSPIFFSNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void printDirectory(const char * dirname, uint8_t levels){
  File root = SPIFFS.open(dirname);
  if(!root){
    return;
  }
  if(!root.isDirectory()){
    return;
  }
  File file = root.openNextFile();
  while(file){
    if (webpage.length() > 1000) {
      SendHTML_Content();
    }
    if(file.isDirectory()){
      webpage += "<tr><td>"+String(file.isDirectory()?"Dir":"File")+"</td><td>"+String(file.name())+"</td><td></td></tr>";
      printDirectory(file.name(), levels-1);
    }
    else
    {
      webpage += "<tr><td>"+String(file.name())+"</td>";
      webpage += "<td>"+String(file.isDirectory()?"Dir":"File")+"</td>";
      int bytes = file.size();
      String fsize = "";
      if (bytes < 1024)                     fsize = String(bytes)+" B";
      else if(bytes < (1024 * 1024))        fsize = String(bytes/1024.0,3)+" KB";
      else if(bytes < (1024 * 1024 * 1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
      else                                  fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";
      webpage += "<td>"+fsize+"</td></tr>";
    }
    file = root.openNextFile();
  }
  file.close();
}
#endif
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#ifdef ESP8266
void SPIFFS_dir(){
  String str;
  if (SPIFFS_present) { 
    Dir dir = SPIFFS.openDir("/");
    SendHTML_Header();
    webpage += F("<h3 class='rcorners_m'>SPIFFS Card Contents</h3><br>");
    webpage += F("<table align='center'>");
    webpage += F("<tr><th>Name/Type</th><th style='width:40%'>File Size</th></tr>");
    while (dir.next()) {
      Serial.print(dir.fileName());
      webpage += "<tr><td>"+String(dir.fileName())+"</td>";
      str  = dir.fileName();
      str += " / ";
      if(dir.fileSize()) {
        File f = dir.openFile("r");
        Serial.println(f.size());
        int bytes = f.size();
        String fsize = "";
        if (bytes < 1024)                     fsize = String(bytes)+" B";
        else if(bytes < (1024 * 1024))        fsize = String(bytes/1024.0,3)+" KB";
        else if(bytes < (1024 * 1024 * 1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
        else                                  fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";
        webpage += "<td>"+fsize+"</td></tr>";
        f.close();
      }
      str += String(dir.fileSize());
      str += "\r\n";
      Serial.println(str);
    }
    webpage += F("</table>");
    SendHTML_Content();
    append_page_footer();
    SendHTML_Content();
    SendHTML_Stop();   // Stop is needed because no content length was sent
  } else ReportSPIFFSNotPresent();
}
#endif
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Stream(){
  if (server.args() > 0 ) { // Arguments were received
    if (server.hasArg("stream")) SPIFFS_file_stream(server.arg(0));
  }
  else SelectInput("Enter a File to Stream","stream","stream");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SPIFFS_file_stream(String filename) { 
  if (SPIFFS_present) { 
    File dataFile = SPIFFS.open("/"+filename,  "r"); // Now read data from SPIFFS Card 
    if (dataFile) { 
      if (dataFile.available()) { // If data is available and present 
        String dataType = "application/octet-stream"; 
        if (server.streamFile(dataFile, dataType) != dataFile.size()) {Serial.print(F("Sent less data than expected!")); } 
      }
      dataFile.close(); // close the file: 
    } else ReportFileNotPresent("Cstream");
  } else ReportSPIFFSNotPresent(); 
}   
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Delete(){
  if (server.args() > 0 ) { // Arguments were received
    if (server.hasArg("delete")) SPIFFS_file_delete(server.arg(0));
  }
  else SelectInput("Select a File to Delete","delete","delete");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SPIFFS_file_delete(String filename) { // Delete the file 
  if (SPIFFS_present) { 
    SendHTML_Header();
    File dataFile = SPIFFS.open("/"+filename, "r"); // Now read data from SPIFFS Card 
    if (dataFile)
    {
      if (SPIFFS.remove("/"+filename)) {
        Serial.println(F("File deleted successfully"));
        webpage += "<h3>File '"+filename+"' has been erased</h3>"; 
        webpage += F("<a href='/delete'>[Back]</a><br><br>");
      }
      else
      { 
        webpage += F("<h3>File was not deleted - error</h3>");
        webpage += F("<a href='delete'>[Back]</a><br><br>");
      }
    } else ReportFileNotPresent("delete");
    append_page_footer(); 
    SendHTML_Content();
    SendHTML_Stop();
  } else ReportSPIFFSNotPresent();
} 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Header(){
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate"); 
  server.sendHeader("Pragma", "no-cache"); 
  server.sendHeader("Expires", "-1"); 
  server.setContentLength(CONTENT_LENGTH_UNKNOWN); 
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves. 
  append_page_header();
  server.sendContent(webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Content(){
  server.sendContent(webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Stop(){
  server.sendContent("");
  server.client().stop(); // Stop is needed because no content length was sent
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SelectInput(String heading1, String command, String arg_calling_name){
  SendHTML_Header();
  webpage += F("<h3>"); webpage += heading1 + "</h3>"; 
  webpage += F("<FORM action='/"); webpage += command + "' method='post'>"; // Must match the calling argument e.g. '/chart' calls '/chart' after selection but with arguments!
  webpage += F("<input type='text' name='"); webpage += arg_calling_name; webpage += F("' value=''><br>");
  webpage += F("<type='submit' name='"); webpage += arg_calling_name; webpage += F("' value=''><br><br>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportSPIFFSNotPresent(){
  SendHTML_Header();
  webpage += F("<h3>No SPIFFS Card present</h3>"); 
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportFileNotPresent(String target){
  SendHTML_Header();
  webpage += F("<h3>File does not exist</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportCouldNotCreateFile(String target){
  SendHTML_Header();
  webpage += F("<h3>Could Not Create Uploaded File (write-protected?)</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
String file_size(int bytes){
  String fsize = "";
  if (bytes < 1024)                 fsize = String(bytes)+" B";
  else if(bytes < (1024*1024))      fsize = String(bytes/1024.0,3)+" KB";
  else if(bytes < (1024*1024*1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
  else                              fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";
  return fsize;
}
