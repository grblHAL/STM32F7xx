## grblHAL driver for STM32F7xx processors

This driver can be built with the [Web Builder](http://svn.io-engineering.com:8080/?driver=STM32F7xx),
for other alternatives see the Wiki-page for [compiling grblHAL](https://github.com/grblHAL/core/wiki/Compiling-GrblHAL) for instructions for how to import the project, configure the driver and compile.

__NOTE:__ This is an initial version, currently only bench tested with an oscillocope using a [Nucleo-F756ZG board](https://www.st.com/en/evaluation-tools/nucleo-f756zg.html).

My intention is to as a reference driver as the Nucleo board has plenty of GPIO pins, native USB and ethernet.

Available driver options can be found [here](Inc/my_machine.h).

---
2023-09-20
