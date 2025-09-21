# Smart_MediBox
Medibox – Developed a smart medicine box using ESP32 with DHT22, LDR, and a servo motor for automated monitoring and controlled access. Real-time data and device settings are managed through MQTT on a Node-RED dashboard. The system was designed, simulated, and tested using Wokwi.

## 🚀 Features
- ⏰ **Medicine Reminder** – configurable alarms for medication schedules
- 🌡️ **Temperature & Humidity Monitoring** – via DHT22 sensor
- 💡 **Light Intensity Tracking** – via LDR sensor
- 🔄 **Servo Motor Control** – for automated medicine access
- 📊 **Node-RED Dashboard** – gauges, charts, and control sliders
- 📤 **MQTT Communication** – using HiveMQ public broker
- 🎛️ **Dynamic Configurations** – set sampling intervals, snooze/stop alarms, adjust ideal storage conditions
- 🧪 **Fully Simulated on Wokwi** – no physical hardware required

- ## 📁 Repository Structure
| File/Folder        | Description |
|-------------------|-------------|
| `sketch.ino`      | Main ESP32 code to handle sensors, servo, and MQTT |
| `wokwi-project.txt` | Wokwi simulation project ID |
| `diagram.json`    | Wokwi circuit diagram (ESP32 + DHT22 + LDR + Servo) |
| `flows.json`      | Node-RED flow for UI and MQTT topics |
| `libraries.txt`   | Required libraries for Wokwi or Arduino IDE |
