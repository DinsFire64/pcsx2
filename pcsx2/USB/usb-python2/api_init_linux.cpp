#include "python2proxy.h"
#include "noop.h"

void usb_python2::RegisterUsbPython2::Register()
{
	auto& inst = RegisterUsbPython2::instance();
	inst.Add(usb_python2::noop::APINAME, new UsbPython2Proxy<usb_python2::noop::NOOP>());
}