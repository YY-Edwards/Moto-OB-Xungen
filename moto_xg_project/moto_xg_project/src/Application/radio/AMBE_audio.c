/*
 * audio.c
 *
 * Created: 2016/7/8 09:28:56
 *  Author: JHDEV3
 */ 


const char AMBE_AudioData_1[1144] = {
	0x98, 0x02, 0xB9, 0x4F, 0xA4, 0xD3, 0x80, 0xAC, 0x01, 0x81, 0x89, 0x4F, 0x23, 0x00, 0xB8, 0x06, 
	0x2E, 0x9F, 0x69, 0x65, 0x00, 0xB8, 0x19, 0x43, 0x87, 0xA1, 0x0C, 0x80, 0xA8, 0x05, 0x84, 0xD2, 
	0x4D, 0x50, 0x80, 0xC8, 0x06, 0x7C, 0xA3, 0xA1, 0xBD, 0x80, 0xC8, 0x02, 0x2F, 0x87, 0x49, 0x7D, 
	0x80, 0xC8, 0x00, 0xD8, 0x38, 0xA1, 0x35, 0x80, 0x98, 0x02, 0xB8, 0x87, 0xA9, 0x15, 0x00, 0x98, 
	0x02, 0xD7, 0xBA, 0x09, 0x7B, 0x00, 0xA8, 0x0B, 0xD4, 0x83, 0x4F, 0xC0, 0x80, 0xE8, 0x00, 0xB9, 
	0x49, 0xA9, 0xC0, 0x00, 0x98, 0x0C, 0xEB, 0x1D, 0x22, 0x04, 0x80, 0x78, 0x03, 0x24, 0x8C, 0x4B, 
	0x22, 0x00, 0x78, 0x02, 0xB3, 0xB0, 0x04, 0xE7, 0x80, 0x78, 0x15, 0xE4, 0x4F, 0x0E, 0x24, 0x80, 
	0xA7, 0x53, 0x52, 0xC9, 0xD3, 0xFF, 0x00, 0xA2, 0x61, 0xD7, 0x6F, 0xE3, 0x73, 0x80, 0xA2, 0x42, 
	0xD6, 0x9F, 0xAC, 0x18, 0x00, 0xC2, 0x39, 0xC2, 0x82, 0x2D, 0x56, 0x00, 0xD2, 0x47, 0x6A, 0x69, 
	0x02, 0x39, 0x80, 0xD2, 0x32, 0x57, 0x81, 0x02, 0x61, 0x00, 0xE8, 0x31, 0x78, 0x45, 0xA4, 0x42, 
	0x00, 0x87, 0x30, 0xA8, 0xC2, 0xFC, 0xDA, 0x00, 0x94, 0x30, 0xFA, 0xE9, 0xAE, 0x28, 0x00, 0xB8, 
	0x30, 0xFA, 0x83, 0xE3, 0x70, 0x00, 0xB8, 0x39, 0xE9, 0x16, 0x05, 0xC1, 0x80, 0x78, 0x3B, 0x62, 
	0xC9, 0xEF, 0x0C, 0x00, 0x77, 0x7C, 0x9D, 0xB4, 0xAF, 0x38, 0x00, 0x4D, 0x7E, 0x07, 0xBD, 0x7E, 
	0x14, 0x80, 0x87, 0xAD, 0x86, 0x54, 0x1C, 0xE8, 0x00, 0x80, 0xAA, 0x4E, 0x1B, 0x0C, 0xC0, 0x00, 
	0x80, 0x96, 0x7D, 0x0F, 0x6E, 0x25, 0x80, 0x82, 0x8E, 0xD4, 0x23, 0x07, 0x3D, 0x80, 0x95, 0x6D, 
	0xB9, 0x1B, 0xC0, 0xAA, 0x00, 0x96, 0x83, 0x92, 0xB4, 0xCA, 0xC0, 0x80, 0x97, 0x60, 0x3B, 0x15, 
	0x05, 0x78, 0x00, 0x97, 0x50, 0xF6, 0x80, 0x26, 0x7B, 0x00, 0xB7, 0x62, 0xB3, 0xC7, 0x03, 0xF9, 
	0x80, 0xC7, 0x67, 0xA2, 0x21, 0x42, 0x49, 0x00, 0xD8, 0x4E, 0xAB, 0x11, 0x85, 0xF2, 0x00, 0xD8, 
	0x51, 0x26, 0x07, 0x45, 0x2A, 0x80, 0xE8, 0x48, 0x62, 0x96, 0x01, 0xBD, 0x80, 0xE8, 0x33, 0x07, 
	0x81, 0xC9, 0x7A, 0x00, 0x98, 0x3A, 0xB9, 0x57, 0xC0, 0xE8, 0x00, 0x98, 0x38, 0x68, 0x22, 0x20, 
	0xF7, 0x00, 0x98, 0x25, 0x6C, 0xC0, 0x0B, 0xBB, 0x00, 0x77, 0x3E, 0x76, 0x27, 0x17, 0x74, 0x00, 
	0x86, 0x8C, 0xB9, 0x1A, 0xC0, 0x5E, 0x80, 0x84, 0xC6, 0x3D, 0x29, 0x70, 0x63, 0x80, 0x81, 0xAC, 
	0xF1, 0xB7, 0xC1, 0xF4, 0x80, 0x81, 0xAF, 0x0D, 0x13, 0x81, 0xB7, 0x80, 0x80, 0xA6, 0xB0, 0x11, 
	0xB9, 0x63, 0x80, 0x80, 0xAC, 0xF4, 0x00, 0x69, 0x81, 0x80, 0x80, 0x8E, 0x20, 0x0F, 0x09, 0x54, 
	0x80, 0x83, 0x9F, 0x99, 0x9F, 0xC1, 0x43, 0x00, 0x83, 0x8A, 0x3F, 0xA9, 0x0A, 0x67, 0x80, 0x80, 
	0x46, 0x18, 0x1E, 0x22, 0xE0, 0x00, 0x80, 0x4A, 0x3A, 0xA6, 0x21, 0xC4, 0x00, 0x84, 0x6A, 0x3A, 
	0x92, 0x01, 0x74, 0x80, 0x86, 0x9F, 0x20, 0xCC, 0x83, 0xCC, 0x80, 0x86, 0x9B, 0x38, 0xD5, 0xA4, 
	0xF1, 0x00, 0x86, 0x5C, 0x7A, 0xC2, 0x24, 0x0C, 0x00, 0x86, 0x4E, 0x2B, 0x5E, 0x65, 0x37, 0x00, 
	0x86, 0x38, 0x3A, 0x41, 0x87, 0xFC, 0x80, 0x87, 0x37, 0xB0, 0x83, 0x67, 0x0F, 0x00, 0xCD, 0xFF, 
	0xCC, 0x6D, 0xD1, 0x00, 0x00, 0x87, 0xDE, 0xE6, 0xA2, 0x39, 0x25, 0x00, 0x80, 0xBE, 0xFB, 0x88, 
	0x61, 0x56, 0x00, 0x80, 0xAE, 0xFB, 0xA6, 0x21, 0xD2, 0x80, 0x82, 0xAE, 0x6B, 0x0F, 0x01, 0xC1, 
	0x80, 0x85, 0xAE, 0x62, 0x9E, 0xAA, 0x52, 0x80, 0x80, 0xCE, 0xFB, 0xA3, 0x53, 0x45, 0x80, 0x82, 
	0x9E, 0x62, 0x97, 0xB3, 0xD0, 0x80, 0x82, 0xAF, 0x8A, 0x80, 0x9B, 0x52, 0x80, 0x83, 0x9E, 0x62, 
	0x9D, 0x83, 0xC3, 0x80, 0x82, 0x9C, 0xAA, 0x94, 0xBC, 0x72, 0x80, 0x86, 0x8E, 0xFB, 0x93, 0x8C, 
	0x55, 0x80, 0x85, 0x9E, 0x7B, 0xC8, 0x74, 0xC8, 0x80, 0x83, 0xDB, 0xD2, 0x95, 0x2C, 0x40, 0x80, 
	0x83, 0xCE, 0xA8, 0xA8, 0x23, 0xD2, 0x80, 0x83, 0xDE, 0xF0, 0xDE, 0x64, 0x17, 0x80, 0x83, 0x98, 
	0x3F, 0x41, 0x46, 0x0F, 0x00, 0x85, 0x34, 0x3A, 0xC8, 0x0F, 0xDD, 0x80, 0x86, 0x8A, 0x56, 0x94, 
	0x05, 0xF0, 0x80, 0x86, 0x5E, 0x73, 0x47, 0x94, 0x97, 0x00, 0x85, 0x4C, 0x72, 0x45, 0x8C, 0x24, 
	0x00, 0x85, 0x4D, 0x92, 0xCD, 0x03, 0x9E, 0x00, 0x82, 0xAC, 0x05, 0x2F, 0x4A, 0xD1, 0x80, 0x84, 
	0xD0, 0x80, 0x86, 0x09, 0x68, 0x80, 0x83, 0xC4, 0x4B, 0x03, 0x31, 0x61, 0x80, 0x83, 0xD5, 0x00, 
	0x91, 0x30, 0x91, 0x80, 0x74, 0xC5, 0x13, 0x0E, 0x17, 0xE3, 0x00, 0x74, 0xB2, 0x19, 0x47, 0x1F, 
	0xCB, 0x00, 0x71, 0xC2, 0x18, 0x93, 0x6E, 0x61, 0x80, 0x72, 0xC8, 0x06, 0x0F, 0x46, 0x00, 0x80, 
	0x74, 0xB6, 0x5C, 0x1F, 0x36, 0xE7, 0x80, 0x84, 0x7E, 0x76, 0xBE, 0x68, 0x94, 0x00, 0x85, 0x5F, 
	0x7A, 0x80, 0xA0, 0x23, 0x00, 0x86, 0x5C, 0xDA, 0x19, 0xC8, 0x34, 0x80, 0x86, 0xCE, 0x20, 0x50, 
	0x19, 0x72, 0x80, 0x85, 0x9E, 0x71, 0x4F, 0x6B, 0x51, 0x80, 0x85, 0x8A, 0x3F, 0x4D, 0x2C, 0x64, 
	0x00, 0x85, 0x78, 0x9E, 0x5A, 0x2C, 0xC9, 0x80, 0x84, 0x74, 0x3E, 0xB1, 0x45, 0xFB, 0x80, 0x84, 
	0x7E, 0x09, 0x45, 0x56, 0x93, 0x00, 0x86, 0x3E, 0x08, 0x4E, 0x27, 0x5C, 0x00, 0x7E, 0x3A, 0x1B, 
	0x2F, 0x83, 0xB3, 0x80, 0x97, 0x29, 0xA8, 0x59, 0x70, 0xAC, 0x00, 0x97, 0x1D, 0xDF, 0xAA, 0xC8, 
	0x1A, 0x00, 0x98, 0x15, 0x52, 0xC6, 0x20, 0x7C, 0x80, 0xCC, 0xF8, 0x0D, 0x1F, 0x28, 0x21, 0x80, 
	0x87, 0xEC, 0x81, 0x05, 0x17, 0xB2, 0x00, 0x86, 0xB6, 0x90, 0x11, 0x54, 0x3F, 0x00, 0x85, 0x76, 
	0x1D, 0x0D, 0x64, 0xD5, 0x00, 0x83, 0x9E, 0x94, 0x4D, 0x43, 0x5E, 0x00, 0x85, 0x98, 0x90, 0x24, 
	0x0B, 0x6B, 0x00, 0x85, 0x9E, 0x5A, 0x21, 0xA3, 0xDB, 0x80, 0x84, 0x7C, 0xE6, 0x57, 0xAB, 0xA0, 
	0x80, 0x85, 0x53, 0xAA, 0x0F, 0x43, 0x19, 0x80, 0x87, 0x9A, 0xEB, 0x51, 0x4E, 0x1C, 0x80, 0xC8, 
	0xF6, 0x18, 0x7F, 0x25, 0x14, 0x00, 0x9D, 0xF4, 0x15, 0x90, 0xD9, 0x96, 0x00, 0xB8, 0xDD, 0x04, 
	0x52, 0x0D, 0x17, 0x80, 0x3E, 0x9C, 0xDA, 0x20, 0x4E, 0xF0, 0x80, 0xA7, 0x8A, 0xBA, 0xD3, 0x10, 
	0x38, 0x00, 0x84, 0xE4, 0xFC, 0x8B, 0xEE, 0x3F, 0x00, 0x83, 0x9E, 0x74, 0x0F, 0x17, 0x71, 0x00, 
	0x84, 0x5A, 0x9C, 0x0D, 0x4F, 0x06, 0x80, 0x94, 0x5C, 0x6A, 0x91, 0x29, 0x69, 0x80, 0x97, 0x60, 
	0x79, 0x0D, 0x85, 0x5C, 0x80, 0x97, 0x6C, 0x28, 0xD2, 0x27, 0x22, 0x00, 0xA8, 0x52, 0x72, 0xD1, 
	0x00, 0xF8, 0x00, 0xA8, 0x5F, 0x22, 0x15, 0x06, 0x19, 0x80, 0xA7, 0x37, 0x7A, 0x45, 0xAF, 0x80, 
	0x00, 0xA7, 0x2A, 0x59, 0x59, 0x02, 0x7C, 0x80, 0x98, 0x15, 0x7A, 0x87, 0x16, 0x69, 0x80, 0x98, 
	0x02, 0xA5, 0xE9, 0xEF, 0x8D, 0x80, 0x98, 0x1D, 0x2A, 0x80, 0x2B, 0x2A, 0x00, 0x98, 0x27, 0xD2, 
	0x81, 0x63, 0xAA, 0x00, 0x88, 0x07, 0x44, 0x93, 0x6A, 0x66, 0x80, 0x78, 0x12, 0xA8, 0xC3, 0x02, 
	0xCE, 0x80, 0x78, 0x1F, 0x2C, 0x21, 0xC2, 0x2A, 0x80, 0x98, 0x19, 0x60, 0x1D, 0xA3, 0x90, 0x80, 
	0x98, 0x1B, 0x24, 0x93, 0x8B, 0xA4, 0x80, 0x78, 0x1B, 0xB3, 0xCF, 0x2C, 0xBD, 0x80, 0x78, 0x03, 
	0x95, 0xB0, 0x0C, 0x97, 0x80, 0x78, 0x13, 0x5C, 0x07, 0x2C, 0x25, 0x00, 0x88, 0x2A, 0x16, 0x53, 
	0xA1, 0x06, 0x80, 0x88, 0xB4, 0x1F, 0x13, 0xC1, 0x76, 0x00, 0x98, 0x47, 0x30, 0x1E, 0x29, 0x34, 
	0x00, 0x98, 0x38, 0x58, 0xA1, 0xA9, 0x08, 0x00, 0x88, 0x7B, 0x61, 0xE1, 0x01, 0x59, 0x00, 0x88, 
	0x85, 0xCA, 0x45, 0x8A, 0x32, 0x00, 0x88, 0x54, 0xCC, 0x05, 0x0A, 0x08, 0x00, 0x68, 0x48, 0x28, 
	0x9B, 0x8C, 0x12, 0x80, 0x67, 0x30, 0x45, 0x45, 0xDB, 0x21, 0x00, 0x64, 0x4E, 0xDA, 0x23, 0xF2, 
	0xA3, 0x00, 0x68, 0x7B, 0xBA, 0x95, 0xE2, 0x7E, 0x00, 0x77, 0x63, 0xBC, 0x09, 0xD2, 0x33, 0x80, 
	0x74, 0x40, 0x80, 0x0F, 0x09, 0x15, 0x80, 0x75, 0x44, 0x4C, 0xD1, 0xE0, 0x15, 0x80, 0x85, 0x30, 
	0x4A, 0x05, 0xE0, 0x83, 0x80, 0x00, 0x00, 0x00
};