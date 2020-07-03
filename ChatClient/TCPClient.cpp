// 2020년 1학기 네트워크프로그래밍 숙제 3번
// 성명: 이민영 학번: 16011100
// 플랫폼: VS2019

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

// 대화상자 프로시저
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

void DisplayText(char* fmt, ...);	// 편집 컨트롤 출력 함수
void err_quit(char* msg);			// 오류 출력 함수
void err_display(char* msg);		// 오류 출력 함수

int isNumber(char* string);			// 포트번호 검사 

int start_chatting();				// 채팅 시작 함수
void setchatIPnPORT(SOCKADDR_IN);	// 채팅 텍스트 헤더 설정 함수
int end_chatting();					// 채팅 종료 함수

time_t t;		// 시간 변수
struct tm tm;	// 시간 구조체

char IP[16];			// IP주소
int  PORT;				// 포트번호
char buf[BUFSIZE + 1];
char chatbuf[BUFSIZE + 1];
BOOL cnt = true, chk1, chk2;

char prevNick[20], newNick[20]; // 대화명 버퍼

HWND hEnterButton;				// 입장 버튼
HWND hExitButton;				// 나가기 버튼
HWND hSendButton;				// 보내기 버튼
HWND hEdit1, hEdit2;			// 편집 컨트롤
HWND hChk1, hChk2;				// 체크박스

HANDLE hReadEvent, hWriteEvent;


SOCKET sock;
SOCKADDR_IN remoteaddr;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	// 이벤트 생성
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if (hReadEvent == NULL) return 1;
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hWriteEvent == NULL) return 1;

	// 대화상자 생성
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// 이벤트 제거
	CloseHandle(hReadEvent);
	CloseHandle(hWriteEvent);

	return 0;
}

