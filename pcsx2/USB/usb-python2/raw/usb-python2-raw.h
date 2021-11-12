/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2020  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef USBPYTHON2RAW_H
#define USBPYTHON2RAW_H
#include <thread>
#include <array>
#include <atomic>
#include "USB/usb-python2/python2proxy.h"
#include "USB/usb-python2/usb-python2.h"
#include "USB/shared/rawinput_usb.h"
#include "USB/readerwriterqueue/readerwriterqueue.h"

namespace usb_python2
{
#define CHECK(exp) \
	do \
	{ \
		if (!(exp)) \
			goto Error; \
	} while (0)
#define SAFE_FREE(p) \
	do \
	{ \
		if (p) \
		{ \
			free(p); \
			(p) = NULL; \
		} \
	} while (0)

	namespace raw
	{
		struct InputStateUpdate
		{
			wxDateTime timestamp;
			bool state;
		};

		const std::vector<TCHAR*> axisLabelList = {
			TEXT("X"),
			TEXT("Y"),
			TEXT("Z"),
			TEXT("RX"),
			TEXT("RY"),
			TEXT("RZ")};

		const std::vector<TCHAR*> buttonLabelList = {
			// Machine
			TEXT("Test"),
			TEXT("Service"),
			TEXT("Coin1"),
			TEXT("Coin2"),

			// Guitar Freaks
			TEXT("GfP1Start"),
			TEXT("GfP1Pick"),
			TEXT("GfP1Wail"),
			TEXT("GfP1EffectInc"),
			TEXT("GfP1EffectDec"),
			TEXT("GfP1NeckR"),
			TEXT("GfP1NeckG"),
			TEXT("GfP1NeckB"),

			TEXT("GfP2Start"),
			TEXT("GfP2Pick"),
			TEXT("GfP2Wail"),
			TEXT("GfP2EffectInc"),
			TEXT("GfP2EffectDec"),
			TEXT("GfP2NeckR"),
			TEXT("GfP2NeckG"),
			TEXT("GfP2NeckB"),

			// Drummania
			TEXT("DmStart"),
			TEXT("DmSelectL"),
			TEXT("DmSelectR"),
			TEXT("DmHihat"),
			TEXT("DmSnare"),
			TEXT("DmHighTom"),
			TEXT("DmLowTom"),
			TEXT("DmCymbal"),
			TEXT("DmBassDrum"),

			// DDR
			TEXT("DdrP1Start"),
			TEXT("DdrP1SelectL"),
			TEXT("DdrP1SelectR"),
			TEXT("DdrP1FootLeft"),
			TEXT("DdrP1FootDown"),
			TEXT("DdrP1FootUp"),
			TEXT("DdrP1FootRight"),

			TEXT("DdrP2Start"),
			TEXT("DdrP2SelectL"),
			TEXT("DdrP2SelectR"),
			TEXT("DdrP2FootLeft"),
			TEXT("Ddr2FootDown"),
			TEXT("Ddr2FootUp"),
			TEXT("DdrP2FootRight"),

			// Thrill Drive
			TEXT("ThrillDriveStart"),
			TEXT("ThrillDriveGearUp"),
			TEXT("ThrillDriveGearDown"),
			TEXT("ThrillDriveWheelAnalog"),
			TEXT("ThrillDriveWheelLeft"),
			TEXT("ThrillDriveWheelRight"),
			TEXT("ThrillDriveAccelAnalog"),
			TEXT("ThrillDriveAccel"),
			TEXT("ThrillDriveBrake"),
			TEXT("ThrillDriveBrakeAnalog"),
			TEXT("ThrillDriveSeatbelt"),

			// Toy's March
			TEXT("ToysMarchP1Start"),
			TEXT("ToysMarchP1SelectL"),
			TEXT("ToysMarchP1SelectR"),
			TEXT("ToysMarchP1DrumL"),
			TEXT("ToysMarchP1DrumR"),
			TEXT("ToysMarchP1Cymbal"),

			TEXT("ToysMarchP2Start"),
			TEXT("ToysMarchP2SelectL"),
			TEXT("ToysMarchP2SelectR"),
			TEXT("ToysMarchP2DrumL"),
			TEXT("ToysMarchP2DrumR"),
			TEXT("ToysMarchP2Cymbal"),

			// ICCA Card Reader
			TEXT("KeypadP1_0"),
			TEXT("KeypadP1_1"),
			TEXT("KeypadP1_2"),
			TEXT("KeypadP1_3"),
			TEXT("KeypadP1_4"),
			TEXT("KeypadP1_5"),
			TEXT("KeypadP1_6"),
			TEXT("KeypadP1_7"),
			TEXT("KeypadP1_8"),
			TEXT("KeypadP1_9"),
			TEXT("KeypadP1_00"),
			TEXT("KeypadP1InsertEject"),
			TEXT("KeypadP2_0"),
			TEXT("KeypadP2_1"),
			TEXT("KeypadP2_2"),
			TEXT("KeypadP2_3"),
			TEXT("KeypadP2_4"),
			TEXT("KeypadP2_5"),
			TEXT("KeypadP2_6"),
			TEXT("KeypadP2_7"),
			TEXT("KeypadP2_8"),
			TEXT("KeypadP2_9"),
			TEXT("KeypadP2_00"),
			TEXT("KeypadP2InsertEject"),
		};

