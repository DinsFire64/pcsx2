#include "USB/usb-python2/usb-python2.h"
#include "acio.h"

namespace usb_python2
{
	class acio_icca_device : public acio_device_base
	{
	private:
		Python2Input* p2dev;

	protected:
		uint8_t keyLastActiveState = 0;
		uint8_t keyLastActiveEvent[2] = {0, 0};
		bool accept = false;
		bool inserted = false;
		bool isCardInsertPressed = false;
		bool isKeypadSwapped = false;
		bool isKeypadSwapPressed = false;

		bool cardLoaded = false;
		uint8_t cardId[16] = {0};

		std::string keypadIdsByDeviceId[2][12] = {
			{"KeypadP1_0",
				"KeypadP1_1",
				"KeypadP1_2",
				"KeypadP1_3",
				"KeypadP1_4",
				"KeypadP1_5",
				"KeypadP1_6",
				"KeypadP1_7",
				"KeypadP1_8",
				"KeypadP1_9",
				"KeypadP1_00",
				"KeypadP1InsertEject"},
			{"KeypadP2_0",
				"KeypadP2_1",
				"KeypadP2_2",
				"KeypadP2_3",
				"KeypadP2_4",
				"KeypadP2_5",
				"KeypadP2_6",
				"KeypadP2_7",
				"KeypadP2_8",
				"KeypadP2_9",
				"KeypadP2_00",
				"KeypadP2InsertEject"},
		};

		void write(std::vector<uint8_t>& packet) {}

	public:
		acio_icca_device(Python2Input* device)
		{
			p2dev = device;
		}

		bool device_write(std::vector<uint8_t>& packet, std::vector<uint8_t>& outputResponse);
	};
} // namespace usb_python2
#pragma once
