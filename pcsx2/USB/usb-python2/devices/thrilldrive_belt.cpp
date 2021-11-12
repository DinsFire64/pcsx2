#include "thrilldrive_belt.h"

namespace usb_python2
{
	bool thrilldrive_belt_device::device_write(std::vector<uint8_t>& packet, std::vector<uint8_t>& outputResponse)
	{
		const auto header = (ACIO_PACKET_HEADER*)packet.data();
		const auto code = BigEndian16(header->code);

		if (p2dev->GetKeyState("ThrillDriveSeatbelt") != 0)
		{
			if (!seatBeltButtonPressed)
				seatBeltStatus = !seatBeltStatus;

			seatBeltButtonPressed = true;
		}
		else
		{
			seatBeltButtonPressed = false;
		}

		std::vector<uint8_t> response;
		bool isEmptyResponse = false;
		if (code == 0x0102)
		{
			// Seems to be a command for feedback or something relating to the motor
			response.push_back(0);
		}
		else if (code == 0x0113)
		{
			uint8_t resp[8] = {0};
			resp[2] = seatBeltStatus == true ? 0 : 0xff; // 0 = fastened
			response.insert(response.end(), std::begin(resp), std::end(resp));
		}
		else
		{
			// Just return 0 for anything else
			response.push_back(0);
		}

		if (response.size() > 0 || isEmptyResponse)
		{
			outputResponse.insert(outputResponse.end(), response.begin(), response.end());
			return true;
		}

		return false;
	}
} // namespace usb_python2
