----- prompt -----
If this is code to test all joints of robotic arm using d1 mini:
/Users/mayankarora/dib/poc/arduino-projects/2026/mayank/d1_mini_pca8965_serial_control

and line 75-80 is the mapping of joints with servo channels with corresponding min/max and deafult.

Could you generate another d1 mini code, referring to :
/Users/mayankarora/dib/poc/home-assistant/docs/md/robot-arm.md
/Users/mayankarora/dib/poc/home-assistant/docs/md/robot-arm-hardware.md
and /Users/mayankarora/dib/poc/arduino-projects/2026/mayank/di_mini_mqtt_pan_tilt/di_mini_mqtt_pan_tilt.ino

In a new folder mayank/d1_mini_pca8965_mqtt_control, so I could find and connect it to home assistant application.
----- prompt ends -----

## How it connects

```
home-assistant app (SERVO_ARM "first-servo-arm", id 33)
   --> homeassistant/sensor/servo_arm_33/state   {"base":0.0,"boom":0.0,"arm":90.0,...}
         --> this D1 Mini --> PCA9685 CH0-CH5
               --> homeassistant/sensor/d1_mini_robot_arm/{config,state,attributes,availability}
                     --> Home Assistant device "D1 Mini Robot Arm"
```

1. Libraries: PubSubClient, ArduinoJson 7.x, Adafruit PWM Servo Driver (all in `../libraries`).
2. Check `MQTT_TOPIC_ARM_STATE` matches your servo arm's sensor id, then flash.
3. In the app, connect `first-servo-arm` and press a joint button: the arm follows.
4. In Home Assistant: Settings > Devices > "D1 Mini Robot Arm" (entity `sensor.d1_mini_robot_arm`,
   plus a **Home** button `button.d1_mini_robot_arm_home`).
5. Home / Reset on http://localhost:4025/robotic-arm.html (or the HA Home button) publishes
   `PRESS` to `homeassistant/button/d1_mini_robot_arm/home`; every joint ramps to its servo default.

Before moving the arm through MQTT for the first time, check the `scale` sign for each joint
in `JOINTS` (use -1 if that joint moves the wrong way).
