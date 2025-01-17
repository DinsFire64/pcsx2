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

#include "PrecompiledHeader.h"
#include "deviceproxy.h"
#include "usb-pad/padproxy.h"
#include "usb-mic/audiodeviceproxy.h"
#include "usb-hid/hidproxy.h"
#include "usb-eyetoy/videodeviceproxy.h"
#include "usb-python2/python2proxy.h"

RegisterDevice* RegisterDevice::registerDevice = nullptr;

void RegisterAPIs()
{
	usb_pad::RegisterPad::Register();
	usb_mic::RegisterAudioDevice::Register();
	usb_hid::RegisterUsbHID::Register();
	usb_eyetoy::RegisterVideoDevice::Register();
	usb_python2::RegisterUsbPython2::Register();
}

void UnregisterAPIs()
{
	usb_pad::RegisterPad::instance().Clear();
	usb_mic::RegisterAudioDevice::instance().Clear();
	usb_hid::RegisterUsbHID::instance().Clear();
	usb_eyetoy::RegisterVideoDevice::instance().Clear();
	usb_python2::RegisterUsbPython2::instance().Clear();
}
