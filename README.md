# ESP32 Wi-Fi Connection Demostration

Hi. The purpose of doing this project is to demostrate how to establish a Wi-Fi connection, and then use it communicate with a TCP server for an ESP32 microcontroller using the ESP-IDF API.


## How to build 
The project can be build using [ESP-IDF](https://idf.espressif.com/). To compile the project, simply..
 1. `git clone` this repository.
 2. Navigate to the repository using the ESP-IDF Terminal.
 3. `idf.py set-target esp32` to tell the compiler to target ESP32, but you can target other ESP devices as well.
 4. `idf.py menuconfig` to build the menus and the `sdkconfig` file.
 5. In the `menuconfig`, go to **WIFI Configuration** > **WiFi SSID** and enter the name of the network you want to connect to.
 6. **WIFI Configuration** > **WiFi Password** to enter the password to this access point. If it is an open network, then simply leave the setting blank.
 7. Open `main/main.c` and enter the IP address and the port number of the running server at the appropriate location.
 8. Exit the menu, and in the IDF Terminal, enter `idf.py build` to build the project.

 ## How to run

 This project can be run by running `idf.py flash` with your ESP32 connected to your computer. However, here I will be using the [Wokwi](https://wokwi.com/) online simulator to run this project.

 To run this project in the Wokwi simulator, install the [Wokwi for VS Code Extension](https://docs.wokwi.com/vscode/getting-started). Then, open the project directory in VS Code and open the `wokwi/diagram.json` file. This will open a graphical simulation of the microcontroller.
 
 **!!Important note!!** - If using the Wokwi Simulator, use the WiFi network **"Wokwi-GUEST"** with no password.


The project is properly running if you can send text data from your TCP server, and it shows in the serial monitor.

Enter `HELLO` and press the `Enter` at the server to terminate the project.
