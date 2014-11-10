# LDR-Ethernet

Read an LDR pulsecounter over the network.

We all know the scenario in which some unconnectable peice of equipment (like a power meter), blinks continuously but wont give you an actual readout. By using an LDR an Arduino and a network module, we can fix this.

## Hardware
* Any Arduino or compatible device with at least 1kB RAM (ATmega168 and up)
* One LDR (mine gives 1ohm to 5ohm of resitance, and I've based my complementary resitors on this)
* ENC28J60 Ethernet module (SPI interface)

## Pin connections
The sketch assumes a certain hardware pin connection. The layout below is based on an Arduino Uno and may be different for other Arduino's (notably, the SPI pins are different on the Leonardo and Mega).

For the ENC28J60 module, connect the following (these are etherCard defaults):
* VCC -> 3.3V
* GND -> GND
* SCK (clock)    -> pin 13 (Hardware SPI clock)
* SO (slave out) -> pin 12 (Hardware SPI master-in)
* SI (slave in)  -> pin 11 (Hardware SPI master-out)
* CS (chip-select) -> pin 8

For the LDR-strip:
* LDR -> pin 2
* LDR -> 5v

The chip-select pin for the ENC28J60 is user configurable, the LDR needs to be connected to a port with an interrupt.

## Libraries
* https://github.com/jcw/ethercard
