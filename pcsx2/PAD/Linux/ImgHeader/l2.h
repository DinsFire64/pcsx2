#pragma once

#include "Pcsx2Types.h"
#include <wx/gdicmn.h>

class res_l2
{
public:
	static const uint Length = 1096;
	static const u8 Data[Length];
	static wxBitmapType GetFormat() { return wxBITMAP_TYPE_PNG; }
};

const u8 res_l2::Data[Length] =
{
		0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,0x00,
		0x00,0x00,0x43,0x00,0x00,0x00,0x1c,0x08,0x06,0x00,0x00,0x00,0x3a,0x12,0x02,0x51,0x00,
		0x00,0x00,0x06,0x62,0x4b,0x47,0x44,0x00,0xff,0x00,0xff,0x00,0xff,0xa0,0xbd,0xa7,0x93,
		0x00,0x00,0x00,0x09,0x70,0x48,0x59,0x73,0x00,0x00,0x0b,0x13,0x00,0x00,0x0b,0x13,0x01,
		0x00,0x9a,0x9c,0x18,0x00,0x00,0x00,0x07,0x74,0x49,0x4d,0x45,0x07,0xdf,0x0b,0x1c,0x10,
		0x32,0x0b,0x24,0x27,0xf3,0xe5,0x00,0x00,0x03,0xd5,0x49,0x44,0x41,0x54,0x58,0xc3,0xe5,
		0x59,0x5b,0x6f,0x1b,0x55,0x10,0xfe,0xe6,0x9c,0xcd,0xda,0x35,0xf5,0xc6,0xa9,0xaf,0x69,
		0xad,0x8d,0x28,0x6e,0x68,0x82,0xeb,0x14,0x42,0x29,0x94,0x97,0x00,0x12,0x02,0xf1,0x10,
		0x10,0x08,0x04,0xa2,0x4f,0x3c,0xf1,0x2b,0x30,0xbf,0x80,0xbf,0x80,0x54,0x24,0x2a,0xc1,
		0x03,0x91,0x40,0x20,0x84,0xc2,0x13,0x20,0x0a,0x2d,0x2a,0xad,0xed,0x34,0x76,0x85,0x9c,
		0x26,0xa1,0x49,0x63,0xa7,0x69,0x1c,0xdb,0xb9,0xec,0x0e,0x0f,0x8e,0x1d,0x5a,0x5f,0xe2,
		0xda,0x59,0x27,0x69,0x46,0xb2,0xbc,0xbe,0xcd,0xcc,0x7e,0x67,0xbe,0xf3,0x9d,0x19,0x13,
		0x2c,0xb0,0xd1,0xd1,0x51,0x2e,0x5f,0x8f,0x8d,0x8d,0x11,0x00,0xcc,0x3e,0xae,0xf2,0x82,
		0xed,0x31,0x08,0x29,0x41,0x44,0x10,0x42,0x00,0x20,0x90,0x20,0x00,0x00,0x9b,0x66,0xe9,
		0x99,0x19,0xa6,0x69,0x56,0x1e,0xa7,0x6f,0xdc,0x25,0x74,0xc8,0xc8,0x4a,0x20,0xca,0xf6,
		0xcd,0x5f,0x63,0x96,0x24,0x9f,0xe8,0xf6,0x61,0xf0,0xda,0xfc,0x8e,0xdd,0x83,0x68,0xd7,
		0xc1,0x5c,0xc8,0xc1,0xc9,0x67,0x82,0xcc,0x3a,0xb8,0x16,0x10,0x00,0xf0,0xe6,0xd3,0xa3,
		0x96,0x80,0x31,0xb0,0x34,0x0f,0xd6,0xc1,0xac,0x83,0xc7,0x5f,0x1c,0xbc,0xc9,0x3a,0x3e,
		0xe9,0x38,0x18,0xf1,0x53,0x3e,0x2e,0x27,0xe1,0x5b,0xcb,0x23,0xb4,0x30,0x8d,0xdd,0xb6,
		0x97,0x6e,0xc5,0x8f,0x03,0x88,0x96,0xf3,0xfa,0xe5,0xf9,0xd0,0x9f,0x96,0x81,0x71,0xf1,
		0xcc,0x89,0x0a,0x00,0x03,0x4b,0xf3,0xd8,0xeb,0x76,0x6e,0x36,0x35,0x5c,0xce,0xb7,0xd9,
		0x8a,0x11,0xdb,0x57,0x81,0x9f,0x59,0x07,0xbf,0x37,0x97,0xb4,0x2c,0x71,0xf3,0x1f,0x03,
		0xb9,0xd8,0x32,0x66,0x7f,0x9b,0xc1,0xfc,0x1f,0x73,0x56,0x84,0x88,0xb2,0x0e,0xfe,0xf6,
		0xe5,0x61,0x6e,0x09,0x8c,0x44,0xc4,0xbf,0x59,0x05,0x73,0x96,0xaf,0xe2,0x8d,0x8f,0x3f,
		0x28,0x29,0xc9,0x67,0x51,0xf8,0xce,0xf8,0x2d,0x8b,0xf3,0x46,0xea,0x32,0x58,0x07,0x7f,
		0xf7,0xca,0xb3,0xdc,0xb4,0x9a,0xb0,0x0e,0x6e,0x35,0x60,0xad,0xcd,0xb2,0x59,0x35,0x49,
		0xfe,0x34,0x09,0xa7,0xd3,0x89,0xc0,0xd9,0xde,0x8e,0x50,0xe9,0xab,0xde,0x93,0x78,0xf7,
		0xf7,0x09,0xaa,0x0b,0x46,0x3b,0x40,0xb4,0x43,0x93,0xf1,0xf1,0x71,0xb8,0xdd,0x6e,0x84,
		0x42,0x21,0x38,0xc3,0x5a,0xc7,0x62,0xff,0x70,0xf2,0x39,0xbc,0xfe,0xe3,0x25,0xaa,0xa2,
		0xc9,0x6e,0x00,0x01,0x00,0xd7,0x3e,0x7a,0x07,0x9a,0xa6,0xa1,0x50,0x28,0x80,0xce,0xf5,
		0xad,0x77,0x32,0xf6,0x6b,0x13,0x97,0xf0,0xfd,0xab,0x67,0xf9,0xbe,0xca,0x98,0x18,0x0a,
		0xf0,0x93,0x8b,0xb7,0x71,0x50,0xed,0x4b,0xcf,0x13,0x5b,0x95,0x71,0x50,0x80,0xc8,0xab,
		0x76,0x4c,0xb8,0x02,0x55,0xef,0x3f,0x75,0x48,0xd9,0xaa,0x8c,0xdd,0xa2,0x48,0xa7,0xed,
		0x6f,0x47,0x0f,0x22,0xf9,0xc5,0xfa,0xd2,0x9a,0x88,0xf8,0x0f,0x04,0x10,0x71,0xcd,0x0b,
		0x9f,0x52,0xfb,0x34,0x91,0x3d,0xec,0x82,0x02,0x00,0xc7,0x97,0x33,0x07,0x82,0x22,0x86,
		0x61,0x20,0xb0,0x92,0xad,0xdd,0x63,0x49,0xb5,0x04,0x86,0x6a,0x6c,0xb4,0x1c,0x60,0x4d,
		0x2a,0x48,0xbb,0x7c,0xf7,0xb5,0xdd,0xcc,0x0c,0x02,0x60,0x32,0x43,0x50,0x89,0x89,0x26,
		0x97,0x8a,0xef,0xff,0xaf,0x69,0xf3,0xba,0xdc,0xd2,0x93,0x10,0x10,0x42,0xc0,0xbf,0xb2,
		0x84,0xee,0xe2,0xca,0x8e,0x02,0x91,0x74,0x1f,0x85,0xca,0x0c,0xac,0xd4,0x6d,0xfa,0xa2,
		0xc4,0x3a,0x7e,0x06,0x30,0xb2,0x9d,0xb3,0x94,0x27,0x88,0xf5,0xf5,0x35,0x18,0x86,0x01,
		0x70,0x49,0x87,0x08,0x80,0x54,0x14,0x08,0x29,0xe1,0x30,0x0d,0x04,0x17,0xdb,0x3b,0xad,
		0xde,0xd6,0xdc,0x58,0xee,0xb2,0xc1,0x30,0x0c,0x18,0x86,0x01,0x66,0x06,0xca,0x20,0x4a,
		0x09,0xa5,0xab,0x0b,0xfd,0x99,0xd9,0x96,0x7c,0x5f,0x3f,0xec,0x46,0x38,0x57,0x9f,0x01,
		0x34,0x05,0x52,0xe2,0x9a,0x77,0x64,0xf0,0xde,0x9d,0x26,0x4a,0x6c,0x03,0x56,0x37,0x68,
		0x81,0x7b,0x19,0x04,0xb6,0x59,0xdd,0x98,0xd3,0x03,0xd3,0x34,0x41,0x00,0xba,0x54,0xb5,
		0x69,0x15,0x64,0xe6,0x86,0x0a,0x03,0x14,0xa1,0xa8,0x36,0x5b,0x53,0xce,0xf6,0x82,0xf4,
		0x9e,0x78,0xa0,0x2a,0x52,0x8e,0x60,0x05,0x1c,0x29,0x25,0x1a,0x2d,0xaa,0x94,0xb2,0xee,
		0x67,0xe9,0x43,0x5a,0x09,0x8c,0xd0,0x9d,0xe9,0x7d,0xbb,0x21,0x3e,0x98,0x7b,0xdc,0xe9,
		0x81,0x61,0x18,0xb0,0xd9,0xed,0xe8,0xcf,0xfe,0xbb,0xd5,0x08,0xf6,0x04,0xa0,0x4a,0xa5,
		0xd1,0x90,0x28,0x0a,0x00,0xb4,0x53,0xe7,0x8b,0x8c,0x43,0xc3,0x82,0xcd,0x51,0xe1,0xbb,
		0xb9,0xc9,0x79,0xd3,0x34,0x31,0x7c,0x33,0x57,0xd5,0x03,0x5d,0x09,0x39,0x19,0x44,0x20,
		0x22,0x48,0x21,0x20,0xa4,0x84,0x7f,0xa3,0x08,0x4f,0x21,0xd7,0x76,0x2e,0x93,0xee,0xa3,
		0x58,0x2d,0x14,0x20,0xa4,0x84,0xcd,0x6e,0x47,0x3e,0x97,0x43,0xa4,0x70,0xb7,0xe1,0x7e,
		0xf1,0xd0,0x60,0x4c,0x1e,0xe9,0xc5,0xea,0xea,0x2a,0x22,0x89,0x6c,0xc7,0x86,0xb4,0x9f,
		0x87,0x83,0xac,0x75,0xbb,0xf0,0xd6,0xad,0xeb,0xad,0x8d,0x07,0x7a,0x02,0x0d,0x29,0x9e,
		0xf4,0x1c,0x43,0xff,0x95,0x99,0xfa,0x60,0x4c,0x1f,0xf1,0x23,0x93,0x2f,0xe2,0xf4,0xe4,
		0x12,0xed,0x45,0x7a,0x7c,0xfd,0xc2,0x00,0xbf,0x3d,0x93,0xd8,0x11,0x5f,0x31,0xa7,0x07,
		0xe1,0xd8,0x42,0x35,0x18,0x5f,0xf4,0xf4,0xe1,0xfc,0xd5,0x34,0xed,0xa7,0x7d,0xe3,0xc2,
		0x50,0x1f,0x8f,0x50,0x11,0xc1,0x6c,0xcb,0xb2,0x1e,0xa5,0x29,0x7c,0x5a,0x77,0xb8,0xb3,
		0x5f,0xed,0xc2,0x50,0x1f,0x7f,0xb8,0x98,0x7e,0xa8,0xdf,0x94,0xf7,0x8b,0xaa,0x79,0xc6,
		0x7e,0xb7,0xf3,0x57,0xd3,0x44,0x53,0x20,0x9a,0x02,0x5d,0xf4,0x86,0x9a,0x38,0x95,0x1e,
		0xab,0x6e,0xd4,0x1e,0x45,0x7b,0xff,0x72,0xaa,0x02,0x4c,0xca,0x1b,0xac,0xf9,0x9d,0x5f,
		0x37,0x1e,0xd9,0xdb,0xdf,0x46,0x95,0x4e,0x05,0x39,0x33,0xe8,0xaa,0xfc,0xdd,0x11,0x0b,
		0x7b,0xab,0x84,0xe3,0x3f,0x0e,0x72,0x9b,0x74,0x9f,0x24,0xb6,0x6b,0x00,0x00,0x00,0x00,
		0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82
};