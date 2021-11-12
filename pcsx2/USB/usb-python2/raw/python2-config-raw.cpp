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

#include <stdio.h>

#include <vector>
#include <ghc/filesystem.h>

#include <wx/wx.h>
#include <wx/collpane.h>
#include <wx/filepicker.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/gbsizer.h>
#include <wx/listctrl.h>

#include "gui/AppCoreThread.h"

#include "gui/AppConfig.h"

#include "usb-python2-raw.h"

#include <commctrl.h>
#include "usb-python2-raw.h"
#include "python2-config-raw-res.h"
#include "PAD/Windows/VKey.h"

#include <windowsx.h>

#define MSG_PRESS_ESC(wnd) SendDlgItemMessageW(wnd, IDC_STATIC_CAP, WM_SETTEXT, 0, (LPARAM)L"Capturing, press ESC to cancel")

namespace usb_python2
{
	namespace raw
	{
		uint32_t axisDiff[32]; //previous axes values
		bool axisPass2 = false;
		uint32_t uniqueKeybindIdx = 0;

		wxString getKeyLabel(const KeyMapping key)
		{
			if (key.bindType == 0)
				return wxString::Format("Button %d", key.value);
			else if (key.bindType == 1)
				return wxString::Format("%s Axis", axisLabelList[key.value]);
			else if (key.bindType == 2)
				return wxString::Format("Hat %d", key.value);
			else if (key.bindType == 3)
				return wxString::Format("%s", GetVKStringW(key.value));

			return wxEmptyString;
		}

		inline bool MapExists(const MapVector& maps, const wxString hid)
		{
			for (auto& it : maps)
				if (!it.hidPath.compare(hid))
					return true;
			return false;
		}

		void LoadMappings(const char* dev_type, MapVector& maps)
		{
			maps.clear();

			uniqueKeybindIdx = 0;

			for (int j = 0; j < 25; j++)
			{
				TSTDSTRING hid;

				if (LoadSetting(dev_type, j, "RAW DEVICE", "HID", hid) && !hid.empty() && !MapExists(maps, hid))
				{
					Mappings m;
					memset(&m, 0, sizeof(Mappings));
					maps.push_back(m);
					Mappings& ptr = maps.back();

					ptr.hidPath = hid;
					ptr.devName = hid;

					for (uint32_t targetBind = 0; targetBind < buttonLabelList.size(); targetBind++)
					{
						TSTDSTRING _tmp;
						if (LoadSetting(dev_type, j, "RAW DEVICE", buttonLabelList[targetBind], _tmp))
						{
							wxString tmp = _tmp;

							while (tmp.size() > 0)
							{
								auto idx = tmp.find_first_of(L',');

								if (idx == wxString::npos)
									idx = tmp.size();

								auto substr = wxString(tmp.begin(), tmp.begin() + idx);

								uint32_t buttonType = 0;
								uint32_t value = 0;
								int isOneshot = 0;
								wxSscanf(substr, "%02X|%08X|%d", &buttonType, &value, &isOneshot);

								KeyMapping keybindMapping = {
									uniqueKeybindIdx++,
									targetBind,
									buttonType,
									value,
									isOneshot};
								ptr.mappings.push_back(keybindMapping);

								tmp.erase(tmp.begin(), tmp.begin() + idx + 1);
							}
						}
					}
				}
			}
		}

		void SaveMappings(const char* dev_type, MapVector& maps)
		{
			uint32_t numDevice = 0;

			for (int j = 0; j < 25; j++)
			{
				RemoveSection(dev_type, j, "RAW DEVICE");
			}

			for (auto& it : maps)
			{
				if (it.mappings.size() > 0)
				{
					SaveSetting(dev_type, numDevice, "RAW DEVICE", "HID", it.hidPath.ToStdWstring());

					std::map<int, std::vector<KeyMapping>> mapByKeybind;

					for (auto& bindIt : it.mappings)
						mapByKeybind[bindIt.keybindId].push_back(bindIt);

					for (auto& bindIt : mapByKeybind)
					{
						wxString val;

						for (auto& keymap : bindIt.second)
						{
							auto tmp = wxString::Format("%02X|%08X|%d", keymap.bindType, keymap.value, keymap.isOneshot);

							if (val.size() > 0)
								val.append(L",");

							val.append(tmp);
						}

						SaveSetting(dev_type, numDevice, "RAW DEVICE", buttonLabelList[bindIt.first], val.ToStdWstring());
					}
				}

				numDevice++;
			}
		}

