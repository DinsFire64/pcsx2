#include "input_device.h"

namespace usb_python2
{
	class extio_device : public input_device
	{
		void write(std::vector<uint8_t>& packet);
	};
} // namespace usb_python2
#pragma once