// 대화상자 프로시저
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// 한 칸이라도 비워져 있으면 connect 비활성화 
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
		EnableWindow(hEdit1, FALSE);	// connect 하기 전에 채팅 입력창 비활성화
		EnableWindow(hExitButton, FALSE);	// connect 하기 전에 나가기버튼 비활성화
		SendMessage(hEdit1, EM_SETLIMITTEXT, BUFSIZE, 0);	// 최대 입력 텍스트 제한
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ENTER:
			GetDlgItemText(hDlg, IDC_IP, IP, 16);	// IP 주소 ( char* )
			GetDlgItemText(hDlg, IDC_PORT, buf, BUFSIZE + 1);		// 포트 번호 ( char* )
			GetDlgItemText(hDlg, IDC_NICK, prevNick, 20);		// 대화명

			PORT = atoi(buf);


			if (!isNumber(buf))
			{
				SetDlgItemText(hDlg, IDC_ERROR, "[ error ] 0 ~ 65535 사이의 포트번호를 입력해주세요.");
			}
			else if (PORT < 0 || PORT > 65535) // 불가능한 포트 번호
			{
				SetDlgItemText(hDlg, IDC_ERROR, "[ error ] 0 ~ 65535 사이의 포트번호를 입력해주세요.");
			}
			else if (strcmp(prevNick, "ERROR") == 0)
			{
				SetDlgItemText(hDlg, IDC_ERROR, "대화명으로 ERROR는 사용할 수 없습니다.");
			}
			else
			{
				chk1 = IsDlgButtonChecked(hDlg, IDC_ROOM1) ? TRUE : FALSE;
				chk2 = IsDlgButtonChecked(hDlg, IDC_ROOM2) ? TRUE : FALSE;

				if (chk1)
				{
					EnableWindow(GetDlgItem(hDlg, IDC_IP), FALSE);	// ip 변경 불가
					EnableWindow(GetDlgItem(hDlg, IDC_PORT), FALSE);		// 포트번호 변경 불가
					EnableWindow(GetDlgItem(hDlg, IDC_ENTER), FALSE);

					start_chatting();										// 채팅 시작

					EnableWindow(hEdit1, TRUE);

					SetDlgItemText(hDlg, IDC_ERROR, "");

					EnableWindow(hChk1, FALSE);
					EnableWindow(hChk2, FALSE);
				}
				else if (chk2)
				{
					EnableWindow(GetDlgItem(hDlg, IDC_IP), FALSE);	// ip 변경 불가
					EnableWindow(GetDlgItem(hDlg, IDC_PORT), FALSE);		// 포트번호 변경 불가

					start_chatting();										// 채팅 시작

					EnableWindow(hEdit1, TRUE);
					EnableWindow(hEnterButton, FALSE);

					SetDlgItemText(hDlg, IDC_ERROR, "");

					EnableWindow(hChk1, FALSE);
					EnableWindow(hChk2, FALSE);
				}
				else
				{
					SetDlgItemText(hDlg, IDC_ERROR, "[ error ] 채팅방을 선택하세요.");
				}
			}
			return TRUE;
		case IDC_SEND:
			/* --------------------------------------------------------------------- 채팅 전송 --------------------------------------------------------------------- */
			SOCKADDR_IN peeraddr;
			int addrlen;
			int retval;

			if (cnt == false)
			{
				SetDlgItemText(hDlg, IDC_ERROR, "[ error ] 채팅방에 접속하세요.");
				SetDlgItemText(hDlg, IDC_CHAT, (LPCSTR)NULL);	// 채팅창 초기화
				EnableWindow(hChk1, TRUE);
				EnableWindow(hChk2, TRUE);
			}
			else
			{
				EnableWindow(hSendButton, FALSE);						// 보내기 버튼 비활성화 : 더블클릭 방지
				WaitForSingleObject(hReadEvent, INFINITE);				// 읽기 완료 기다리기

				strncpy(chatbuf, "", strlen(chatbuf));
				strncpy(buf, "", strlen(buf));

				GetDlgItemText(hDlg, IDC_CHAT, chatbuf, BUFSIZE + 1);	// 입력한 채팅

				if (strcmp(chatbuf, "접속자") == 0)
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

				SetEvent(hWriteEvent);							// 쓰기 완료 알리기			

				SendMessage(hEdit1, EM_SETSEL, 0, -1);
				SetDlgItemText(hDlg, IDC_CHAT, (LPCSTR)NULL);	// 채팅창 초기화
				SetFocus(hEdit1);
				EnableWindow(hSendButton, TRUE);				// 보내기 버튼 활성화
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

// 편집 컨트롤 출력 함수
void DisplayText(char* fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[BUFSIZE + 256];
	vsprintf(cbuf, fmt, arg);
	strcat(cbuf, "\r\n");
	int nLength = GetWindowTextLength(hEdit2);
	SendMessage(hEdit2, EM_SETSEL, nLength, nLength);	 		// 현재 채팅 뒤에 이어 붙이기
	SendMessage(hEdit2, EM_REPLACESEL, FALSE, (LPARAM)cbuf);

	va_end(arg);
}

// 소켓 함수 오류 출력 후 종료
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

// 소켓 함수 오류 출력
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

// 데이터 받는 쓰레드 함수
DWORD WINAPI Receiver(LPVOID arg)
{
	// 데이터 통신에 사용할 변수
	int retval;

	// 데이터 받기
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
				"---------------- 접속실패 : 이미 존재하는 대화명 입니다. ----------------"));
			cnt = false;
		}
		else
		{
			strcpy(buf, chatbuf);
			DisplayText(buf);
			cnt = true;
		}

		SetEvent(hReadEvent); // 읽기 완료 알리기
	}
	return 0;
}

int start_chatting()
{
	int retval;

	// 윈속 초기화
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

	char temp[23] = ""; // 이름과 방번호
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

	// 리시버 스레드 생성
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

	// 윈속 종료
	WSACleanup();
	return 0;
}