# AC-Monitor

Here should be everything you need to get this running.

1. Suggest set static IP address for the Hubitat hub and the esp8266. 

2. Log into your Hubitat hub, goto “Diver Code” then at the top right select “New Driver” copy the Hubitat driver from Github and paste it into the new driver, then save it.

3. Now on the Hubitat select ”Devices”, now at the top right select “Add Virtual Device”, at the “type drop down menu select “Matt’s AC Monitor” it should be the last thing on the list, give it a “Device Name” i.e. Bedroom AC, hit “Save”.

4. Now select “Apps”, at top right select “Add Built-In App”, select “Maker API”, change the “Maker API Label” if you want, select “Select Devices”, select the device you made in step 3, select done.

5. Now select “Apps” again, at the bottom of the screen you should see something similar to this Get Device Info (replace [Device ID] with actual subscribed device id http://192.168.1.180/apps/api/934/devices/[Device ID]?access_token=77c764cf-0b31-4f6d-9403-3c452295d26a
write down the number after apps/api/ also your going to need the access_token

6. Now select “Apps” again, then select “Get All Devices” should get a pop up window right down the “id” number.

7. Now open a new sketch in the Ardunio IDE with the ESP8266AC.ino file or you can download file.

8. Input all data you got from steps 1 through 6 into the ESP8266AC sketch the up load to your esp8266 circuit card.
