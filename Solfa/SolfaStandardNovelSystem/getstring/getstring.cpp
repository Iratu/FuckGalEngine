#include <Windows.h>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <sstream>
#include <iostream>
#include <iomanip>

using namespace std;

typedef  unsigned char byte;
typedef  unsigned long dword;

typedef unordered_map<dword, dword> DwordMap;

wchar_t *AnsiToUnicode(const char *str)
{
	static wchar_t result[1024];
	int len = MultiByteToWideChar(932, 0, str, -1, NULL, 0);
	MultiByteToWideChar(932, 0, str, -1, result, len);
	result[len] = L'\0';
	return result;
}



//È«²¿Ìæ»»
string replace_all(string dststr, string oldstr, string newstr)
{
	string::size_type old_len = oldstr.length();
	if (old_len == 0)
		return dststr;

	string ret = dststr;
	string::size_type off;
	while (true)
	{
		off = ret.find(oldstr);
		if (off == string::npos)
			break;
		ret = ret.replace(off, old_len, newstr);
	}

	return ret;
}


void fixstr(char *str, dword len)
{
	for (dword i = 0; i < len; i++)
	{
		if (str[i] == 0)
			str[i] = 'b';
	}
}


byte* p_fileend;


dword vm_strlen(byte* b)
{
	byte* c = b;
	if (c >= p_fileend) return 0;
	while (1)
	{
		if ((*c == 0x1B && *(c + 1) == 0x60) || (*c == 0x1B && *(c + 1) == 0x00))
		{
			break;
		}
		c++;
	}
	return c - b;
}

/*
//1b 12 00 01 06 00 20 1d 00 00 00 ff 02 06 00 70 da 5e 02 00 ff ff
byte cmp_byte[]={0x1B,0x12,0x00,0x01,0x06,0x00,0x20,0x1d,0x00,0x00,0x00,0xFF,0x02,0x06,0x00,0x70,0xda,0x5e,0x02,0x00,0xFF,0xFF};
byte* text_point(byte* b)
{
	if(memcmp(b,cmp_byte,sizeof(cmp_byte))==0)
	{
		return &b[sizeof(cmp_byte)];
	}
	return 0;
}
*/


byte start_byte[] = { 0x1B, 0x12, 0x00, 0x01};
byte end_byte[] = {0x00, 0x00, 0xff, 0xFF};
byte tag[] = {0x1B, 0x03, 0x00, 0x01};
//byte tag[] = { 0x00, 0x7a, 0x00, 0x04 };
byte* text_point(byte* b)
{
	byte *pos = b;
	if (!memcmp(b, start_byte, sizeof(start_byte)))
	{

		while (memcmp(pos, end_byte, sizeof(end_byte)))
		{
			if (pos >= p_fileend)
				return NULL;
			pos++;
		}
		byte* end = pos + sizeof(end_byte);
		if (!memcmp(end, tag, 4))
		{
			return end + sizeof(tag);
		}
		if (*end != 0x1B)
			return end;
	}
	return NULL;
}


//1B 12 00 01 ?? 00 78 4F 00 00 00 79 01 7A 00 strlen
//This was easier than I thought it'd be, seems the hex string for this is similar in most games, just had to replace b[7], same as always find a name, and get the hex string needed.
byte* is_name_text(byte* b, dword &length)
{
	length = 0;
	if (b[0] == 0x1B &&
		b[1] == 0x12 &&
		b[2] == 0x00 &&
		b[3] == 0x01 &&

		b[5] == 0x00 &&
		b[6] == 0x78 &&
		b[7] == 0x16 &&//Only this line changed.
		b[8] == 0x00 &&
		b[9] == 0x00 &&
		b[10] == 0x00 &&
		b[11] == 0x79 &&
		b[12] == 0x01 &&
		b[13] == 0x7A &&
		b[14] == 0x00)
	{
		length = b[15];
		return &b[16];
	}
	return 0;
}
//1B 12 00 01 ?? 00 78 4F 00 00 00 79 01 7A 00
//1B 12 00 01 ?? 00 78 70 00 00 00 79 01 7A 00 strlen
//1B 12 00 01 25 00 78 16 00 00 00 79 01 7a 00 
//1B 12 00 01 2d 00 78 16 00 00 00 79 01 7a 00
//Seems to be the same for most games, see the b[7] line for more info.
byte* is_select_text(byte* b, dword &length)
{
	length = 0;
	if (b[0] == 0x1B &&
		b[1] == 0x12 &&
		b[2] == 0x00 &&
		b[3] == 0x01 &&

		b[5] == 0x00 &&
		b[6] == 0x78 &&
		b[7] == 0x16 && //go figure, only had to replace this line? as with the is_box_text function, find this number before the string hex-code
		b[8] == 0x00 &&
		b[9] == 0x00 &&
		b[10] == 0x00 &&
		b[11] == 0x79 &&
		b[12] == 0x01 &&
		b[13] == 0x7A &&
		b[14] == 0x00)
	{
		length = b[15];
		return &b[16];
	}
	return 0;
}

