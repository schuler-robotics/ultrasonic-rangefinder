Simple and Low Cost Ultrasonic Rangefinder
------------------------------------------
Adding basic component level analog circuits to low cost digital microcontrollers reduces development time, cost, and complexity of electromechanical-software prototypes.  In this project, the Raspberry Pi Pico integrated resistors and PWM are used in a novel way to prototype an ultrasonic rangefinder transmitter and receiver.  The goal is to achieve high functionality with minimal resources.  When combined with the Pico for clocking, the transistor, diode, capacitor, and hand wound inductor in Figure (1) generate voltage levels sufficient for ultrasound signaling.

The target application for this project is a yard-scale positioning system for an autonomous lawn mower.  Typical autonomous mowers rely on RTK (real time kinetic) GPS, trained neural network vision systems, or random motion within a buried sense wire boundary.  RTK GPS receivers are expensive, and rely on RF error correction data from an external service.  RTK signal providers usually require a paid subscription, if available at all in the mowing location.  Neural network vision systems are complex to implement, and training data requires sufficiently static visual cues in the mowing area.  Random motion mowers are inefficient, with mowing time, and hence battery capacity, an exponential function of mowing area.  Positioning by time of flight ultrasonic measurement may provide a less complex and lower cost solution, while maintaining deterministic motion.


<p float="left">
<img src="https://github.com/schuler-robotics/ultrasonic-rangefinder/blob/master/images/fig01-tx-01-sw-03.jpg" width="300" />
</p>
Figure (1) Analog Boost Converter
<br />
<br />


System Description
------------------
The system under consideration is an ultrasonic (40KHz) transmitter-receiver pair, providing an omnidirectional distance measurement.  The design has a working diameter greater than 32 ft(1), and sub-inch accuracy.

Breadboard prototypes of the transmitter-receiver pair are illustrated in Figure (2), and a short demonstration video is linked to the image.

<p align="center">
<a href="https://youtu.be/8H4MsyW-lHo" title="Video demonstration"><img src="https://github.com/schuler-robotics/ultrasonic-rangefinder/blob/master/images/fig02-ultra-tx-rx-01.jpg" width="500"/></a>
</p>
Figure (2) Ultrasonic Rangefinder Prototype; Click image for video demonstration
<br />
<br />

The transmitter includes an inductive switcher, clocked by the Pico PWM (pulse width modulator), to generate 20V from a 5V battery.  The 20V source powers a level-shifter and high voltage driver, generating a 40V peak-to-peak pulse train.  The high voltage pulses are fed to parallel piezo transducers (speakers) coupled to a 3D printed reflector to spread the signal uniformly in all lateral directions.

The converter output, determined by the PWM duty cycle and LC component selection, is voltage clamped to 20V by the level-shifter transistors.  With a distance update rate of hundreds of milliseconds, the heat generated to power a 20 microsecond ultrasound pulse is insignificant.  A heat sink may be added to the switcher transistor to accommodate single digit millisecond distance update rates.  The inductor and capacitor values are selected to support the required output current and voltage ripple.  

The schematic and simulation results of the transmitter are shown in Figure (3). The resources inside the dashed lines represent the GPIO (general purpose input/output) signals, including pin models, that physically reside on the Pico microcontroller.

<p float="left">
<img src="https://github.com/schuler-robotics/ultrasonic-rangefinder/blob/master/images/fig03-tx-01-transmitter-04-simb-2.jpg" width="800" />
</p>
Figure (3) Transmitter Schematic and Simulation Results
<br />
<br />


The receiver consists of a single piezo transducer (microphone) tied to a 4 transistor amplifier feeding the Pico ADC (analog to digital converter).  The 10X NPN gain stage biasing, normally prototyped with a network of discrete resistors, are provided by the pull-up and pull-down resistors on the PICO GPIO pins.

The schematic and simulation results of the receiver are shown in Figure (4).  The ADC, as well as the 7 pull-up and pull-down resistors, inside dashed lines physically reside on the Pico microcontroller.  The current source and passive devices in the larger dashed line box comprise a mathematical model of the received transmitter pulse.

<p float="left">
<img src="https://github.com/schuler-robotics/ultrasonic-rangefinder/blob/master/images/fig04-rx-04-receiver-04-sima.jpg" width="800" />
</p>
Figure (4) Receiver Schematic and Simulation Results
<br />
<br />


Five LEDs on the receiver board provide visual feedback during operation.  Four LEDs are grouped linearly to provide a binary estimate (1 ft resolution) of the measured distance, with the green LED indicating the LSB (least significant bit).  One additional red led provides a 'signal lost' bit which is illuminated when the transmitter is out of range or obstructed.

Distance Measurement
--------------------
Sound waves in room temperature air travel at approximately 1ft/millisecond.  The precise value is strongly dependent on temperature, and weakly dependent on frequency and air composition (e.g. humidity).

