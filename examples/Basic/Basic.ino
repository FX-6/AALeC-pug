#include <AALeC-V2.h>
#include <AALeC-pug.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

#include "main.h"

// Your wifi ssid
const String ssid = "your wifi ssid";
// Your wifi password
const String password = "your wifi password";

// The web server
ESP8266WebServer server(80);

/**
 * @brief The setup
 */
void setup() {
    // Init aalec
    aalec.init();

    // Mount LittleFS
    Serial.println("Mounting LittleFS...");

    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed");
        return;
    }

    Serial.println("LittleFS mounted");

    // Connect to WiFi
    Serial.printf("Connecting to '%s'...\n", ssid.c_str());

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    Serial.printf(
        "Connected, IP address: %s\n",
        WiFi.localIP().toString().c_str()
    );

    aalec.print_line(1, "IP: " + WiFi.localIP().toString());

    // Start web server
    Serial.println("Starting server...");

    server.onNotFound(handleRequest);
    server.begin();

    Serial.println("Server started");
}

/**
 * @brief Main loop
 */
void loop() {
    server.handleClient();
}

/**
 * @brief Get the Mime Type of a file based on the file extension
 *
 * @param path Path to the file
 * @return String Mime Type, defaults to "text/plain"
 */
String getMimeType(String path) {
    if (path.endsWith(".html")) {
        return "text/html";
    } else if (path.endsWith(".css")) {
        return "text/css";
    } else if (path.endsWith(".js")) {
        return "application/javascript";
    } else if (path.endsWith(".json")) {
        return "application/json";
    } else {
        return "text/plain";
    }
}

/**
 * @brief Handle an incoming request
 */
void handleRequest() {
    // Get the requested URI
    String uri = ESP8266WebServer::urlDecode(server.uri());

    // Send a response
    if (uri.endsWith("/")) {
        sendIndex(uri);
    } else if (uri.endsWith(".pug")) {
        sendPug(uri);
    } else {
        sendFile(uri);
    }
}

/**
 * @brief Check for index files in a directory
 *
 * @param path Path to the directory (with trailing slash)
 */
void sendIndex(String path) {
    // Check for index.pug and index.html files
    if (LittleFS.exists(path + "index.pug")) {
        // Found the index.pug file, compile it and send the result
        sendPug(path + "index.pug");
    } else if (LittleFS.exists(path + "index.html")) {
        // Found the index.html file (but no index.pug file), send it directly
        sendFile(path + "index.html");
    } else {
        // Found neither the index.pug nor the index.html file
        server.send(
            404,
            "text/plain",
            "Index File not found: '" + path + "index.pug' or '" + path
                + "index.html'"
        );
    }
}

/**
 * @brief Compile a pug file and send it to the client
 *
 * @param path Path to the pug file
 */
void sendPug(String path) {
    // Open the pug file
    File pugFile = LittleFS.open(path, "r");

    // Check if the pug file exists
    if (!pugFile.isFile()) {
        server.send(404, "text/plain", "PUG File not found: '" + path + "'");
        return;
    }

    // Create the compiled file path
    String outFilePath = path + ".html";

    // Compile the pug file
    if (!aalec_pug(path, outFilePath)) {
        // Failed to compile the pug file
        server.send(
            500,
            "text/plain",
            "Failed to compile PUG file '" + path + "'"
        );
        return;
    }

    // Open the compiled file
    File outFile = LittleFS.open(outFilePath, "r");

    // Check if the compiled file exists
    if (!outFile.isFile()) {
        // Failed to open the compiled file
        server.send(
            500,
            "text/plain",
            "Failed to read compiled PUG file '" + outFilePath + "'"
        );
        return;
    }

    // Send the compiled file
    server.streamFile(outFile, "text/html");
}

/**
 * @brief Try to send a file to the client
 *
 * @param path Path to the file that should be sends
 */
void sendFile(String path) {
    // Open the file
    File file = LittleFS.open(path, "r");

    // Check if the file exists
    if (!file.isFile()) {
        // File not found or is not a file
        server.send(404, "text/plain", "Normal File not found: '" + path + "'");
        return;
    }

    // Send the file
    server.streamFile(file, getMimeType(path));
}
