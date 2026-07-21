# Smartwatch Interface
This is my first hardware related project, in which I use a *Grove Beginner Kit for Arduino* to create a interface on the OLED Display which is supposed to somewhat imitate a smartwatch.

Although it makes use of an OLED Display, Piezo Buzzer, Button, Rotary Potentiometer, Sound Sensor and a Temperature & Humidity Sensor, I would consider this to be mainly a software project as the whole time I spent on this (roughly 11 hours), was writing the code.

It was very satisfying to write as I had to solve problems like how to get multiple inputs with only a button and a potentiometer, and I ended up using short and long button presses for different input types.

Its features are:
- A password input screen, in which you need to input a 4 digit code, and can only see the main screen if logged in
- A lockdown script, so if you enter an incorrect password more than 2 times it locks you out for a certain amount of times (depending on number of consecutive incorrect login attempts)
- A clock (although it does not know the actual time and assumes time of boot is 12pm)
- A stopwatch with abilities to stop, start from the stopped time, and reset
- A timer for 1 min, 5 min and 10 min, with abilities to pause and restart the timer
- Temperature + Humidity display 
- Sound Monitor, which tells you to stop yelling if the sound quantity is above the sound limit
- Settings, in which you can edit the time format (am+pm/24 hour), sound limit (300/500/700), and temperature unit (C/F)


I learnt a lot about writing code for hardware, how to refactor code so it uses as less memory as possible, how to only redraw the screen when new content is registered, and how to get sensor data as input.
