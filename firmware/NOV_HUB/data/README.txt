NOV_HUB embedded dashboard sources.

These HTML/CSS/JS files are compressed into the ESP32 application image by
tools/embed_dashboard.py during pio run. The Hub serves them at192.168.4.1.
Do not deploy a separate dashboard or run uploadfs for these assets.
The filesystem is used only for optional camera-photo storage.
