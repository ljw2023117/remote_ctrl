#pragma once
#include "pch.h"
#include "framework.h"

class CPacket
{
public:
	CPacket() : sHead(0), nLength(0), sCmd(0), sSum(0) {}
	~CPacket() = default;
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize)
	{
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) strData = std::string((const char*)pData, nSize);
		sSum = 0;
		for (size_t i = 0; i < nSize; i++)
		{
			sSum += BYTE(pData[i]) & 0xFF;
		}
	}

	CPacket(const BYTE* pData, size_t& nSize)
	{
		size_t i = 0;
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF)
			{
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		// 包数据可能不全，或者包头未能全部接收到
		if (i + 4 + 2 + 2 > nSize)
		{
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i), i += 4;
		if (nLength + i > nSize) // 包未完全接收到，就返回，解析失败
		{
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i), i += 2;
		if (nLength > 4) strData = std::string((const char*)(pData + i), (size_t)(nLength - 4));
		i += nLength - 4;
		sSum = *(WORD*)(pData + i), i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		nSize = sSum == sum ? i : 0;
	}
	CPacket(const CPacket& pack)
	{
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}

	CPacket& operator=(const CPacket& pack)
	{
		if (this == &pack) return *this;
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}

	int Size() { return nLength + 6; }
	const char* Data()
	{
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead, pData += 2;
		*(DWORD*)pData = nLength, pData += 4;
		*(WORD*)pData = sCmd, pData += 2;
		memcpy(pData, strData.c_str(), strData.size()), pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}
public:
	WORD sHead;	// 固定包头 0xFEFF
	DWORD nLength; // 包长度，从控制命令到校验和结束
	WORD sCmd;	// 控制命令
	std::string strData; // 包数据
	WORD sSum; // 和校验
	std::string strOut;	// 整个包的数据
};

typedef struct MouseEvent
{
	WORD nAction; // 移动、单击、双击
	WORD nButton; // 左键、中键、右键
	POINT ptXY;	  // 坐标
}MOUSEEV, *PMOUSEEV;

typedef struct FileInfo
{
	FileInfo() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;			// 是否有效
	BOOL IsDirectory;		// 是否为目录 0否1是
	BOOL HasNext;			// 是否还有后续 0否1是
	wchar_t szFileName[256];	// 文件名
}FILEINFO, *PFILEINFO;

class CServerSocket
{
public:
	static CServerSocket* getInstance()
	{
		static CServerSocket m_instance;
		return &m_instance;
	}

	bool InitSocket()
	{
		if (m_sock == -1) return false;
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_port = htons(9527);
		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		{
			return false;
		}
		if (listen(m_sock, 1) == -1)
		{
			return false;
		}
		return true;
	}

	bool AcceptClient()
	{
		TRACE("enter AcceptClient\r\n");	// TRACE是MFC提供的调试宏，工作在debug模式下
		sockaddr_in client_adr;
		int cli_sz = sizeof(client_adr);
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
		TRACE("m_client = %d\r\n", m_client);
		if (m_client == -1) return false;
		return true;
	}

#define BUFFER_SIZE 4096
	int DealCommand()
	{
		if (m_client == -1) return -1;
		char* buffer = new char[BUFFER_SIZE];
		if (buffer == NULL)
		{
			TRACE("内存不足！\r\n");
			return -2;
		}
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true)
		{
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0)
			{
				delete[] buffer;
				return -1;
			}
			TRACE("recv %d\r\n", len);
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0)
			{
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				delete[] buffer;
				return m_packet.sCmd;
			}
		}
		delete[] buffer;
		return -1;
	}

	void CloseClient()
	{
		closesocket(m_client);
		m_client = INVALID_SOCKET;
	}

	bool Send(const char* pData, int nSize)
	{
		if (m_client == -1) return false;
		return send(m_client, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& pack)
	{
		if (m_client == -1) return false;
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}

	bool GetFilePath(CString& strPath)
	{
		if ((m_packet.sCmd >= 2 && m_packet.sCmd <= 4) || m_packet.sCmd == 9)
		{
			TRACE(_T("接收到的路径: %s\n"), (wchar_t*)m_packet.strData.c_str());
			strPath = CString((wchar_t*)m_packet.strData.c_str());
			return true;
		}
		return false;
	}

	bool GetMouseEvent(MOUSEEV& mouse)
	{
		if (m_packet.sCmd == 5)
		{
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(mouse));
			return true;
		}
		return false;
	}

	CPacket& GetPacket()
	{
		return m_packet;
	}

private:
	SOCKET m_client;
	SOCKET m_sock;
	CPacket m_packet;
	CServerSocket()
		: m_client(INVALID_SOCKET)
	{
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, _T("无法初始化套接字环境"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(AF_INET, SOCK_STREAM, 0);
	}
	~CServerSocket()
	{
		closesocket(m_sock);
		WSACleanup();
	}
	CServerSocket(const CServerSocket& server) = delete;
	CServerSocket& operator=(const CServerSocket& server) = delete;

	BOOL InitSockEnv()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
		{
			return FALSE;
		}
		return TRUE;
	}
};