//²»ÖªµÀÕâÊÇÊ²Ã´¡­¡­
//1E 00 00 00 00 ?? ?? 00 00
//06 00 70 df 59 02 00 ff ff
//?? 42 7b 02 52 ff 02 06 00 70 df 59 02 00 ff ff
//1B 60 00 ff 1B 1a 02 ff 1b 03 00 01 06 00 70 55 10 02 00 ff ff 

//This function grabs the Box text, find the hex-code that is before a string used inside a box, see example folder, file: box_text_example.png
byte* is_box_text(byte* b,dword &length)
{

	length = 0;
	if( 
		b[1] == 0x00 &&
		b[2] == 0x70 &&
		b[3] == 0xdf &&
		b[4] == 0x59 &&
		b[5] == 0x02 &&
		b[6] == 0x00 &&
		b[7] == 0xff &&
		b[8] == 0xff
		)
	{
		//1b 12 00 01 06 00 20 1d
		size_t i = 8;
		while( i < 500) {
			i++;
			if (b[i] == 0x1B && (b[i + 1]) == 0x12 && (b[i + 2]) == 0x00)
			{
				//i = 0xff;
				//length - 8;
				break;
			}
			length++;
		}
		return &b[9];
	}

	return 0;
}


int main()
{

	FILE* f;
	FILE* txt;
	byte* data;
	dword size;
	dword read_tell;

	static char print_chars[1024];
	byte* char_pointer;
	dword char_length;

	DwordMap mydic;


	f = fopen("CODE","rb");
	txt = fopen("game_text.txt","wb");

	fwrite("\xFF\xFE", 2, 1, txt);

	if(f && txt)
	{
		fseek(f,0,SEEK_END);
		size = ftell(f);
		//size = sizeof(byte);
		fseek(f,0,SEEK_SET);


		data = (byte*)malloc(size);

		fread(data,size,1,f);
		fclose(f);
		
		p_fileend = data + size;

		read_tell = 0;

		dword line_num = 0;
		for (read_tell = 0; read_tell < size; read_tell++)
		{
			
			char_pointer = text_point(&data[read_tell]);
			if (char_pointer)
			{
				
				char_length = vm_strlen(char_pointer);
				if (char_length != 0)
				{
					memcpy(print_chars, char_pointer, char_length);
					print_chars[char_length] = 0;
					fixstr(print_chars, char_length);
					if (mydic.find((char_pointer - data)) == mydic.end())
					{
						string dispstr = replace_all(print_chars, "\x1b\x60\x00\xff", "\\n");
						dispstr = replace_all(dispstr, "\n", "\\a");
						fwprintf(txt, L"○%08X○%08d●\r\n%s\r\n\r\n", (char_pointer - data), line_num++, AnsiToUnicode(dispstr.c_str()));

						mydic.insert(DwordMap::value_type((char_pointer - data), 0));
					}

				}

				//fprintf(txt, "%s\r\n", print_chars);
			}
			
			char_pointer = is_name_text(&data[read_tell], char_length);
			if (char_pointer && char_length)
			{
				memcpy(print_chars, char_pointer, char_length);
				print_chars[char_length] = 0;
				if (mydic.find((char_pointer - data)) == mydic.end())
				{
					fwprintf(txt, L"○%08X○%08d●\r\n%s\r\n\r\n[", (char_pointer - data), line_num++, AnsiToUnicode(print_chars));
					for (size_t i = 0; i < char_length; i++)
					{
						fwprintf(txt, L"%02X ", char_pointer[i]);
					}
					fwprintf(txt, L"]\r\n\r\n");

					mydic.insert(DwordMap::value_type((char_pointer - data), 0));
				}
			}

			char_pointer = is_select_text(&data[read_tell], char_length);

			if (char_pointer && char_length)
			{

				memcpy(print_chars, char_pointer, char_length);
				print_chars[char_length] = 0;
				if (mydic.find((char_pointer - data)) == mydic.end())
				{
					fwprintf(txt, L"○%08X○%08d●\r\n%s\r\n\r\n[", (char_pointer - data), line_num++, AnsiToUnicode(print_chars));
					for (size_t i = 0; i < char_length; i++)
					{
						fwprintf(txt, L"%02X ", char_pointer[i]);
					}
					fwprintf(txt, L"]\r\n\r\n");

					mydic.insert(DwordMap::value_type((char_pointer - data), 0));
				}
			}

			char_pointer = is_box_text(&data[read_tell], char_length);
			if (char_pointer && char_length)
			{
				memcpy(print_chars, char_pointer, char_length);

				print_chars[char_length] = 0;
				if (mydic.find((char_pointer - data)) == mydic.end())
				{
					fwprintf(txt, L"○%08X○%08d●\r\n%s\r\n\r\n[ ", (char_pointer - data), line_num++, AnsiToUnicode(print_chars));
					for (size_t i = 0; i < char_length; i++)
					{
						fwprintf(txt, L"%02X ", char_pointer[i]);
					}
					fwprintf(txt, L"]\r\n\r\n");
					mydic.insert(DwordMap::value_type((char_pointer - data), 0));
				}
			}
		}
		
		fclose(txt);
		free(data);
	}

	return 0;
}