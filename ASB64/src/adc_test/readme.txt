adc_test

	build:
		$ arm-cortex_a9-linux-gnueabi-gcc -o adc_test adc_test.c -DARCH_S5P6818

	test:
		Usage: adc_test
                [-m TestMode]        (ASB or VTK. default:VTK)
                [-t torelance]       (default:10)
                [-n sample count]    (default:50)
                [-v voltage]         (0~1.8V. default:1.800000)
                [-c channel]         (0~8. 0:all, else:channel. default:0)

		ex> (ASB)
				$ ./adc_test -m ASB -t 5 -n 10

			(VTK)
				$ ./adc_test -m VTK -t 10 -n 50 -v 1.2 -c 3

		description:
			read "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
			   ~ "/sys/bus/iio/devices/iio:device0/in_voltage7_raw"
			 to check get voltage value success.

			'ASB mode' checks channel 1 to 8 with preset voltage.
				Some channels get voltage from pwm.


		return:
			EXIT_SUCCESS(0) is sucess, Otherwise errno

