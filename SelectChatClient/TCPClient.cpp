// 2020�� 1�б� ��Ʈ��ũ���α׷��� ���� 3��
// ����: �̹ο� �й�: 16011100
// �÷���: VS2019

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <time.h>
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"

#define BUFSIZE		512

// ��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

void DisplayText(char* fmt, ...);	// ���� ��Ʈ�� ��� �Լ�
void err_quit(char* msg);			// ���� ��� �Լ�
void err_display(char* msg);		// ���� ��� �Լ�

int isNumber(char* string);			// ��Ʈ��ȣ �˻� 

int start_chatting();				// ä�� ���� �Լ�
void setchatIPnPORT(SOCKADDR_IN);	// ä�� �ؽ�Ʈ ��� ���� �Լ�
int end_chatting();					// ä�� ���� �Լ�

time_t t;		// �ð� ����
struct tm tm;	// �ð� ����ü

char IP[16];			// IP�ּ�
int  PORT;				// ��Ʈ��ȣ
char buf[BUFSIZE + 1];
char chatbuf[BUFSIZE + 1];
BOOL cnt = true, chk1, chk2;

char prevNick[20], newNick[20]; // ��ȭ�� ����

HWND hEnterButton;				// ���� ��ư
HWND hExitButton;				// ������ ��ư
HWND hSendButton;				// ������ ��ư
HWND hEdit1, hEdit2;			// ���� ��Ʈ��
HWND hChk1, hChk2;				// üũ�ڽ�

HANDLE hReadEvent, hWriteEvent;


SOCKET sock;
SOCKADDR_IN remoteaddr;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	// �̺�Ʈ ����
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if (hReadEvent == NULL) return 1;
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hWriteEvent == NULL) return 1;

	// ��ȭ���� ����
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// �̺�Ʈ ����
	CloseHandle(hReadEvent);
	CloseHandle(hWriteEvent);

	return 0;
}

// ��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// �� ĭ�̶� ����� ������ connect ��Ȱ��ȭ 
	if (GetDlgItemText(hDlg, IDC_IP, IP, 16) != NULL &&
		GetDlgItemText(hDlg, IDC_PORT, buf, BUFSIZE + 1) != NULL &&
		GetDlgItemText(hDlg, IDC_NICK, buf, 20) != NULL)
		EnableWindow(GetDlgItem(hDlg, IDC_ENTER), TRUE);
	else
		EnableWindow(GetDlgItem(hDlg, IDC_ENTER), FALSE);

	switch (uMsg) {
	case WM_INITDIALOG:
		hEdit1 = GetDlgItem(hDlg, IDC_CHAT);
		hEdit2 = GetDlgItem(hDlg, IDC_CHATTING);
		hSendButton = GetDlgItem(hDlg, IDC_ENTER);
		hExitButton = GetDlgItem(hDlg, IDC_EXIT);
		hChk1 = GetDlgItem(hDlg, IDC_ROOM1);
		hChk2 = GetDlgItem(hDlg, IDC_ROOM2);
		EnableWindow(hEdit1, FALSE);	// connect �ϱ� ���� ä�� �Է�â ��Ȱ��ȭ
		EnableWindow(hExitButton, FALSE);	// connect �ϱ� ���� �������ư ��Ȱ��ȭ
		SendMessage(hEdit1, EM_SETLIMITTEXT, BUFSIZE, 0);	// �ִ� �Է� �ؽ�Ʈ ����
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ENTER:
			GetDlgItemText(hDlg, IDC_IP, IP, 16);	// IP �ּ� ( char* )
			GetDlgItemText(hDlg, IDC_PORT, buf, BUFSIZE + 1);		// ��Ʈ ��ȣ ( char* )
			GetDlgItemText(hDlg, IDC_NICK, prevNick, 20);		// ��ȭ��

			PORT = atoi(buf);


			if (!isNumber(buf))
			{
				SetDlgItemText(hDlg, IDC_ERROR, "[ error ] 0 ~ 65535 ������ ��Ʈ��ȣ�� �Է����ּ���.");
			}
			else if (PORT < 0 || PORT > 65535) // �Ұ����� ��Ʈ ��ȣ
			{
				SetDlgItemText(hDlg, IDC_ERROR, "[ error ] 0 ~ 65535 ������ ��Ʈ��ȣ�� �Է����ּ���.");
			}
			else if (strcmp(prevNick, "ERROR") == 0)
			{
				SetDlgItemText(hDlg, IDC_ERROR, "��ȭ������ ERROR�� ����� �� �����ϴ�.");
			}
			else
			{
				chk1 = IsDlgButtonChecked(hDlg, IDC_ROOM1) ? TRUE : FALSE;
				chk2 = IsDlgButtonChecked(hDlg, IDC_ROOM2) ? TRUE : FALSE;

				if (chk1)
				{
					EnableWindow(GetDlgItem(hDlg, IDC_IP), FALSE);	// ip ���� �Ұ�
					EnableWindow(GetDlgItem(hDlg, IDC_PORT), FALSE);		// ��Ʈ��ȣ ���� �Ұ�
					EnableWindow(GetDlgItem(hDlg, IDC_ENTER), FALSE);

					start_chatting();										// ä�� ����

					EnableWindow(hEdit1, TRUE);

					SetDlgItemText(hDlg, IDC_ERROR, "");

					EnableWindow(hChk1, FALSE);
					EnableWindow(hChk2, FALSE);
				}
				else if (chk2)
				{
					EnableWindow(GetDlgItem(hDlg, IDC_IP), FALSE);	// ip ���� �Ұ�
					EnableWindow(GetDlgItem(hDlg, IDC_PORT), FALSE);		// ��Ʈ��ȣ ���� �Ұ�

					start_chatting();										// ä�� ����

					EnableWindow(hEdit1, TRUE);
					EnableWindow(hEnterButton, FALSE);

					SetDlgItemText(hDlg, IDC_ERROR, "");

					EnableWindow(hChk1, FALSE);
					EnableWindow(hChk2, FALSE);
				}
				else
				{
					SetDlgItemText(hDlg, IDC_ERROR, "[ error ] ä�ù��� �����ϼ���.");
				}
			}
			return TRUE;
		case IDC_SEND:
			/* --------------------------------------------------------------------- ä�� ���� --------------------------------------------------------------------- */
			SOCKADDR_IN peeraddr;
			int addrlen;
			int retval;

			if (cnt == false)
			{
				SetDlgItemText(hDlg, IDC_ERROR, "[ error ] ä�ù濡 �����ϼ���.");
				SetDlgItemText(hDlg, IDC_CHAT, (LPCSTR)NULL);	// ä��â �ʱ�ȭ
				EnableWindow(hChk1, TRUE);
				EnableWindow(hChk2, TRUE);
			}
			else
			{
				EnableWindow(hSendButton, FALSE);						// ������ ��ư ��Ȱ��ȭ : ����Ŭ�� ����
				WaitForSingleObject(hReadEvent, INFINITE);				// �б� �Ϸ� ��ٸ���

				strncpy(chatbuf, "", strlen(chatbuf));
				strncpy(buf, "", strlen(buf));

				GetDlgItemText(hDlg, IDC_CHAT, chatbuf, BUFSIZE + 1);	// �Է��� ä��

				if (strcmp(chatbuf, "������") == 0)
					strcat(buf, chatbuf);
				else
				{
					strcat(buf, prevNick);
					strcat(buf, "  : ");
					strcat(buf, chatbuf);
				}

				retval = send(sock, buf, strlen(buf), 0);
				if (retval == SOCKET_ERROR) {
					err_display(const_cast<char*>("send()"));
				}

				SetEvent(hWriteEvent);							// ���� �Ϸ� �˸���			

				SendMessage(hEdit1, EM_SETSEL, 0, -1);
				SetDlgItemText(hDlg, IDC_CHAT, (LPCSTR)NULL);	// ä��â �ʱ�ȭ
				SetFocus(hEdit1);
				EnableWindow(hSendButton, TRUE);				// ������ ��ư Ȱ��ȭ
				EnableWindow(hExitButton, TRUE);
			}
			return TRUE;
		case IDC_EXIT:
			closesocket(sock);
			EnableWindow(hExitButton, FALSE);
			EnableWindow(hChk1, TRUE);
			EnableWindow(hChk2, TRUE);
			SetDlgItemText(hDlg, IDC_CHATTING, (LPCSTR)NULL);
			SetDlgItemText(hDlg, IDC_CHAT, (LPCSTR)NULL);
			strcpy(prevNick, "");
			strcpy(newNick, "");
			return TRUE;
		case IDC_CLOSE:
			if (strcmp(prevNick, ""))
				end_chatting();
			EndDialog(hDlg, IDCLOSE);
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

// ���� ��Ʈ�� ��� �Լ�
void DisplayText(char* fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[BUFSIZE + 256];
	vsprintf(cbuf, fmt, arg);
	strcat(cbuf, "\r\n");
	int nLength = GetWindowTextLength(hEdit2);
	SendMessage(hEdit2, EM_SETSEL, nLength, nLength);	 		// ���� ä�� �ڿ� �̾� ���̱�
	SendMessage(hEdit2, EM_REPLACESEL, FALSE, (LPARAM)cbuf);

	va_end(arg);
}

// ���� �Լ� ���� ��� �� ����
void err_quit(char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// ���� �Լ� ���� ���
void err_display(char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// ������ �޴� ������ �Լ�
DWORD WINAPI Receiver(LPVOID arg)
{
	// ������ ��ſ� ����� ����
	int retval;

	// ������ �ޱ�
	while (1)
	{
		strncpy(chatbuf, "", strlen(chatbuf));

		retval = recv(sock, chatbuf, BUFSIZE + 1, 0);
		if (retval == SOCKET_ERROR) {
			err_display(const_cast<char*>("recv()"));
			break;
		}
		else if (retval == 0)
			break;

		if (strncmp(chatbuf, "ERROR", 5) == 0) {
			closesocket(sock);
			DisplayText(const_cast<char*>(
				"---------------- ���ӽ��� : �̹� �����ϴ� ��ȭ�� �Դϴ�. ----------------"));
			cnt = false;
		}
		else
		{
			strcpy(buf, chatbuf);
			DisplayText(buf);
			cnt = true;
		}

		SetEvent(hReadEvent); // �б� �Ϸ� �˸���
	}
	return 0;
}

int start_chatting()
{
	int retval;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit(const_cast<char*>("socket()"));

	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(IP);
	serveraddr.sin_port = htons(PORT);
	retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit(const_cast<char*>("connect()"));

	char temp[23] = ""; // �̸��� ���ȣ
	strcpy(temp, prevNick);
	strcat(temp, "/");
	if (chk1)
		strcat(temp, "1");
	else
		strcat(temp, "2");

	retval = sendto(sock, temp, strlen(temp), 0,
		(SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) {
		err_display(const_cast<char*>("sendto()"));
	}

	// ���ù� ������ ����
	HANDLE hThread = CreateThread(NULL, 0, Receiver,
		(LPVOID)sock, 0, NULL);
	if (hThread == NULL) { closesocket(sock); }
	else { CloseHandle(hThread); }
	return 1;
}

int isNumber(char* port)
{
	int i;
	for (i = 0; i < strlen(port); i++)
	{
		if (port[i] < '0' || port[i] > '9') return 0;
	}
}

int end_chatting()
{
	// closesocket()
	closesocket(sock);

	// ���� ����
	WSACleanup();
	return 0;
}