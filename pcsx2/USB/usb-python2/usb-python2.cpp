/*
 * QEMU USB HID devices
 *
 * Copyright (c) 2005 Fabrice Bellard
 * Copyright (c) 2007 OpenMoko, Inc.  (andrew@openedhand.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "PrecompiledHeader.h"
#include <algorithm>
#include <numeric>
#include <queue>

#include <wx/ffile.h>
#include <wx/fileconf.h>

#include "common/IniInterface.h"
#include "gui/AppConfig.h"

#include "USB/deviceproxy.h"
#include "USB/qemu-usb/desc.h"
#include "USB/shared/inifile_usb.h"

#include "usb-python2.h"
#include "python2proxy.h"

#include "USB/usb-python2/devices/ddr_extio.h"
#include "USB/usb-python2/devices/thrilldrive_belt.h"
#include "USB/usb-python2/devices/thrilldrive_handle.h"
#include "USB/usb-python2/devices/toysmarch_drumpad.h"
#include "USB/usb-python2/devices/icca.h"

#include "patches.h"

#ifdef PCSX2_DEVBUILD
#define Python2Con DevConWriterEnabled&& DevConWriter
#define Python2ConVerbose DevConWriterEnabled&& DevConWriter
#else
#define Python2Con DevConWriterEnabled&& DevConWriter
#define Python2ConVerbose ConsoleWriter_Null
#endif

#ifdef _MSC_VER
#define BigEndian16(in) _byteswap_ushort(in)
#else
#define BigEndian16(in) __builtin_bswap16(in)
#endif

namespace usb_python2
{
	constexpr USBDescStrings python2io_desc_strings = {
		"",
	};

	constexpr uint8_t python2_dev_desc[] = {
		0x12, /*  u8 bLength; */
		0x01, /*  u8 bDescriptorType; Device */
		WBVAL(0x101), /*  u16 bcdUSB; v1.01 */

		0x00, /*  u8  bDeviceClass; */
		0x00, /*  u8  bDeviceSubClass; */
		0x00, /*  u8  bDeviceProtocol; [ low/full speeds only ] */
		0x08, /*  u8  bMaxPacketSize0; 8 Bytes */

		WBVAL(0x0000), /*  u16 idVendor; */
		WBVAL(0x7305), /*  u16 idProduct; */
		WBVAL(0x0020), /*  u16 bcdDevice */

		0, /*  u8  iManufacturer; */
		0, /*  u8  iProduct; */
		0, /*  u8  iSerialNumber; */
		0x01 /*  u8  bNumConfigurations; */
	};

	constexpr uint8_t python2_config_desc[] = {
		USB_CONFIGURATION_DESC_SIZE, // bLength
		USB_CONFIGURATION_DESCRIPTOR_TYPE, // bDescriptorType (Configuration)
		WBVAL(40), // wTotalLength
		0x01, // bNumInterfaces 1
		0x01, // bConfigurationValue
		0x00, // iConfiguration (String Index)
		USB_CONFIG_POWERED_MASK, // bmAttributes
		USB_CONFIG_POWER_MA(100), // bMaxPower

		/* Interface Descriptor */
		USB_INTERFACE_DESC_SIZE, // bLength
		USB_INTERFACE_DESCRIPTOR_TYPE, // bDescriptorType
		0, // bInterfaceNumber
		0, // bAlternateSetting
		3, // bNumEndpoints
		USB_CLASS_RESERVED, // bInterfaceClass
		0, // bInterfaceSubClass
		0, // bInterfaceProtocol
		0, // iInterface (String Index)

		USB_ENDPOINT_DESC_SIZE, // bLength
		USB_ENDPOINT_DESCRIPTOR_TYPE, // bDescriptorType
		USB_ENDPOINT_IN(3), // bEndpointAddress
		USB_ENDPOINT_TYPE_INTERRUPT, // bmAttributes
		WBVAL(16), // wMaxPacketSize
		3, // bInterval

		USB_ENDPOINT_DESC_SIZE, // bLength
		USB_ENDPOINT_DESCRIPTOR_TYPE, // bDescriptorType
		USB_ENDPOINT_IN(1), // bEndpointAddress
		USB_ENDPOINT_TYPE_BULK, // bmAttributes
		WBVAL(64), // wMaxPacketSize
		10, // bInterval

		USB_ENDPOINT_DESC_SIZE, // bLength
		USB_ENDPOINT_DESCRIPTOR_TYPE, // bDescriptorType
		USB_ENDPOINT_OUT(2), // bEndpointAddress
		USB_ENDPOINT_TYPE_BULK, // bmAttributes
		WBVAL(64), // wMaxPacketSize
		10, // bInterval

		//0x34 // Junk data that's in the descriptor. Uncommenting this breaks things.
	};

	typedef struct UsbPython2State
	{
		USBDevice dev;
		USBDesc desc;
		USBDescDevice desc_dev;

		Python2Input* p2dev;

		std::unique_ptr<input_device> devices[2];

		// For Thrill Drive 3
		const static int32_t wheelCenter = 0x7fdf;

		std::vector<uint8_t> buf;

		struct freeze
		{
			int ep = 0;
			uint8_t port;

			int gameType = 0;
			uint32_t jammaIoStatus = 0xf0ffff80; // Default state of real hardware
			bool force31khz = false;

			uint32_t coinsInserted[2] = {0, 0};
			bool coinButtonHeld[2] = {false, false};

			char dipSwitch[4] = {'0', '0', '0', '0'};

			int requestedDongle = -1;
			bool isDongleSlotLoaded[2] = {false, false};
			uint8_t dongleSlotPayload[2][40] = {{0}, {0}};

			// For Thrill Drive 3
			int32_t wheel = wheelCenter;
			int32_t brake = 0;
			int32_t accel = 0;

			// For Guitar Freaks
			int32_t knobs[2] = {0, 0};
		} f;
	} UsbPython2State;

	void load_configuration(USBDevice* dev)
	{
		auto s = reinterpret_cast<UsbPython2State*>(dev);

		// Called when the device is initialized so just load settings here
		wxFileName iniPath = EmuFolders::Settings.Combine(wxString("Python2.ini"));
		std::unique_ptr<wxFileConfig> hini(OpenFileConfig(iniPath.GetFullPath()));
		IniLoader ini((wxConfigBase*)hini.get());

		TSTDSTRING selectedDevice;
		LoadSetting(Python2Device::TypeName(), s->f.port, APINAME, N_DEVICE, selectedDevice);

		{
			ScopedIniGroup cardReaderEntry(ini, L"CardReader");
			wxString cardFilenameP1 = wxEmptyString;
			ini.Entry(TEXT("Player1Card"), cardFilenameP1, wxEmptyString);

			wxString cardFilenameP2 = wxEmptyString;
			ini.Entry(TEXT("Player2Card"), cardFilenameP2, wxEmptyString);

			Console.WriteLn(TEXT("Player 1 card filename: %s"), WX_STR(cardFilenameP1));
			Console.WriteLn(TEXT("Player 2 card filename: %s"), WX_STR(cardFilenameP2));
		}

		const auto prevGameType = s->f.gameType;
		wxString groupName;
		long groupIdx = 0;
		auto foundGroup = hini->GetFirstGroup(groupName, groupIdx);
		while (foundGroup)
		{
			//Console.WriteLn(TEXT("Group: %s"), groupName);

			if (!groupName.Matches(selectedDevice))
			{
				foundGroup = hini->GetNextGroup(groupName, groupIdx);
				continue;
			}

			ini.SetPath(groupName);

			wxString tmp = wxEmptyString;

			ini.Entry(TEXT("Name"), tmp, wxEmptyString);
			Console.WriteLn(TEXT("Name: %s"), WX_STR(tmp));

			ini.Entry(TEXT("DongleBlackPath"), tmp, wxEmptyString);
			Console.WriteLn(TEXT("DongleBlackPath: %s"), WX_STR(tmp));
			if (!tmp.IsEmpty())
			{
				wxFFile fin(tmp, "rb");
				if (fin.IsOpened())
				{
					fin.Read(s->f.dongleSlotPayload[0], fin.Length() > 40 ? 40 : fin.Length());
					fin.Close();

					s->f.isDongleSlotLoaded[0] = true;
				}
				else
				{
					s->f.isDongleSlotLoaded[0] = false;
				}
			}

			ini.Entry(TEXT("DongleWhitePath"), tmp, wxEmptyString);
			Console.WriteLn(TEXT("DongleWhitePath: %s"), WX_STR(tmp));
			if (!tmp.IsEmpty())
			{
				wxFFile fin(tmp, "rb");
				if (fin.IsOpened())
				{
					fin.Read(s->f.dongleSlotPayload[1], fin.Length() > 40 ? 40 : fin.Length());
					fin.Close();

					s->f.isDongleSlotLoaded[1] = true;
				}
				else
				{
					s->f.isDongleSlotLoaded[1] = false;
				}
			}

			ini.Entry(TEXT("InputType"), tmp, wxEmptyString);
			Console.WriteLn(TEXT("InputType: %s"), WX_STR(tmp));
			if (!tmp.IsEmpty())
				s->f.gameType = atoi(tmp);
			else
				s->f.gameType = 0;

			ini.Entry(TEXT("DipSwitch"), tmp, wxEmptyString);
			Console.WriteLn(TEXT("DipSwitch: %s"), WX_STR(tmp));
			if (!tmp.IsEmpty())
			{
				for (size_t j = 0; j < 4 && j < tmp.size(); j++)
					s->f.dipSwitch[j] = tmp[j];
			}
			else
			{
				for (size_t j = 0; j < 4 && j < tmp.size(); j++)
					s->f.dipSwitch[j] = '0';
			}

			ini.Entry(TEXT("HddImagePath"), tmp, wxEmptyString);
			Console.WriteLn(TEXT("HddImagePath: %s"), WX_STR(tmp));
			if (!tmp.IsEmpty())
				HddImageOverridePath = tmp;
			else
				HddImageOverridePath.clear();

			ini.Entry(TEXT("HddIdPath"), tmp, wxEmptyString);
			Console.WriteLn(TEXT("HddIdPath: %s"), WX_STR(tmp));
			if (!tmp.IsEmpty())
				HddIdPath = tmp;
			else
				HddIdPath = L"";

			ini.Entry(TEXT("IlinkIdPath"), tmp, wxEmptyString);
			Console.WriteLn(TEXT("IlinkIdPath: %s"), WX_STR(tmp));
			if (!tmp.IsEmpty())
				IlinkIdPath = tmp;
			else
				IlinkIdPath.clear();

			ini.Entry(TEXT("GfdmFrameSizeFix"), tmp, wxEmptyString);
			Console.WriteLn(TEXT("GfdmFrameSizeFix: %s"), WX_STR(tmp));
			if (!tmp.IsEmpty())
				GfdmFrameSizeFixEnabled = tmp == "1";
			else
				GfdmFrameSizeFixEnabled = false;

			ini.Entry(TEXT("Force31kHz"), tmp, wxEmptyString);
			Console.WriteLn(TEXT("Force31kHz: %s"), WX_STR(tmp));
			if (!tmp.IsEmpty())
				s->f.force31khz = tmp == "1";
			else
				s->f.force31khz = false;

			ini.Entry(TEXT("PatchFile"), tmp, wxEmptyString);
			Console.WriteLn(TEXT("PatchFile: %s"), WX_STR(tmp));
			if (!tmp.IsEmpty())
				PatchFileOverridePath = tmp;
			else
				PatchFileOverridePath.clear();

			break;
		}

		// It seems like the device is recreated from scratch every time the config dialog is closed so this isn't all that helpful after all
		if (s->f.gameType != prevGameType && s->devices[0] != nullptr)
			s->devices[0].reset();

		if (s->f.gameType != prevGameType && s->devices[1] != nullptr)
			s->devices[1].reset();

		if (s->f.gameType == GAMETYPE_DM)
		{
			if (s->devices[0] == nullptr)
			{
				auto aciodev = std::make_unique<acio_device>();
				aciodev->add_acio_device(1, std::make_unique<acio_icca_device>(s->p2dev));
				s->devices[0] = std::move(aciodev);
			}
		}
		else if (s->f.gameType == GAMETYPE_GF)
		{
			if (s->devices[0] == nullptr)
			{
				auto aciodev = std::make_unique<acio_device>();
				aciodev->add_acio_device(1, std::make_unique<acio_icca_device>(s->p2dev));
				aciodev->add_acio_device(2, std::make_unique<acio_icca_device>(s->p2dev));
				s->devices[0] = std::move(aciodev);
			}
		}
		else if (s->f.gameType == GAMETYPE_DDR)
		{
			if (s->devices[0] == nullptr)
				s->devices[0] = std::make_unique<extio_device>();

			if (s->devices[1] == nullptr)
			{
				auto aciodev = std::make_unique<acio_device>();
				aciodev->add_acio_device(1, std::make_unique<acio_icca_device>(s->p2dev));
				aciodev->add_acio_device(2, std::make_unique<acio_icca_device>(s->p2dev));
				s->devices[1] = std::move(aciodev);
			}
		}
		else if (s->f.gameType == GAMETYPE_THRILLDRIVE)
		{
			if (s->devices[1] == nullptr)
			{
				auto aciodev = std::make_unique<acio_device>();
				aciodev->add_acio_device(1, std::make_unique<thrilldrive_handle_device>(s->p2dev));
				aciodev->add_acio_device(2, std::make_unique<thrilldrive_belt_device>(s->p2dev));
				s->devices[1] = std::move(aciodev);
			}
		}
		else if (s->f.gameType == GAMETYPE_TOYSMARCH)
		{
			if (s->devices[0] == nullptr)
				s->devices[0] = std::make_unique<toysmarch_drumpad_device>(s->p2dev);
		}
	}

	static void p2io_cmd_handler(USBDevice* dev, USBPacket* p, std::vector<uint8_t> &data)
	{
		auto s = reinterpret_cast<UsbPython2State*>(dev);

		// Remove any garbage from beginning of buffer if it exists
		for (size_t i = 0; i < s->buf.size(); i++)
		{
			if (s->buf[i] == P2IO_HEADER_MAGIC)
			{
				if (i != 0)
					s->buf.erase(s->buf.begin(), s->buf.begin() + i);

				break;
			}
		}

		if (s->buf.size() < sizeof(P2IO_PACKET_HEADER))
			return;

		const P2IO_PACKET_HEADER* header = (P2IO_PACKET_HEADER*)s->buf.data();
		const size_t totalPacketLen = header->len + 2; // header byte + sequence byte

		if (s->buf.size() >= totalPacketLen && header->magic == P2IO_HEADER_MAGIC)
		{
			data.push_back(header->seqNo);
			data.push_back(P2IO_STATUS_OK); // Status

			if (header->cmd == P2IO_CMD_GET_VERSION)
			{
				Python2Con.WriteLn("p2io: P2IO_CMD_GET_VERSION");

				// Get version returns D44:1.6.4
				const uint8_t resp[] = {
					'D', '4', '4', '\0', // product code
					1, // major
					6,  // minor
					4 // revision
				};
				data.insert(data.end(), std::begin(resp), std::end(resp));
			}
			else if (header->cmd == P2IO_CMD_SET_AV_MASK)
			{
				Python2Con.WriteLn("p2io: P2IO_CMD_SET_AV_MASK %02x", s->buf[4]);
				data.push_back(0);
			}
			else if (header->cmd == P2IO_CMD_GET_AV_REPORT)
			{
				Python2Con.WriteLn("p2io: P2IO_CMD_GET_AV_REPORT");
				data.push_back(s->f.force31khz ? P2IO_AVREPORT_MODE_31KHZ : P2IO_AVREPORT_MODE_15KHZ);
			}
			else if (header->cmd == P2IO_CMD_DALLAS)
			{
				const auto subCmd = s->buf[4];
				Python2Con.WriteLn("p2io: P2IO_CMD_DALLAS_CMD %02x", subCmd);

				if (subCmd == 0 || subCmd == 1 || subCmd == 2)
				{
					// Dallas Read SID/Mem
					if (subCmd != 2)
						s->f.requestedDongle = subCmd;

					if (s->f.requestedDongle >= 0)
					{
						data.push_back(s->f.isDongleSlotLoaded[s->f.requestedDongle]);
						data.insert(data.end(), std::begin(s->f.dongleSlotPayload[s->f.requestedDongle]), std::end(s->f.dongleSlotPayload[s->f.requestedDongle]));
					}
					else
					{
						data.push_back(0);
						data.insert(data.end(), s->buf.begin() + 5, s->buf.begin() + 5 + 40); // Return received data in buffer
					}
				}
				else if (subCmd == 3)
				{
					// TODO: is this ever used?
					// Dallas Write Mem
					data.push_back(0);
					data.insert(data.end(), s->buf.begin() + 5, s->buf.begin() + 5 + 40); // Return received data in buffer
				}
			}
			else if (header->cmd == P2IO_CMD_READ_DIPSWITCH)
			{
				Python2ConVerbose.WriteLn("P2IO_CMD_READ_DIPSWITCH %02x\n", s->buf[4]);

				uint8_t val = 0;
				for (size_t i = 0; i < 4; i++)
					val |= (1 << (3 - i)) * (s->f.dipSwitch[i] == '1');

				data.push_back(val & 0x7f); // 0xff is ignored
			}

			else if (header->cmd == P2IO_CMD_COIN_STOCK)
			{
				Python2Con.WriteLn("P2IO_CMD_COIN_STOCK");

				const uint8_t resp[] = {
					0, // If this is non-zero then the following 4 bytes are not processed
					uint8_t((s->f.coinsInserted[0] >> 8)), uint8_t(s->f.coinsInserted[0]),
					uint8_t((s->f.coinsInserted[1] >> 8)), uint8_t(s->f.coinsInserted[1]),
				};
				data.insert(data.end(), std::begin(resp), std::end(resp));
			}
			else if (header->cmd == P2IO_CMD_COIN_COUNTER && (s->buf[4] & 0x10) == 0x10)
			{
				// p2sub_coin_counter_merge exists which calls "32 10" and "32 11"
				Python2Con.WriteLn("p2io: P2IO_CMD_COIN_COUNTER_MERGE %02x %02x", s->buf[4], s->buf[5]);
				data.push_back(0);
			}
			else if (header->cmd == P2IO_CMD_COIN_COUNTER)
			{
				// p2sub_coin_counter accepts parameters with a range of 0x00 to 0x0f
				Python2Con.WriteLn("p2io: P2IO_CMD_COIN_COUNTER %02x %02x", s->buf[4], s->buf[5]);
				data.push_back(0);
			}
			else if (header->cmd == P2IO_CMD_COIN_BLOCKER)
			{
				// Param 1 seems to either be 0/1
				Python2Con.WriteLn("p2io: P2IO_CMD_COIN_BLOCKER %02x %02x", s->buf[4], s->buf[5]);
				data.push_back(0);
			}
			else if (header->cmd == P2IO_CMD_COIN_COUNTER_OUT)
			{
				// Param 1 seems to either be 0/1
				Python2Con.WriteLn("p2io: P2IO_CMD_COIN_COUNTER_OUT %02x %02x", s->buf[4], s->buf[5]);
				data.push_back(0);
			}

			else if (header->cmd == P2IO_CMD_LAMP_OUT && s->buf[4] == 0xff)
			{
				//Python2Con.WriteLn("p2io: P2IO_CMD_LAMP_OUT_ALL %08x", *(int*)&s->buf[5]);
				data.push_back(0);
			}
			else if (header->cmd == P2IO_CMD_LAMP_OUT)
			{
				/*
				* DDR:
				* 00 73 1P HALOGEN UP
				* 00 b3 1P HALOGEN DOWN
				* 00 d3 2P HALOGEN UP
				* 00 e3 2P HALOGEN DOWN
				* 00 f2 1P BUTTON
				* 00 f1 2P BUTTON
				*/

				//Python2Con.WriteLn("p2io: P2IO_CMD_LAMP_OUT %02x", s->buf[5]);
				data.push_back(0);
			}

			else if (header->cmd == P2IO_CMD_PORT_READ)
			{
				// TODO: What port?
				Python2Con.WriteLn("p2io: P2IO_CMD_PORT_READ %02x", s->buf[4]);
				data.push_back(0);
			}
			else if (header->cmd == P2IO_CMD_PORT_READ_POR)
			{
				// TODO: What port?
				Python2Con.WriteLn("p2io: P2IO_CMD_PORT_READ_POR %02x", s->buf[4]);
				data.push_back(0);
			}
			else if (header->cmd == P2IO_CMD_SEND_IR)
			{
				Python2Con.WriteLn("p2io: P2IO_CMD_SEND_IR %02x", s->buf[4]);
				data.push_back(0);
			}
			else if (header->cmd == P2IO_CMD_SET_WATCHDOG)
			{
				Python2Con.WriteLn("p2io: P2IO_CMD_SET_WATCHDOG %02x", s->buf[4]);
				data.push_back(0);
			}
			else if (header->cmd == P2IO_CMD_JAMMA_START)
			{
				Python2Con.WriteLn("p2io: P2IO_CMD_JAMMA_START");
				data.push_back(0); // 0 = succeeded, anything else = fail
			}
			else if (header->cmd == P2IO_CMD_GET_JAMMA_POR)
			{
				Python2Con.WriteLn("p2io: P2IO_CMD_GET_JAMMA_POR");
				const uint8_t resp[] = {0, 0, 0, 0}; // Real hardware returns 0x00ff 0x00ff?
				data.insert(data.end(), std::begin(resp), std::end(resp));
			}
			else if (header->cmd == P2IO_CMD_FWRITEMODE && s->buf[4] == 0xaa)
			{
				// WARNING: FIRMWARE WRITE MODE! Be careful with testing this on real hardware!
				// The device will restart/disconnect(?) if this is called and lights seem to go out.
				// In the P2IO driver, usbboot_init is called after this command so it must be reconnected to be usable again.
				// If p2sub_setmode is called with a value of 0x20 then the P2IO will go into firmware write mode. A value of 2 calls inits the device with P2IO_CMD_JAMMA_START.
				Python2Con.WriteLn("p2io: P2IO_CMD_FWRITEMODE");
				data.push_back(0); // 0 = succeeded, anything else = fail
			}

			else if (header->cmd == P2IO_CMD_SCI_SETUP)
			{
				const auto port = s->buf[4];
				const auto cmd = s->buf[5];
				const auto param = s->buf[6];

				Python2Con.WriteLn("p2io: P2IO_CMD_SCI_OPEN %02x %02x %02x", port, cmd, param);
				data.push_back(0);

				const auto device = s->devices[port].get();
				if (device != nullptr)
				{
					if (cmd == 0)
						device->open();
					else if (header->cmd == 0xff)
						device->close();
				}
			}
			else if (header->cmd == P2IO_CMD_SCI_WRITE)
			{
				const auto port = s->buf[4];
				const auto packetLen = s->buf[5];

				#ifdef PCSX2_DEVBUILD
				Python2Con.WriteLn("p2io: P2IO_CMD_SCI_WRITE: ");
				for (auto i = 0; i < s->buf.size(); i++)
				{
					printf("%02x ", s->buf[i]);
				}
				printf("\n");
				#endif

				const auto device = s->devices[port].get();
				if (device != nullptr)
				{
					const auto startIdx = s->buf.begin() + 6;
					auto escapedPacket = acio_unescape_packet(std::vector<uint8_t>(startIdx, startIdx + packetLen));
					device->write(escapedPacket);
				}

				data.push_back(packetLen);
			}
			else if (header->cmd == P2IO_CMD_SCI_READ)
			{
				const auto port = s->buf[4];
				const auto requestedLen = s->buf[5];

				//Python2Con.WriteLn("P2IO_CMD_SCI_READ %02x %02x\n", port, requestedLen);

				const auto packetLenOffset = data.size();
				data.push_back(0);

				const auto device = s->devices[port].get();
				auto readLen = 0;
				if (device != nullptr && requestedLen > 0)
				{
					readLen = device->read(data, requestedLen);
				}

				data[packetLenOffset] = readLen;
			}

			else
			{
				#ifdef PCSX2_DEVBUILD
				printf("usb_python2_handle_data %02x\n", s->buf.size());
				for (auto i = 0; i < s->buf.size(); i++)
				{
					printf("%02x ", s->buf[i]);
				}
				printf("\n");
				#endif
			}

			data.insert(data.begin(), data.size());
			data = acio_escape_packet(data);
			data.insert(data.begin(), P2IO_HEADER_MAGIC);

			s->buf.erase(s->buf.begin(), s->buf.begin() + totalPacketLen);
		}
	}

	static int jammaUpdateCounter = 0;
	static void usb_python2_handle_data(USBDevice* dev, USBPacket* p)
	{
		auto s = reinterpret_cast<UsbPython2State*>(dev);

		switch (p->pid)
		{
			case USB_TOKEN_IN:
			{
				std::vector<uint8_t> data;

				if (p->ep->nr == 3) // JAMMA IO pipe
				{
					if (s->p2dev->isPassthrough())
					{
						s->p2dev->ReadIo(data);
					}
					else
					{
						// Real hardware seems to have the analog values non-0 when no I/O is attached.
						// Here is the results of 20 consecutive reads with no I/O attached.
						// The values are roughly the same but there's some small movement between reads. Possibly just noise.
						// 10 40 10 f0 10 80 10 c0
						// 10 40 11 00 10 80 10 d0
						// 10 50 11 00 10 80 10 c0
						// 10 40 11 00 10 80 10 d0
						// 10 40 11 00 10 70 10 c0
						// 10 40 11 00 10 80 10 c0
						// 10 40 11 00 10 80 10 e0
						// 10 40 11 00 10 80 10 d0
						// 10 40 11 00 10 80 10 d0
						// 10 40 11 00 10 80 10 e0
						// 10 40 11 00 10 80 10 c0
						// 10 40 11 00 10 90 10 c0
						// 10 30 11 00 10 80 10 c0
						// 10 20 11 00 10 90 10 d0
						// 10 10 11 00 10 90 10 c0
						// 10 10 11 00 10 90 10 d0
						// 10 10 11 00 10 90 10 c0
						// 10 00 11 00 10 80 10 e0
						// 10 00 11 00 10 90 10 d0
						// 10 20 11 00 10 90 10 c0
						uint8_t resp[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
						uint32_t* jammaIo = reinterpret_cast<uint32_t*>(&resp[0]);
						uint16_t* analogIo = reinterpret_cast<uint16_t*>(&resp[4]);

						s->f.jammaIoStatus ^= 2; // Watchdog, if this bit isn't flipped every update then a security error will be raised

#define CheckKeyState(key, val) \
	{ \
		if (s->p2dev->GetKeyState((key))) \
			s->f.jammaIoStatus &= ~(val); \
		else \
			s->f.jammaIoStatus |= (val); \
	}

#define CheckKeyStateOneShot(key, val) \
	{ \
		if (s->p2dev->GetKeyStateOneShot((key))) \
			s->f.jammaIoStatus &= ~(val); \
		else \
			s->f.jammaIoStatus |= (val); \
	}

#define KnobStateInc(key, val, playerId) \
	{ \
		if (s->p2dev->GetKeyState((key))) \
			s->f.knobs[(playerId)] = (s->f.knobs[(playerId)] + 1) % 4; \
	}


#define KnobStateDec(key, val, playerId) \
	{ \
		if (s->p2dev->GetKeyState((key))) \
			s->f.knobs[(playerId)] = (s->f.knobs[(playerId)] - 1) < 0 ? 3 : (s->f.knobs[(playerId)] - 1); \
	}
						// Hold the state for a certain amount of updates so the game can register quick changes.
						// Setting this value too low will result in very fast key changes being dropped.
						// Setting this value too high or there will be latency with key presses.
						if (jammaUpdateCounter >= 8)
						{
							CheckKeyState(TEXT("Test"), P2IO_JAMMA_IO_TEST);
							CheckKeyState(TEXT("Service"), P2IO_JAMMA_IO_SERVICE);
							CheckKeyState(TEXT("Coin1"), P2IO_JAMMA_IO_COIN1);
							CheckKeyState(TEXT("Coin2"), P2IO_JAMMA_IO_COIN2);

							// Python 2 games only accept coins via the P2IO directly, even though the game sees the JAMMA coin buttons returned here(?)
							if (!(s->f.jammaIoStatus & P2IO_JAMMA_IO_COIN1))
							{
								if (!s->f.coinButtonHeld[0])
								{
									s->f.coinsInserted[0]++;
									s->f.coinButtonHeld[0] = true;
								}
							}
							else
							{
								s->f.coinButtonHeld[0] = false;
							}

							if (!(s->f.jammaIoStatus & P2IO_JAMMA_IO_COIN2))
							{
								if (!s->f.coinButtonHeld[1])
								{
									s->f.coinsInserted[1]++;
									s->f.coinButtonHeld[1] = true;
								}
							}
							else
							{
								s->f.coinButtonHeld[1] = false;
							}

							if (s->f.gameType == GAMETYPE_DM)
							{
								CheckKeyState(TEXT("DmSelectL"), P2IO_JAMMA_DM_SELECT_L);
								CheckKeyState(TEXT("DmSelectR"), P2IO_JAMMA_DM_SELECT_R);
								CheckKeyState(TEXT("DmStart"), P2IO_JAMMA_DM_START);
								CheckKeyStateOneShot(TEXT("DmHihat"), P2IO_JAMMA_DM_HIHAT);
								CheckKeyStateOneShot(TEXT("DmSnare"), P2IO_JAMMA_DM_SNARE);
								CheckKeyStateOneShot(TEXT("DmBassDrum"), P2IO_JAMMA_DM_BASS_DRUM);
								CheckKeyStateOneShot(TEXT("DmHighTom"), P2IO_JAMMA_DM_HIGH_TOM);
								CheckKeyStateOneShot(TEXT("DmLowTom"), P2IO_JAMMA_DM_LOW_TOM);
								CheckKeyStateOneShot(TEXT("DmCymbal"), P2IO_JAMMA_DM_CYMBAL);
							}
							else if (s->f.gameType == GAMETYPE_GF)
							{
								CheckKeyState(TEXT("GfP1Start"), P2IO_JAMMA_GF_P1_START);
								CheckKeyState(TEXT("GfP1NeckR"), P2IO_JAMMA_GF_P1_R);
								CheckKeyState(TEXT("GfP1NeckG"), P2IO_JAMMA_GF_P1_G);
								CheckKeyState(TEXT("GfP1NeckB"), P2IO_JAMMA_GF_P1_B);
								CheckKeyState(TEXT("GfP1Pick"), P2IO_JAMMA_GF_P1_PICK);
								CheckKeyState(TEXT("GfP1Wail"), P2IO_JAMMA_GF_P1_WAILING);
								KnobStateInc(TEXT("GfP1EffectInc"), P2IO_JAMMA_GF_P1_EFFECT1, 0);
								KnobStateDec(TEXT("GfP1EffectDec"), P2IO_JAMMA_GF_P1_EFFECT2, 0);

								CheckKeyState(TEXT("GfP2Start"), P2IO_JAMMA_GF_P2_START);
								CheckKeyState(TEXT("GfP2NeckR"), P2IO_JAMMA_GF_P2_R);
								CheckKeyState(TEXT("GfP2NeckG"), P2IO_JAMMA_GF_P2_G);
								CheckKeyState(TEXT("GfP2NeckB"), P2IO_JAMMA_GF_P2_B);
								CheckKeyStateOneShot(TEXT("GfP2Pick"), P2IO_JAMMA_GF_P2_PICK);
								CheckKeyState(TEXT("GfP2Wail"), P2IO_JAMMA_GF_P2_WAILING);
								KnobStateInc(TEXT("GfP2EffectInc"), P2IO_JAMMA_GF_P2_EFFECT1, 1);
								KnobStateDec(TEXT("GfP2EffectDec"), P2IO_JAMMA_GF_P2_EFFECT2, 1);

								s->f.jammaIoStatus |= P2IO_JAMMA_GF_P1_EFFECT3;
								if (s->f.knobs[0] == 1)
									s->f.jammaIoStatus &= ~P2IO_JAMMA_GF_P1_EFFECT1;
								else if (s->f.knobs[0] == 2)
									s->f.jammaIoStatus &= ~P2IO_JAMMA_GF_P1_EFFECT2;
								else if (s->f.knobs[0] == 3)
									s->f.jammaIoStatus &= ~P2IO_JAMMA_GF_P1_EFFECT3;

								s->f.jammaIoStatus |= P2IO_JAMMA_GF_P2_EFFECT3;
								if (s->f.knobs[1] == 1)
									s->f.jammaIoStatus &= ~P2IO_JAMMA_GF_P2_EFFECT1;
								else if (s->f.knobs[1] == 2)
									s->f.jammaIoStatus &= ~P2IO_JAMMA_GF_P2_EFFECT2;
								else if (s->f.knobs[1] == 3)
									s->f.jammaIoStatus &= ~P2IO_JAMMA_GF_P2_EFFECT3;
							}
							else if (s->f.gameType == GAMETYPE_DDR)
							{
								CheckKeyState(TEXT("DdrP1Start"), P2IO_JAMMA_DDR_P1_START);
								CheckKeyState(TEXT("DdrP1SelectL"), P2IO_JAMMA_DDR_P1_LEFT);
								CheckKeyState(TEXT("DdrP1SelectR"), P2IO_JAMMA_DDR_P1_RIGHT);
								CheckKeyState(TEXT("DdrP1FootLeft"), P2IO_JAMMA_DDR_P1_FOOT_LEFT);
								CheckKeyState(TEXT("DdrP1FootDown"), P2IO_JAMMA_DDR_P1_FOOT_DOWN);
								CheckKeyState(TEXT("DdrP1FootUp"), P2IO_JAMMA_DDR_P1_FOOT_UP);
								CheckKeyState(TEXT("DdrP1FootRight"), P2IO_JAMMA_DDR_P1_FOOT_RIGHT);

								CheckKeyState(TEXT("DdrP2Start"), P2IO_JAMMA_DDR_P2_START);
								CheckKeyState(TEXT("DdrP2SelectL"), P2IO_JAMMA_DDR_P2_LEFT);
								CheckKeyState(TEXT("DdrP2SelectR"), P2IO_JAMMA_DDR_P2_RIGHT);
								CheckKeyState(TEXT("DdrP2FootLeft"), P2IO_JAMMA_DDR_P2_FOOT_LEFT);
								CheckKeyState(TEXT("DdrP2FootDown"), P2IO_JAMMA_DDR_P2_FOOT_DOWN);
								CheckKeyState(TEXT("DdrP2FootUp"), P2IO_JAMMA_DDR_P2_FOOT_UP);
								CheckKeyState(TEXT("DdrP2FootRight"), P2IO_JAMMA_DDR_P2_FOOT_RIGHT);
							}
							else if (s->f.gameType == GAMETYPE_THRILLDRIVE)
							{
								CheckKeyState(TEXT("ThrillDriveStart"), P2IO_JAMMA_THRILLDRIVE_START);

								CheckKeyState(TEXT("ThrillDriveGearUp"), P2IO_JAMMA_THRILLDRIVE_GEARSHIFT_UP);
								CheckKeyState(TEXT("ThrillDriveGearDown"), P2IO_JAMMA_THRILLDRIVE_GEARSHIFT_DOWN);
							}
							else if (s->f.gameType == GAMETYPE_TOYSMARCH)
							{
								CheckKeyState(TEXT("ToysMarchP1Start"), P2IO_JAMMA_TOYSMARCH_P1_START);
								CheckKeyState(TEXT("ToysMarchP1SelectL"), P2IO_JAMMA_TOYSMARCH_P1_LEFT);
								CheckKeyState(TEXT("ToysMarchP1SelectR"), P2IO_JAMMA_TOYSMARCH_P1_RIGHT);
								CheckKeyState(TEXT("ToysMarchP2Start"), P2IO_JAMMA_TOYSMARCH_P2_START);
								CheckKeyState(TEXT("ToysMarchP2SelectL"), P2IO_JAMMA_TOYSMARCH_P2_LEFT);
								CheckKeyState(TEXT("ToysMarchP2SelectR"), P2IO_JAMMA_TOYSMARCH_P2_RIGHT);
							}

							jammaUpdateCounter = 0;
						}
						else
						{
							jammaUpdateCounter++;
						}

						if (s->f.gameType == GAMETYPE_THRILLDRIVE)
						{
							const auto isBrakePressed = s->p2dev->GetKeyState(TEXT("ThrillDriveBrake"));
							if (isBrakePressed)
								s->f.brake = 0xffff;
							else
								s->f.brake = s->p2dev->GetKeyStateAnalog(TEXT("ThrillDriveBrakeAnalog"));

							const auto isAccelerationPressed = s->p2dev->GetKeyState(TEXT("ThrillDriveAccel"));
							if (isAccelerationPressed)
							{
								if (!isBrakePressed)
									s->f.accel = 0xffff;
							}
							else
								s->f.accel = s->p2dev->GetKeyStateAnalog(TEXT("ThrillDriveAccelAnalog"));

							const auto isLeftWheelTurned = s->p2dev->GetKeyState(TEXT("ThrillDriveWheelLeft"));
							const auto isRightWheelTurned = s->p2dev->GetKeyState(TEXT("ThrillDriveWheelRight"));
							if (isLeftWheelTurned)
								s->f.wheel = 0xffff;
							else if (isRightWheelTurned)
								s->f.wheel = 0;
							else if (s->p2dev->IsAnalogKeybindAvailable(TEXT("ThrillDriveWheelAnalog")))
								s->f.wheel = uint16_t(0xffff - (0xffff * s->p2dev->GetKeyStateAnalog(TEXT("ThrillDriveWheelAnalog"))));
							else
								s->f.wheel = s->wheelCenter;

							analogIo[0] = BigEndian16(s->f.wheel);
							analogIo[1] = BigEndian16(s->f.accel);
							analogIo[2] = BigEndian16(s->f.brake);
						}

						jammaIo[0] = s->f.jammaIoStatus;

						data.insert(data.end(), std::begin(resp), std::end(resp));
					}
				}
				else if (p->ep->nr == 1) // P2IO output pipe
				{
					if (s->p2dev->isPassthrough())
					{
						// Send to real Python 2 I/O device for passthrough
						s->p2dev->ReadPacket(data);
					}
					else
					{
						p2io_cmd_handler(dev, p, data);
					}
				}

				if (data.size() <= 0)
					data.push_back(0);

				if (data.size() > 0)
					usb_packet_copy(p, data.data(), data.size());
				else
					p->status = USB_RET_NAK;

				break;
			}

			case USB_TOKEN_OUT:
			{
				if (p->ep->nr == 2) // P2IO input pipe
				{
					const auto len = usb_packet_size(p);
					auto buf = std::vector<uint8_t>(len);
					usb_packet_copy(p, buf.data(), buf.size());

					if (s->p2dev->isPassthrough())
					{
						// Send to real Python 2 I/O device for passthrough
						s->p2dev->WritePacket(buf);
					}
					else
					{
						buf = acio_unescape_packet(buf);
						s->buf.insert(s->buf.end(), buf.begin(), buf.end());
					}
				}
				break;
			}

			default:
				p->status = USB_RET_STALL;
				break;
		}
	}

	static void usb_python2_handle_control(USBDevice* dev, USBPacket* p,
		int request, int value, int index, int length, uint8_t* data)
	{
		const int ret = usb_desc_handle_control(dev, p, request, value, index, length, data);
		if (ret >= 0)
			return;

		DevCon.WriteLn("usb-python2: Unimplemented handle control request! %04x\n", request);
		p->status = USB_RET_STALL;
	}

	static void usb_python2_unrealize(USBDevice* dev)
	{
		auto s = reinterpret_cast<UsbPython2State*>(dev);
		if (s)
			delete s;
	}

	int usb_python2_open(USBDevice* dev)
	{
		auto s = reinterpret_cast<UsbPython2State*>(dev);

		if (s)
		{
			// Load the configuration and start SPDIF patcher thread every time a game is started
			load_configuration(dev);

			if (!mPatchSpdifAudioThreadIsRunning)
			{
				if (mPatchSpdifAudioThread.joinable())
					mPatchSpdifAudioThread.join();
				mPatchSpdifAudioThread = std::thread(Python2Patch::PatchSpdifAudioThread, s->p2dev);
			}

			return s->p2dev->Open();
		}

		return 0;
	}

	void usb_python2_close(USBDevice* dev)
	{
		auto s = reinterpret_cast<UsbPython2State*>(dev);
		if (s)
			s->p2dev->Close();
	}

	USBDevice* Python2Device::CreateDevice(int port)
	{
		DevCon.WriteLn("%s\n", __func__);

		std::string varApi;
#ifdef _WIN32
		std::wstring tmp;
		LoadSetting(nullptr, port, TypeName(), N_DEVICE_API, tmp);
		varApi = wstr_to_str(tmp);
#else
		LoadSetting(nullptr, port, TypeName(), N_DEVICE_API, varApi);
#endif
		const UsbPython2ProxyBase* proxy = RegisterUsbPython2::instance().Proxy(varApi);
		if (!proxy)
		{
			Console.WriteLn("Invalid API: %s\n", varApi.c_str());
			return nullptr;
		}

		Python2Input* p2dev = proxy->CreateObject(port, TypeName());

		if (!p2dev)
			return nullptr;

		auto s = new UsbPython2State();
		s->desc.full = &s->desc_dev;
		s->desc.str = &python2io_desc_strings[0];

		int ret = usb_desc_parse_dev(&python2_dev_desc[0], sizeof(python2_dev_desc), s->desc, s->desc_dev);
		if (ret >= 0)
			ret = usb_desc_parse_config(&python2_config_desc[0], sizeof(python2_config_desc), s->desc_dev);

		if (ret < 0)
		{
			usb_python2_unrealize((USBDevice*)s);
			return nullptr;
		}

		s->dev.speed = USB_SPEED_FULL;
		s->dev.klass.handle_attach = usb_desc_attach;
		s->dev.klass.handle_control = usb_python2_handle_control;
		s->dev.klass.handle_data = usb_python2_handle_data;
		s->dev.klass.unrealize = usb_python2_unrealize;
		s->dev.klass.open = usb_python2_open;
		s->dev.klass.close = usb_python2_close;
		s->dev.klass.usb_desc = &s->desc;
		s->dev.klass.product_desc = "";

		s->p2dev = p2dev;
		s->f.port = port;
		s->f.gameType = -1;
		s->devices[0] = nullptr;
		s->devices[1] = nullptr;

		usb_desc_init(&s->dev);
		usb_ep_init(&s->dev);

		return (USBDevice*)s;
	}

	int Python2Device::Configure(int port, const std::string& api, void* data)
	{
		auto proxy = RegisterUsbPython2::instance().Proxy(api);
		if (proxy)
			return proxy->Configure(port, TypeName(), data);
		return RESULT_CANCELED;
	}

	int Python2Device::Freeze(FreezeAction mode, USBDevice* dev, void* data)
	{
		auto s = reinterpret_cast<UsbPython2State*>(dev);
		if (!s)
			return 0;

		auto freezed = reinterpret_cast<UsbPython2State::freeze*>(data);
		switch (mode)
		{
			case FreezeAction::Load:
				if (!s)
					return -1;
				s->f = *freezed;
				return sizeof(UsbPython2State::freeze);
			case FreezeAction::Save:
				if (!s)
					return -1;
				*freezed = s->f;
				return sizeof(UsbPython2State::freeze);
			case FreezeAction::Size:
				return sizeof(UsbPython2State::freeze);
			default:
				break;
		}
		return 0;
	}

	std::list<std::string> Python2Device::ListAPIs()
	{
		return RegisterUsbPython2::instance().Names();
	}

	const TCHAR* Python2Device::LongAPIName(const std::string& name)
	{
		auto proxy = RegisterUsbPython2::instance().Proxy(name);
		if (proxy)
			return proxy->Name();
		return nullptr;
	}

} // namespace usb_python2
