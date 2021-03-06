#pragma once

#include "my.h"
#include "iUnpackObject.h"
#include "DecodeControl.h"
#include "Tool.h"
#include <ShlObj.h>
#include <string>
#include "Compression.h"



class UnpackGameexe : public iUnpackObject
{
	std::wstring m_FileName;

public:
	UnpackGameexe(){}
	~UnpackGameexe(){};

	static BYTE GameExeKey[256];

	Void FASTCALL SetFile(LPCWSTR FileName)
	{
		m_FileName = FileName;
	}

	NTSTATUS FASTCALL Unpack(PVOID UserData)
	{
		NTSTATUS          Status;
		NtFileDisk        File, Writer;
		PDecodeControl    Code;
		PBYTE             Buffer, CorrentBuffer, DecompBuffer;
		ULONG_PTR         Size, Attribute;
		DWORD             CompLen, DecompLen;
		std::wstring      FileName, FullPath, FullOutDirectory;
		WCHAR             ExeDirectory[MAX_PATH];

		Code = (PDecodeControl)UserData;
		Status = File.Open(m_FileName.c_str());
		if (NT_FAILED(Status))
			return Status;

		Size = File.GetSize32();
		Buffer = (PBYTE)AllocateMemoryP(Size);
		if (!Buffer)
		{
			File.Close();
			return STATUS_NO_MEMORY;
		}

		File.Read(Buffer, Size);
		File.Close();

		CorrentBuffer = Buffer + 8;

		for (ULONG i = 0; i < Size - 8; i++)
		{
			if (Code->DatNeedKey)
				CorrentBuffer[i] ^= Code->PrivateKey[i & 0x0f];

			CorrentBuffer[i] ^= GameExeKey[i & 0xff];
		}

		CompLen = *(PDWORD)(CorrentBuffer);
		DecompLen = *(PDWORD)(CorrentBuffer + 4);
		DecompBuffer = (PBYTE)AllocateMemoryP(DecompLen);
		DecompressData(CorrentBuffer + 8, DecompBuffer, DecompLen); 

		RtlZeroMemory(ExeDirectory, sizeof(ExeDirectory));
		Nt_GetExeDirectory(ExeDirectory, countof(ExeDirectory));

		static WCHAR OutDirectory[] = L"__Unpack__\\Gameexe\\";

		FullOutDirectory = ExeDirectory + std::wstring(OutDirectory);
		Attribute = Nt_GetFileAttributes(FullOutDirectory.c_str());
		if (Attribute == 0xffffffff)
			SHCreateDirectory(NULL, FullOutDirectory.c_str());

		FileName = GetFileName(m_FileName);
		FullPath = FullOutDirectory + FileName;

		FullPath = ReplaceFileNameExtension(FullPath, L".txt");
		Status = Writer.Create(FullPath.c_str());
		if (NT_FAILED(Status))
		{
			FreeMemoryP(Buffer);
			FreeMemoryP(DecompBuffer);
			return Status;
		}

		static BYTE Bom[] = { 0xff, 0xfe };

		Writer.Write(Bom, sizeof(Bom));
		Writer.Write(DecompBuffer, DecompLen);

		FreeMemoryP(Buffer);
		FreeMemoryP(DecompBuffer);
		Writer.Close();
		
		return STATUS_SUCCESS;
	}
};