# Bayang2PPM
Use Bayang transmitter to get PPM signal out

Attention!!! Use this code at your own risks. It wasn't working well enough for me. The PPM signal seems to be lost for a fraction of second and this turned to be enough the quadcopter was unflyable.

This project is using a cheap (actually very bad transmitters comparing to a FlySky I6 and similar) transmitter like this ![Screenshot](https://raw.githubusercontent.com/0zoon0/Bayang2PPM/master/images/cont2.webp) or this ![Screenshot](https://raw.githubusercontent.com/0zoon0/Bayang2PPM/master/images/contr1.jfif) to read control values and output PPM signal.
It requires an arduino (micro, nano, uno, etc) and nrf24 communication module.

See images folder for the receiver build. Note that PPM output is on D2 pin.
