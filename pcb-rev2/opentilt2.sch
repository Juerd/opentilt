EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:special
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:arduino
LIBS:opentilt2-cache
EELAYER 27 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date "18 nov 2013"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L CONN_6 P1
U 1 1 5232286D
P 3500 3250
F 0 "P1" V 3450 3250 60  0000 C CNN
F 1 "CONN_6" V 3550 3250 60  0000 C CNN
F 2 "~" H 3500 3250 60  0000 C CNN
F 3 "~" H 3500 3250 60  0000 C CNN
	1    3500 3250
	1    0    0    -1  
$EndComp
$Comp
L R RB1
U 1 1 523228F2
P 2800 3000
F 0 "RB1" V 2880 3000 40  0000 C CNN
F 1 "R" V 2807 3001 40  0000 C CNN
F 2 "~" V 2730 3000 30  0000 C CNN
F 3 "~" H 2800 3000 30  0000 C CNN
	1    2800 3000
	0    1    1    0   
$EndComp
$Comp
L R RG1
U 1 1 52322904
P 2800 3100
F 0 "RG1" V 2880 3100 40  0000 C CNN
F 1 "R" V 2807 3101 40  0000 C CNN
F 2 "~" V 2730 3100 30  0000 C CNN
F 3 "~" H 2800 3100 30  0000 C CNN
	1    2800 3100
	0    1    1    0   
$EndComp
$Comp
L R RR1
U 1 1 5232290A
P 2800 3200
F 0 "RR1" V 2880 3200 40  0000 C CNN
F 1 "R" V 2807 3201 40  0000 C CNN
F 2 "~" V 2730 3200 30  0000 C CNN
F 3 "~" H 2800 3200 30  0000 C CNN
	1    2800 3200
	0    1    1    0   
$EndComp
Wire Wire Line
	3050 3000 3150 3000
Wire Wire Line
	3050 3100 3150 3100
Wire Wire Line
	3050 3200 3150 3200
$Comp
L CONN_12 APM1
U 1 1 5288F956
P 1850 1550
F 0 "APM1" V 1800 1550 60  0000 C CNN
F 1 "CONN_12" V 1900 1550 60  0000 C CNN
F 2 "~" H 1850 1550 60  0000 C CNN
F 3 "~" H 1850 1550 60  0000 C CNN
	1    1850 1550
	1    0    0    -1  
$EndComp
$Comp
L CONN_12 APM2
U 1 1 5288F965
P 1850 2900
F 0 "APM2" V 1800 2900 60  0000 C CNN
F 1 "CONN_12" V 1900 2900 60  0000 C CNN
F 2 "~" H 1850 2900 60  0000 C CNN
F 3 "~" H 1850 2900 60  0000 C CNN
	1    1850 2900
	1    0    0    -1  
$EndComp
Text GLabel 1450 1000 0    60   Input ~ 0
TX
Text GLabel 1450 1100 0    60   Input ~ 0
RX
Text GLabel 1450 1400 0    60   Input ~ 0
D2
Text GLabel 1450 1500 0    60   Input ~ 0
D3
Text GLabel 1450 1600 0    60   Input ~ 0
D4
Text GLabel 1450 1700 0    60   Input ~ 0
D5
Text GLabel 1450 1800 0    60   Input ~ 0
D6
Text GLabel 1450 1900 0    60   Input ~ 0
D7
Text GLabel 1450 2000 0    60   Input ~ 0
D8
Text GLabel 1450 2100 0    60   Input ~ 0
D9
Wire Wire Line
	1450 1000 1500 1000
Wire Wire Line
	1500 1100 1450 1100
Wire Wire Line
	1450 1400 1500 1400
Wire Wire Line
	1500 1500 1450 1500
Wire Wire Line
	1450 1600 1500 1600
Wire Wire Line
	1500 1700 1450 1700
Wire Wire Line
	1450 1800 1500 1800
Wire Wire Line
	1450 1900 1500 1900
Wire Wire Line
	1500 2000 1450 2000
Wire Wire Line
	1450 2100 1500 2100
Text GLabel 1450 2350 0    60   Input ~ 0
VIN
Text GLabel 1450 2450 0    60   Input ~ 0
GND
Text GLabel 1450 2650 0    60   Input ~ 0
5V
Text GLabel 1450 2750 0    60   Input ~ 0
A3
Text GLabel 1450 2850 0    60   Input ~ 0
A2
Text GLabel 1450 2950 0    60   Input ~ 0
A1
Text GLabel 1450 3050 0    60   Input ~ 0
A0
Text GLabel 1450 3150 0    60   Input ~ 0
CLK
Text GLabel 1450 3250 0    60   Input ~ 0
MISO
Text GLabel 1450 3350 0    60   Input ~ 0
MOSI
Text GLabel 1450 3450 0    60   Input ~ 0
D10
Wire Wire Line
	1450 2350 1500 2350
Wire Wire Line
	1500 2450 1450 2450
Wire Wire Line
	1450 2550 1500 2550