The time of flight for a signal traveling at 1ft per second is well within the measurement range of the Pico's 1us resolution timers.  Flight time from transmitter to receiver is converted to distance by the equation distance = velocity * time.  The precise velocity of the signal is determined by the system during a startup calibration procedure.  If the system is initialized (turned on) at a known distance, say 1 ft, the flight time for the first received pulse may be used to calculate the signal velocity by solving the distance equation for time.  The four LEDs, indicating distance in binary, flash in unison once the system is calibrated and ready to measure arbitrary distances. Figure (5) shows the approximate 1ft/ms relationship between the orange transmitted pulse and the magenta received pulse.

<p float="left">
<img src="https://github.com/schuler-robotics/ultrasonic-rangefinder/blob/master/images/fig05-rx-02-1ft-per-ms.jpg" width="600" />
</p>
Figure (5) Transmit and Receive Pulses Timing at 1ft
<br />
<br />

Another non-ideality which must be accounted for is clock frequency skew between the transmitter and receiver Picos.  Figure (6) shows an approximately constant skew of 18us per pulse between the transmitter and receiver clocks.  This skew depends on environmental factors, such as process variation of the Picos, battery voltage, and temperature.  Similar to the velocity calibration, this skew may be auto-compensated at startup by measuring the skew of successive pulses.

<p float="left">
<img src="https://github.com/schuler-robotics/ultrasonic-rangefinder/blob/master/images/fig06-tx01-rx04-timing-skew.jpg" width="500" />
</p>
Figure (6) Frequency Skew between Transmitter and Receiver
<br />
<br />

The signal intensity of sound waves, including ultrasound, is inversely proportional to the square of distance between transmission and reception.  The working distance of the system depends on the ability of the receiver to reliably resolve a valid pulse.  In addition to signal intensity, the line of sight angle between transmitter and receiver contributes to working distance.  Figure (7) shows the distance-intensity relationship between transmitter and receiver.  In this example, the omnidirectional reflector was removed to give a sense of intensity variation due to line of sight angle.  The intensity variation of the three 18ft pulses is due to manually 'aiming' the receiver and transmitter.  All of the pulses in Figure (7) were manually aimed. Although the sensitivity to angle is reduced with the transmitter reflector in place, the working distance is reduced by roughly a factor of 2 due to absorption and spreading.

<p float="left">
<img src="https://github.com/schuler-robotics/ultrasonic-rangefinder/blob/master/images/fig07-pico-ux-tx-01-rx-04-dist-exp-02.jpg" width="500" />
</p>
Figure (7) Received Pulses vs Distance
<br />
<br />

Cost
----
The single unit retail cost of the Pico is 4USD.  The 10 unit cost of the TCT-40 piezo transducers are 0.7USD per element.  The remaining components used for this project were re-purposed from obsolete boards, or easily fabricated (e.g. the 32uH hand wound inductor).  The transmitter reflector and piezo transducer mount was 3D printed on a Monoprice Voxel 3D printer (350USD, 0.4mm nozzle diameter.)  Circuit simulations were accomplished using the freely available LTspice simulator, and mechanical models were generated on open source Freecad software.

Final Thoughts
--------------
The Ultrasonic-rangefinder project depicted here affects a single line of sight distance measurement.  Multiple methods exist to enhance this into full Cartesian positioning.  One such method is coupling this rangefinder with a dead reckoning device, such as a silicon accelerometer.  Alternatively, two additional receive paths may be added (the Pico has 3 ADC channels) for triangulation navigation.

Logistics
---------
The software controlling the RX/TX pair is written in C/C++, using the well documented Pico SDK (software development kit). Source code directory descriptions are as follows:
- rcs-tx01-02 - Transmitter Pico firmware
- rcs-rx04-03 - Receiver Pico firmware
- rcs-common  - Common Pico utility functions

Thank you for your time.  I welcome your questions and feedback.

<pre>
Ray Schuler
schuler at usa.com
</pre>

<p float="left">
<img src="https://github.com/schuler-robotics/ultrasonic-rangefinder/blob/master/images/mower-01-concept-A-20220505.jpg" width="350" />
<img src="https://github.com/schuler-robotics/ultrasonic-rangefinder/blob/master/images/mower-01-photo-20220517.jpg" width="350" />
</p>
An ultrasonic rangefinder receiver will sit atop this 3D printed robot lawn mower under development.  While I enjoy designing, building, and programming robots, I'm not a fan of mowing my lawn.
<br />
<br />


<p float="left">

</p>

<br />
<br />

(1) At the time of this writing, imperial units are widely used in the USA for lawn mower cutting diameters and lawn dimensions-- inches and ft^2, respectively.  Other distances will be given in MKS units, e.g. mm^2, as appropriate.  To paraphrase Andy Weir in his scifi book "Hail Mary", sometimes it's hard being an American scientist.