		class Python2RawConfigDialog : public wxDialog
		{
			wxChoice* gameListChoice;
			std::vector<wxString> gameList;
			wxListView* keybindListBox;
			wxListView* keybindListBox2;
			bool captureButton = false;

			const int ID_BIND_BUTTON = 5000;
			const int ID_UNBIND_BUTTON = 5001;
			const int ID_ONESHOT_BUTTON = 5002;

		public:
			Python2RawConfigDialog(std::vector<wxString> gameList)
				: wxDialog(nullptr, wxID_ANY, _("Python 2 Configuration"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER)
				, gameList(gameList)
			{
				#ifdef _WIN32
				InitHid();

				auto hW = GetHWND();
				RAWINPUTDEVICE rid[3];
				rid[0].usUsagePage = 0x01;
				rid[0].usUsage = 0x05;
				rid[0].dwFlags = hW ? RIDEV_INPUTSINK : RIDEV_REMOVE; // adds game pad
				rid[0].hwndTarget = hW;

				rid[1].usUsagePage = 0x01;
				rid[1].usUsage = 0x04;
				rid[1].dwFlags = hW ? RIDEV_INPUTSINK : RIDEV_REMOVE; // adds joystick
				rid[1].hwndTarget = hW;

				rid[2].usUsagePage = 0x01;
				rid[2].usUsage = 0x06;
				rid[2].dwFlags = hW ? RIDEV_INPUTSINK /*| RIDEV_NOLEGACY*/ : RIDEV_REMOVE; // adds HID keyboard and also ignores legacy keyboard messages
				rid[2].hwndTarget = hW;

				RegisterRawInputDevices(rid, 3, sizeof(rid[0]));
				#endif

				auto* padding = new wxBoxSizer(wxVERTICAL);
				auto* topBox = new wxBoxSizer(wxVERTICAL);

				wxArrayString gameListArray;
				for (auto& game : gameList)
					gameListArray.Add(game);

				auto* gameListBox = new wxGridBagSizer(18, 18);
				gameListChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, gameListArray);

				auto* gameListLabel = new wxStaticText(this, wxID_ANY, _("Game Entry:"));
				gameListBox->Add(gameListLabel, wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_RIGHT | wxALIGN_CENTRE_VERTICAL);
				gameListBox->Add(gameListChoice, wxGBPosition(0, 1), wxGBSpan(0, 8), wxEXPAND);
				gameListBox->AddGrowableCol(1);

				keybindListBox = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
				keybindListBox->AppendColumn("Keybinds");
				keybindListBox->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);

				keybindListBox2 = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);
				keybindListBox2->AppendColumn("Keybind");
				keybindListBox2->SetColumnWidth(0, 150);
				keybindListBox2->AppendColumn("Button");
				keybindListBox2->SetColumnWidth(1, 100);
				keybindListBox2->AppendColumn("Oneshot");
				keybindListBox2->SetColumnWidth(2, 75);
				keybindListBox2->AppendColumn("Device");

				gameListBox->Add(keybindListBox, wxGBPosition(1, 0), wxGBSpan(12, 6), wxEXPAND);
				gameListBox->Add(keybindListBox2, wxGBPosition(1, 6), wxGBSpan(12, 12), wxEXPAND);
				topBox->Add(gameListBox, wxSizerFlags().Expand());

				auto* buttonBox = new wxBoxSizer(wxHORIZONTAL);
				auto bindButton = new wxButton(this, ID_BIND_BUTTON, "Bind Key", wxDefaultPosition, wxSize(100, 25), 0);
				auto oneshotToggleButton = new wxButton(this, ID_ONESHOT_BUTTON, "Toggle Oneshot", wxDefaultPosition, wxSize(100, 25), 0);
				auto unbindButton = new wxButton(this, ID_UNBIND_BUTTON, "Unbind Key", wxDefaultPosition, wxSize(100, 25), 0);
				buttonBox->Add(bindButton, wxSizerFlags().Left());
				buttonBox->AddStretchSpacer();
				buttonBox->Add(oneshotToggleButton, wxSizerFlags().Right());
				buttonBox->Add(unbindButton, wxSizerFlags().Right());