Wire Wire Line
	1500 2650 1450 2650
Wire Wire Line
	1450 2750 1500 2750
Wire Wire Line
	1500 2850 1450 2850
Wire Wire Line
	1450 2950 1500 2950
Wire Wire Line
	1500 3050 1450 3050
Wire Wire Line
	1450 3150 1500 3150
Wire Wire Line
	1500 3250 1450 3250
Wire Wire Line
	1450 3350 1500 3350
Wire Wire Line
	1500 3450 1450 3450
$Comp
L CONN_5 ACC1
U 1 1 52890342
P 2850 1200
F 0 "ACC1" V 2800 1200 50  0000 C CNN
F 1 "CONN_5" V 2900 1200 50  0000 C CNN
F 2 "~" H 2850 1200 60  0000 C CNN
F 3 "~" H 2850 1200 60  0000 C CNN
	1    2850 1200
	1    0    0    -1  
$EndComp
$Comp
L CONN_5 ACC2
U 1 1 52890351
P 2850 1800
F 0 "ACC2" V 2800 1800 50  0000 C CNN
F 1 "CONN_5" V 2900 1800 50  0000 C CNN
F 2 "~" H 2850 1800 60  0000 C CNN
F 3 "~" H 2850 1800 60  0000 C CNN
	1    2850 1800
	1    0    0    -1  
$EndComp
Text GLabel 2350 1200 0    60   Input ~ 0
A2
Text GLabel 2350 1100 0    60   Input ~ 0
A1
Text GLabel 2350 1000 0    60   Input ~ 0
A0
Text GLabel 2350 1300 0    60   Input ~ 0
3V3
Text GLabel 2350 1600 0    60   Input ~ 0
D7
Text GLabel 2350 1700 0    60   Input ~ 0
3V3
Text GLabel 2350 1800 0    60   Input ~ 0
GND
Text GLabel 2350 1900 0    60   Input ~ 0
3V3
Wire Wire Line
	2350 1000 2450 1000
Wire Wire Line
	2450 1100 2350 1100
Wire Wire Line
	2350 1200 2450 1200
Wire Wire Line
	2350 1300 2450 1300
Wire Wire Line
	2450 1600 2350 1600
Wire Wire Line
	2350 1700 2450 1700
Wire Wire Line
	2450 1800 2350 1800
Wire Wire Line
	2350 1900 2450 1900
NoConn ~ 2450 2000
NoConn ~ 2450 1400
$Comp
L CONN_4X2 NRF1
U 1 1 5289079E
P 2900 2600
F 0 "NRF1" H 2900 2850 50  0000 C CNN
F 1 "CONN_4X2" V 2900 2600 40  0000 C CNN
F 2 "~" H 2900 2600 60  0000 C CNN
F 3 "~" H 2900 2600 60  0000 C CNN
	1    2900 2600
	1    0    0    -1  
$EndComp
Text GLabel 2450 2450 0    60   Input ~ 0
GND
Text GLabel 2450 2550 0    60   Input ~ 0
D8
Text GLabel 2450 2650 0    60   Input ~ 0
CLK
Text GLabel 2450 2750 0    60   Input ~ 0
MISO
Text GLabel 3350 2650 2    60   Input ~ 0
MOSI
Text GLabel 3350 2550 2    60   Input ~ 0
D10
NoConn ~ 3300 2750
Wire Wire Line
	2450 2450 2500 2450
Wire Wire Line
	2500 2550 2450 2550
Wire Wire Line
	2450 2650 2500 2650
Wire Wire Line
	2500 2750 2450 2750
Wire Wire Line
	3350 2650 3300 2650
Wire Wire Line
	3350 2550 3300 2550
Wire Wire Line
	3350 2450 3300 2450
Text GLabel 2500 3100 0    60   Input ~ 0
D6
Text GLabel 2500 3000 0    60   Input ~ 0
D9
Text GLabel 2500 3300 0    60   Input ~ 0
GND
Text GLabel 2500 3400 0    60   Input ~ 0
GND
Text GLabel 2500 3500 0    60   Input ~ 0
D2
Wire Wire Line
	2550 3000 2500 3000
Wire Wire Line
	2500 3100 2550 3100
Wire Wire Line
	2550 3200 2500 3200
Wire Wire Line
	2500 3300 3150 3300
Wire Wire Line
	3150 3400 2500 3400
Wire Wire Line
	2500 3500 3150 3500
Wire Wire Line
	1450 1200 1500 1200
Wire Wire Line
	1500 1300 1450 1300
Text GLabel 1450 1300 0    60   Input ~ 0
GND
NoConn ~ 1500 1300
NoConn ~ 1500 1200
NoConn ~ 1500 1100
NoConn ~ 1500 1000
NoConn ~ 1500 2550
NoConn ~ 1500 2350
Text GLabel 3350 2450 2    60   Input ~ 0
3V3
Text GLabel 2500 3200 0    60   Input ~ 0
D5
$EndSCHEMATC