		class RawInputPad : public Python2Input, shared::rawinput::ParseRawInputCB
		{
		public:
			RawInputPad(int port, const char* dev_type)
				: Python2Input(port, dev_type)
			{
				if (!InitHid())
					throw UsbPython2Error("InitHid() failed!");
			}
			~RawInputPad()
			{
				Close();
			}
			int Open() override;
			int Close() override;
			int ReadPacket(std::vector<uint8_t>& data) override { return 0; };
			int WritePacket(const std::vector<uint8_t>& data) override { return 0; };
			void ReadIo(std::vector<uint8_t>& data) override {}
			int Reset() override { return 0; }
			void ParseRawInput(PRAWINPUT pRawInput);

			bool isPassthrough() override { return false; }

			static const TCHAR* Name()
			{
				return TEXT("Raw Input");
			}

			void UpdateKeyStates(TSTDSTRING keybind) override;
			bool GetKeyState(TSTDSTRING keybind) override;
			bool GetKeyStateOneShot(TSTDSTRING keybind) override;
			double GetKeyStateAnalog(TSTDSTRING keybind) override;
			bool IsKeybindAvailable(TSTDSTRING keybind);
			bool IsAnalogKeybindAvailable(TSTDSTRING keybind);

			static int Configure(int port, const char* dev_type, void* data);

		protected:
		private:
		};

		enum KeybindType
		{
			KeybindType_Button = 0,
			KeybindType_Axis,
			KeybindType_Hat,
			KeybindType_Keyboard
		};

		struct KeyMapping
		{
			uint32_t uniqueId;
			uint32_t keybindId;
			uint32_t bindType; // 0 = button, 1 = axis, 2 = hat, 3 = keyboard
			uint32_t value;
			bool isOneshot; // Immediately trigger an off after on
		};

		struct Mappings
		{
			std::vector<KeyMapping> mappings;

			wxString devName;
#if _WIN32
			wxString hidPath;
#endif
		};

		struct Python2DlgConfig
		{
			int port;
			const char* dev_type;

			const std::vector<wxString> devList;
			const std::vector<wxString> devListGroups;

			Python2DlgConfig(int p, const char* dev_type_, const std::vector<wxString>& devList, const std::vector<wxString>& devListGroups)
				: port(p)
				, dev_type(dev_type_)
				, devList(devList)
				, devListGroups(devListGroups)
			{
			}
		};

		typedef std::vector<Mappings> MapVector;
		static MapVector mapVector;
		static std::map<HANDLE, Mappings*> mappings;

		static std::map<TSTDSTRING, std::deque<InputStateUpdate>> keyStateUpdates;
		static std::map<TSTDSTRING, bool> isOneshotState;
		static std::map<TSTDSTRING, bool> currentKeyStates;
		static std::map<TSTDSTRING, int> currentInputStateKeyboard;
		static std::map<TSTDSTRING, int> currentInputStatePad;
		static std::map<TSTDSTRING, double> currentInputStateAnalog;

		static std::map<USHORT, bool> keyboardButtonIsPressed;
		static std::map<TSTDSTRING, std::map<uint32_t, bool>> gamepadButtonIsPressed;

		void LoadMappings(const char* dev_type, MapVector& maps);
		void SaveMappings(const char* dev_type, MapVector& maps);

	} // namespace raw
} // namespace usb_python2
#endif