				topBox->AddSpacer(4);
				topBox->Add(buttonBox, wxSizerFlags().Expand());
				topBox->AddSpacer(16);
				topBox->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), wxSizerFlags().Right());

				padding->Add(topBox, wxSizerFlags().Expand().Border(wxALL, 5));
				SetSizerAndFit(padding);

				SetMaxSize(wxSize(wxDefaultCoord, GetMinSize().y));

				Bind(wxEVT_BUTTON, &Python2RawConfigDialog::OnBindButton, this, ID_BIND_BUTTON);
				Bind(wxEVT_BUTTON, &Python2RawConfigDialog::OnUnbindButton, this, ID_UNBIND_BUTTON);
				Bind(wxEVT_BUTTON, &Python2RawConfigDialog::OnOneshotButton, this, ID_ONESHOT_BUTTON);
			}

			#ifdef _WIN32
			WXLRESULT MSWWindowProc(WXUINT uMsg, WXWPARAM wParam, WXLPARAM lParam)
			{
				switch (uMsg)
				{
					case WM_INPUT:
						auto ret = S_FALSE;

						if (captureButton)
						{
							ret = 0;

							UINT bufferSize;
							GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &bufferSize, sizeof(RAWINPUTHEADER));

							auto pRawInput = (PRAWINPUT)malloc(bufferSize);
							if (!pRawInput)
								break;

							if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, pRawInput, &bufferSize, sizeof(RAWINPUTHEADER)) > 0)
								ParseRawInput(pRawInput);

							free(pRawInput);

							if (!captureButton)
								ResetState();
						}

						return ret;
				}

				return wxDialog::MSWWindowProc(uMsg, wParam, lParam);
			}

			void ParseRawInput(PRAWINPUT pRawInput)
			{
				PHIDP_PREPARSED_DATA pPreparsedData = NULL;
				HIDP_CAPS Caps;
				PHIDP_BUTTON_CAPS pButtonCaps = NULL;
				PHIDP_VALUE_CAPS pValueCaps = NULL;
				USHORT capsLength;
				UINT bufferSize;
				USAGE usage[32];
				ULONG usageLength;
				char name[1024] = {0};
				UINT nameSize = 1024;
				UINT pSize;
				RID_DEVICE_INFO devInfo;
				std::string devName;
				Mappings* mapping = NULL;
				char buf[256];

				//
				// Get the preparsed data block
				//
				GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_DEVICENAME, name, &nameSize);
				pSize = sizeof(devInfo);
				GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_DEVICEINFO, &devInfo, &pSize);

				if (devInfo.dwType == RIM_TYPEKEYBOARD)
				{
					devName = "Keyboard";
				}
				else
				{
					CHECK(GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_PREPARSEDDATA, NULL, &bufferSize) == 0);
					CHECK(pPreparsedData = (PHIDP_PREPARSED_DATA)malloc(bufferSize));
					CHECK((int)GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_PREPARSEDDATA, pPreparsedData, &bufferSize) >= 0);
					CHECK(HidP_GetCaps(pPreparsedData, &Caps) == HIDP_STATUS_SUCCESS);

					devName = name;
					std::transform(devName.begin(), devName.end(), devName.begin(), ::toupper);
				}

				for (auto& it : mapVector)
				{
					if (it.hidPath == devName)
					{
						mapping = &(it);
						break;
					}
				}

				if (mapping == NULL)
				{
					Mappings m; // = new Mappings;
					ZeroMemory(&m, sizeof(Mappings));
					mapVector.push_back(m);
					mapping = &mapVector.back();
					mapping->devName = devName;
					mapping->hidPath = devName;
				}
				//TODO get real dev name, probably from registry (see PAD Windows)
				if (!mapping->devName.length())
					mapping->devName = devName;

				const auto targetBind = keybindListBox->GetFirstSelected();
				if (targetBind == -1)
				{
					// Nothing is actually selected, bail early
					captureButton = false;
					return;
				}

				if (devInfo.dwType == RIM_TYPEKEYBOARD &&
					(pRawInput->data.keyboard.Flags & RI_KEY_BREAK) != RI_KEY_BREAK)
				{
					if (pRawInput->data.keyboard.VKey == 0xff)
						return; //TODO
					if (pRawInput->data.keyboard.VKey == VK_ESCAPE)
					{
						//resetState(hW);
						return;
					}

					/*
					swprintf_s(buf, "Captured KB button %d", pRawInput->data.keyboard.VKey);
					SendDlgItemMessage(dgHwnd2, IDC_STATIC_CAP, WM_SETTEXT, 0, (LPARAM)buf);
					*/

					KeyMapping keybindMapping = {
						uniqueKeybindIdx++,
						targetBind,
						KeybindType_Keyboard,
						pRawInput->data.keyboard.VKey};
					mapping->mappings.push_back(keybindMapping);

					captureButton = false;
				}
				else
				{
					// Button caps
					CHECK(pButtonCaps = (PHIDP_BUTTON_CAPS)malloc(sizeof(HIDP_BUTTON_CAPS) * Caps.NumberInputButtonCaps));

					capsLength = Caps.NumberInputButtonCaps;
					CHECK(HidP_GetButtonCaps(HidP_Input, pButtonCaps, &capsLength, pPreparsedData) == HIDP_STATUS_SUCCESS);
					int numberOfButtons = pButtonCaps->Range.UsageMax - pButtonCaps->Range.UsageMin + 1;

					usageLength = numberOfButtons;
					CHECK(
						HidP_GetUsages(
							HidP_Input, pButtonCaps->UsagePage, 0, usage, &usageLength, pPreparsedData,
							(PCHAR)pRawInput->data.hid.bRawData, pRawInput->data.hid.dwSizeHid) == HIDP_STATUS_SUCCESS);

					if (usageLength > 0) //Using first button only though
					{
						/*
						swprintf_s(buf, "Captured HID button %d", usage[0]);
						SendDlgItemMessage(dgHwnd2, IDC_STATIC_CAP, WM_SETTEXT, 0, (LPARAM)buf);
						*/

						KeyMapping keybindMapping = {
							uniqueKeybindIdx++,
							targetBind,
							KeybindType_Button,
							usage[0]};
						mapping->mappings.push_back(keybindMapping);

						captureButton = false;
					}
					else
					{
						ULONG value = 0;

						// Value caps
						CHECK(pValueCaps = (PHIDP_VALUE_CAPS)malloc(sizeof(HIDP_VALUE_CAPS) * Caps.NumberInputValueCaps));
						capsLength = Caps.NumberInputValueCaps;
						CHECK(HidP_GetValueCaps(HidP_Input, pValueCaps, &capsLength, pPreparsedData) == HIDP_STATUS_SUCCESS);

						for (auto i = 0; i < Caps.NumberInputValueCaps; i++)
						{
							CHECK(
								HidP_GetUsageValue(
									HidP_Input, pValueCaps[i].UsagePage, 0, pValueCaps[i].Range.UsageMin, &value, pPreparsedData,
									(PCHAR)pRawInput->data.hid.bRawData, pRawInput->data.hid.dwSizeHid) == HIDP_STATUS_SUCCESS);

							uint32_t logical = pValueCaps[i].LogicalMax - pValueCaps[i].LogicalMin;

							switch (pValueCaps[i].Range.UsageMin)
							{
								case HID_USAGE_GENERIC_X:
								case HID_USAGE_GENERIC_Y:
								case HID_USAGE_GENERIC_Z:
								case HID_USAGE_GENERIC_RX:
								case HID_USAGE_GENERIC_RY:
								case HID_USAGE_GENERIC_RZ:
								{
									auto axis = pValueCaps[i].Range.UsageMin - HID_USAGE_GENERIC_X;
									if (axisPass2)
									{
										if ((uint32_t)abs((int)(axisDiff[axis] - value)) > (logical >> 2))
										{
											axisPass2 = false;

											/*
											swprintf_s(buf, "Captured axis %d", axis);
											SendDlgItemMessage(dgHwnd2, IDC_STATIC_CAP, WM_SETTEXT, 0, (LPARAM)buf);
											*/

											KeyMapping keybindMapping = {
												uniqueKeybindIdx++,
												targetBind,
												KeybindType_Axis,
												axis};
											mapping->mappings.push_back(keybindMapping);
											captureButton = false;

											goto Error;
										}
										break;
									}
									axisDiff[axis] = value;
									break;
								}

								case 0x39: // Hat Switch
									if (value < 0x8)
									{
										axisPass2 = false;

										/*
										swprintf_s(buf, "Captured hat switch %d", value);
										SendDlgItemMessage(dgHwnd2, IDC_STATIC_CAP, WM_SETTEXT, 0, (LPARAM)buf);
										*/

										KeyMapping keybindMapping = {
											uniqueKeybindIdx++,
											targetBind,
											KeybindType_Hat,
											value};
										mapping->mappings.push_back(keybindMapping);
										captureButton = false;

										goto Error;
									}
									break;
							}
						}

						axisPass2 = true;
					}
				}

			Error:
				SAFE_FREE(pPreparsedData);
				SAFE_FREE(pButtonCaps);
				SAFE_FREE(pValueCaps);
			}
			#endif

			void OnBindButton(wxCommandEvent& ev)
			{
				printf("OnBindButton pressed!\n");
				captureButton = true;
			}

			void OnUnbindButton(wxCommandEvent& ev)
			{
				printf("OnUnbindButton pressed!\n");

				auto selectedIdx = keybindListBox2->GetFirstSelected();
				while (selectedIdx != -1)
				{
					auto itemData = keybindListBox2->GetItemData(selectedIdx);
					const size_t mapIdx = (itemData >> 16) & 0xffff;
					const auto uniqueId = itemData & 0xffff;

					if (mapIdx < mapVector.size())
					{
						mapVector[mapIdx].mappings.erase(
							std::remove_if(
								mapVector[mapIdx].mappings.begin(),
								mapVector[mapIdx].mappings.end(),
								[uniqueId](KeyMapping& v) { return v.uniqueId == uniqueId; }),
							mapVector[mapIdx].mappings.end());
					}

					selectedIdx = keybindListBox2->GetNextSelected(selectedIdx);
				}

				ResetState();
			}

			void OnOneshotButton(wxCommandEvent& ev)
			{
				printf("OnOneshotButton pressed!\n");

				std::vector<long> selectedItemList;

				auto selectedIdx = keybindListBox2->GetFirstSelected();
				while (selectedIdx != -1)
				{
					auto itemData = keybindListBox2->GetItemData(selectedIdx);
					const size_t mapIdx = (itemData >> 16) & 0xffff;
					const auto uniqueId = itemData & 0xffff;

					if (mapIdx < mapVector.size())
					{
						for (auto& it : mapVector[mapIdx].mappings)
						{
							if (it.uniqueId == uniqueId)
							{
								it.isOneshot = !it.isOneshot;
							}
						}
					}

					selectedItemList.push_back(selectedIdx);

					selectedIdx = keybindListBox2->GetNextSelected(selectedIdx);
				}

				ResetState();

				for (auto& idx : selectedItemList)
					keybindListBox2->Select(idx, true);
				keybindListBox2->SetFocus();
			}

			void ResetState()
			{
				captureButton = false;
				PopulateMappings();
			}

			void PopulateMappings()
			{
				keybindListBox2->DeleteAllItems();

				auto itemIdx = 0;
				for (size_t mapIdx = 0; mapIdx < mapVector.size(); mapIdx++)
				{
					for (auto& bindIt : mapVector[mapIdx].mappings)
					{
						auto lastItemIdx = keybindListBox2->InsertItem(itemIdx++, buttonLabelList[bindIt.keybindId]);
						keybindListBox2->SetItem(lastItemIdx, 1, getKeyLabel(bindIt));
						keybindListBox2->SetItem(lastItemIdx, 2, bindIt.isOneshot ? "On" : "Off");
						keybindListBox2->SetItem(lastItemIdx, 3, mapVector[mapIdx].devName);
						keybindListBox2->SetItemData(lastItemIdx, (mapIdx << 16) | bindIt.uniqueId);
					}
				}
			}

			void Load(Python2DlgConfig& config, wxString selectedDevice)
			{
				for (size_t i = 0; i != config.devList.size(); i++)
				{
					if (config.devListGroups[i] == selectedDevice)
					{
						gameListChoice->SetSelection(i);
						break;
					}
				}

				LoadMappings(config.dev_type, mapVector);

				for (size_t i = 0; i < buttonLabelList.size(); i++)
				{
					keybindListBox->InsertItem(i, buttonLabelList[i]);
				}

				PopulateMappings();
			}
		};

		void ConfigurePython2DlgProc(Python2DlgConfig& config)
		{
			ScopedCoreThreadPause paused_core;

			TSTDSTRING selectedDevice;
			LoadSetting(Python2Device::TypeName(), config.port, "python2", N_DEVICE, selectedDevice);

			Python2RawConfigDialog dialog(config.devList);
			dialog.Load(config, selectedDevice);

			if (dialog.ShowModal() == wxID_OK)
			{
			}

			paused_core.AllowResume();
		}
	} // namespace raw
} // namespace usb_python2
