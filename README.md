# Advanced-Projects---App-Design
This is a part of Mr. Mauro's Hibotics Project.

Here is the Arduino Client for MQTT API documentation: https://pubsubclient.knolleary.net/api.html

Here are the notable qualities of each version:

V1: Basic receiving messages using the delay() function. The hall effect sensor stuff is not implemented whatsoever.

V2: Still uses delay()s, but the hall effect sensor stuff is in. The only thing that has not been tested is the convergence algorithm.

V2.5: USE THIS VERSION!!!!!! It has no delay()s, works basically perfectly (with the exception of the convergence stuff), and you can do two tasks at once (g.g. turn on the LED as the motor runs). This only works if one of the tasks uses a timer and the other one does not. It also sends out a stop command so the button on the app turns off. The convergence stuff is left out so it does not waste space. 

Experimental Versions (DO NOT USE, PLEASE!!!!!!!!!!!! This is only for Nathaniel Shalev to mess with):

V2.6: Same as 2.5 but has the convergence stuff. It needs to be cleaned up more.

V2.75: An attempt to reconcile the functional timer stuff (from 2.5) with the buffer system.

V3: Just the buffer system with an incorrect timer system. This is the most hectic version.

There will be a link to see the documentation of the app itself soon. Promise.
