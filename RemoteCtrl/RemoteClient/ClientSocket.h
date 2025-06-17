#pragma once
#include "pch.h"
#include "framework.h"

#include <string>
#include <vector>

inline std::string GetErrInfo(int wsaErrCode)
{
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}


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
		if (nSize > 0)
		{
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else strData.clear();
		sSum = 0;
		for (size_t i = 0; i < nSize; i++)
		{
			sSum += BYTE(strData[i]) & 0xFF;
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
		// �����ݿ��ܲ�ȫ�����߰�ͷδ��ȫ�����յ�
		if (i + 4 + 2 + 2 > nSize)
		{
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i), i += 4;
		if (nLength + i > nSize) // ��δ��ȫ���յ����ͷ��أ�����ʧ��
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
	WORD sHead;	// �̶���ͷ 0xFEFF
	DWORD nLength; // �����ȣ��ӿ������У��ͽ���
	WORD sCmd;	// ��������
	std::string strData; // ������
	WORD sSum; // ��У��
	std::string strOut;	// ������������
};

typedef struct MouseEvent
{
	WORD nAction; // �ƶ���������˫��
	WORD nButton; // ������м����Ҽ�
	POINT ptXY;	  // ����
}MOUSEEV, * PMOUSEEV;

typedef struct FileInfo
{
	FileInfo() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;			// �Ƿ���Ч
	BOOL IsDirectory;		// �Ƿ�ΪĿ¼ 0��1��
	BOOL HasNext;			// �Ƿ��к��� 0��1��
	wchar_t szFileName[256];	// �ļ���
}FILEINFO, * PFILEINFO;

class CClientSocket
{
public:
	static CClientSocket* getInstance()
	{
		static CClientSocket m_instance;
		return &m_instance;
	}

	bool InitSocket(int nIP, unsigned short nPort)
	{
		if (m_sock != INVALID_SOCKET) CloseSocket();
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (m_sock == -1) return false;
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = htonl(nIP);
		serv_adr.sin_port = htons(nPort);
		if (serv_adr.sin_addr.s_addr == INADDR_NONE)
		{
			AfxMessageBox(_T("����ʧ��"));
			return false;
		}
		int ret = connect(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr));
		if (ret == -1)
		{
			AfxMessageBox(_T("����ʧ��"));
			TRACE("����ʧ�ܣ�%d %s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
		}
		return true;
	}

#define BUFFER_SIZE 2048000
	int DealCommand()
	{
		if (m_sock == -1) return -1;
		char* buffer = m_buffer.data();
		static size_t index = 0;
		while (true)
		{
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
			if ((int)len < 0)
			{
				int err = WSAGetLastError();
				TRACE("--- error :%d\r\n", err);
				GetErrInfo(err);
			}
			if (len == 0 && index == 0)
			{
				TRACE("�ر�����\r\n");
				return -1;
			}
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0)
			{
				//TRACE("ʹ����%d�ֽ�\r\n", len);
				memmove(buffer, buffer + len, index - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}

	void CloseSocket()
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}

	bool Send(const char* pData, int nSize)
	{
		if (m_sock == -1) return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& pack)
	{
		TRACE("m_sock = %d\r\n", m_sock);
		if (m_sock == -1) return false;
		return send(m_sock, pack.Data(), pack.Size(), 0) > 0;
	}

	bool GetFilePath(std::string& strPath)
	{
		if ((m_packet.sCmd >= 2 && m_packet.sCmd <= 4) || m_packet.sCmd == 9)
		{
			strPath = m_packet.strData;
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
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;
	CClientSocket()
	{
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
		memset(m_buffer.data(), 0, BUFFER_SIZE);
	}
	~CClientSocket()
	{
		closesocket(m_sock);
		WSACleanup();
	}
	CClientSocket(const CClientSocket& server) = delete;
	CClientSocket& operator=(const CClientSocket& server) = delete;

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
