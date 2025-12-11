# Flipper esctester application

![ESC tester application screenshot](esctester/assets/ESC_Tester_Before_ARM.png) ![ESC tester application screenshot](esctester/assets/ESC_Tester_Manual_mode.png)

This application could be used for testing devices containing standard [ESC](https://en.wikipedia.org/wiki/Motor_controller), by generating a [PWM RC servo signal](https://en.wikipedia.org/wiki/Servo_control) with some edits. 

## Installation instructions

Please install the application from [Flipper Zero application catalog](https://docs.flipper.net/apps), or go to the releases, download apps.zip, and extract it to your SD card.

## Usage

- Connect the ESC PWM input to **A7** Flipper Zero in.
- Enable 5V output in the GPIO menu or plug-in USB-C charging cable. (If you're supplying power from an external power source, make sure it has common ground with the flipper GND)
- Start the ESC Tester app
- Press OK for arm a controller (some controllers needs this)
- Use arrow keys for edit ESC power

## Controls

| Button | Action                       |
| :----- | :--------------------------- |
| 🔼     | Up button increases pulse width by 10 us.|
| 🔽     | The down button decreases pulse width by 10 us.   |
| ◀️     | Left button decreases pulse width by 1 us.   |
| ▶️     | The right button increases pulse width by 1 us.  |
| ↩️     | Back button exit application.   |
| 🔵     | Center button arm controller. |

## Build instructions

Install [uFBT - micro Flipper Build Tool](https://github.com/flipperdevices/flipperzero-ufbt) to get a toolkit for building flipper zero applications. 

- Clone this git repository
- cd to flipper-esctester.
- run *ufbt && ufbt launch* with flipper connected. It automatically installs the compiled application into Flipper Zero.

