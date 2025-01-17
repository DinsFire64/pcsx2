#include "PrecompiledHeader.h"
#include "USB/USB.h"
#include "usb-python2-passthrough.h"

#include <wx/fileconf.h>
#include "common/IniInterface.h"
#include "gui/AppConfig.h"

#include <algorithm>
#include <chrono>

constexpr int USB_ENDPOINT_INTERRUPT = (LIBUSB_ENDPOINT_IN | 3);
constexpr int USB_ENDPOINT_IN = (LIBUSB_ENDPOINT_IN | 1);
constexpr int USB_ENDPOINT_OUT = (LIBUSB_ENDPOINT_OUT | 2);
constexpr int USB_TIMEOUT = 100;

namespace usb_python2
{
	namespace passthrough
	{
		void PassthroughInput::InterruptReaderThread(void* ptr)
		{
			PassthroughInput* dev = static_cast<PassthroughInput*>(ptr);
			dev->isInterruptReaderThreadRunning = true;

			while (dev->handle != NULL)
				libusb_interrupt_transfer(dev->handle, USB_ENDPOINT_INTERRUPT, dev->ioData, sizeof(dev->ioData), NULL, 0);

			dev->isInterruptReaderThreadRunning = false;
		}

		int PassthroughInput::ReadPacket(std::vector<uint8_t>& data)
		{
			if (handle == NULL)
				return 0;

			uint8_t receiveBuf[64] = {0};
			int nread = 0;

			auto ret = libusb_bulk_transfer(handle, USB_ENDPOINT_IN, receiveBuf, sizeof(receiveBuf), &nread, 0);
			if (ret)
				return -1;

			//printf("Received %d bytes from device\n", nread);
			data.insert(data.end(), std::begin(receiveBuf), std::begin(receiveBuf) + nread);

			return nread;
		}

		int PassthroughInput::WritePacket(const std::vector<uint8_t>& data)
		{
			if (handle == NULL)
				return 0;

			return libusb_bulk_transfer(handle, USB_ENDPOINT_OUT, (unsigned char*)data.data(), data.size(), NULL, USB_TIMEOUT);
		}

		void PassthroughInput::ReadIo(std::vector<uint8_t> &data)
		{
			data.insert(data.end(), std::begin(ioData), std::begin(ioData) + sizeof(ioData));
		}

		int PassthroughInput::Open()
		{
			libusb_init(&ctx);

			handle = libusb_open_device_with_vid_pid(ctx, 0x0000, 0x7305);
			if (!handle) {
				printf("Could not open P2IO!\n");
				return 1;
			}

			const auto ret = libusb_claim_interface(handle, 0);
			if (ret < 0) {
				printf("Could not claim P2IO interface!\n");
				return 2;
			}

			printf("Opened P2IO device for passthrough!\n");

			const uint8_t defaultIoState[] = {0x80, 0xff, 0xff, 0xf0, 0, 0, 0, 0, 0, 0, 0, 0};
			memcpy(ioData, defaultIoState, sizeof(defaultIoState));

			if (!isInterruptReaderThreadRunning)
			{
				if (interruptThread.joinable())
					interruptThread.join();
				interruptThread = std::thread(PassthroughInput::InterruptReaderThread, this);
			}

			return 0;
		}

		int PassthroughInput::Close()
		{
			if (handle != NULL)
				libusb_close(handle);

			handle = NULL;

			return 0;
		}

		void ConfigurePython2Passthrough(Python2DlgConfig& config);
		int PassthroughInput::Configure(int port, const char* dev_type, void* data)
		{
			std::vector<wxString> devList;
			std::vector<wxString> devListGroups;

			wxFileName iniPath = EmuFolders::Settings.Combine(wxString("Python2.ini"));
			if (iniPath.FileExists())
			{
				std::unique_ptr<wxFileConfig> hini(OpenFileConfig(iniPath.GetFullPath()));
				IniLoader ini((wxConfigBase*)hini.get());

				wxString groupName;
				long groupIdx = 0;
				auto foundGroup = hini->GetFirstGroup(groupName, groupIdx);
				while (foundGroup)
				{
					if (groupName.StartsWith(L"GameEntry"))
						devListGroups.push_back(groupName);

					foundGroup = hini->GetNextGroup(groupName, groupIdx);
				}

				for (auto& groupName : devListGroups)
				{
					ScopedIniGroup groupEntry(ini, groupName);

					wxString tmp = wxEmptyString;
					ini.Entry(L"Name", tmp, wxEmptyString);
					if (tmp.empty())
						continue;

					devList.push_back(tmp);
				}

				Python2DlgConfig config(port, dev_type, devList, devListGroups);
				ConfigurePython2Passthrough(config);
			}

			return 0;
		}

	} // namespace passthrough
} // namespace usb_python2
