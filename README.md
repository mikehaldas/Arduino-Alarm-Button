# Arduino-Alarm-Button
This Arduino sketch creates a wireless alarm button. When the button is pressed, it triggers a virtual alarm on a Viewtron IP camera NVR by sending an HTTP Post to the API webhook endpoint on the NVR.
The Arduino code is located in the Alarm_Button.ino file.

The viewtron_alarm_sample.py is a very simple python script that can be used for testing the /TriggerVirtualAlarm API endpoint very easily. The python script is provided for convenience and has nothing to do with the Arduino project.

The project was created by Mike Haldas at CCTV Camera Pros.
mike@viewtron.com

You can learn more about this project by going to:
https://www.viewtron.com/valarm

You can learn more about Viewtron IP camera NVRs here:
https://www.cctvcamerapros.com/IP-Camera-NVRs-s/1472.htm

You can learn more about Viewtron IP cameras here:
https://www.cctvcamerapros.com/viewtron-IP-cameras-s/1474.htm
