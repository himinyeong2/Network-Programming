// 2020년 1학기 네트워크프로그래밍 숙제 3번
// 성명: 이민영 학번: 16011100
// 플랫폼: VS2019

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERPORT	9000
#define BUFSIZE		512

// 소켓 정보 저장을 위한 구조체와 변수
struct SOCKETINFO
{
	SOCKET sock;
	char buf[BUFSIZE + 1];
	int recvbytes;
	int sendbytes;
	SOCKADDR_IN ip;
};

struct CHATMANAGER
{
	SOCKETINFO* SocketInfoArray[FD_SETSIZE];
	char nicknames[20][21]; // 최대 20명
	int nTotalSockets = 0;
};

CHATMANAGER chatRoom[2];

int ch;

// 소켓 관리 함수
BOOL AddSocketInfo(SOCKET sock);
void RemoveSocketInfo(int nIndex);

// 오류 출력 함수
void err_quit(char* msg);
void err_display(char* msg);

int main(int argc, char* argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit(const_cast<char*>("socket()"));

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit(const_cast<char*>("bind()"));

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit(const_cast<char*>("listen()"));

	// 넌블로킹 소켓으로 전환
	u_long on = 1;
	retval = ioctlsocket(listen_sock, FIONBIO, &on);
	if (retval == SOCKET_ERROR) err_display(const_cast<char*>("ioctlsocket()"));

	// 데이터 통신에 사용할 변수
	FD_SET rset, wset;
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen, i, j;

	while (1) {
		// 소켓 셋 초기화
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(listen_sock, &rset);
		for (i = 0; i < chatRoom[ch].nTotalSockets; i++) {
			if (chatRoom[ch].SocketInfoArray[i]->recvbytes > chatRoom[ch].SocketInfoArray[i]->sendbytes)
				FD_SET(chatRoom[ch].SocketInfoArray[i]->sock, &wset);
			else
				FD_SET(chatRoom[ch].SocketInfoArray[i]->sock, &rset);
		}

		// select()
		retval = select(0, &rset, &wset, NULL, NULL);
		if (retval == SOCKET_ERROR) err_quit(const_cast<char*>("select()"));

		// 소켓 셋 검사(1): 클라이언트 접속 수용
		if (FD_ISSET(listen_sock, &rset)) {
			addrlen = sizeof(clientaddr);
			client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
			if (client_sock == INVALID_SOCKET) {
				err_display(const_cast<char*>("accept()"));
			}

			char temp[23] = "";
			char name[23] = "";
			BOOL dup = FALSE;

			retval = recvfrom(client_sock, temp, 23, 0,
				(SOCKADDR*)&clientaddr, &addrlen);
			if (retval == SOCKET_ERROR) {
				err_display(const_cast<char*>("recvfrom()"));
				continue;
			}
			strcpy(name, temp);
			strtok(name, "/");

			if (temp[strlen(name) + 1] == '1')
				ch = 0;
			else
				ch = 1;

			for (int i = 0; i < chatRoom[ch].nTotalSockets; i++)
			{
				if (strcmp(name, chatRoom[ch].nicknames[i]) == 0) {
					dup = TRUE;
					break;
				}
			}
			if (dup) //중복
			{
				retval = sendto(client_sock, "ERROR", 6, 0,
					(SOCKADDR*)&clientaddr, addrlen);
				if (retval == SOCKET_ERROR) {
					err_display(const_cast<char*>("sendto()"));
				}
			}
			else {
				printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
					inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

				// 소켓 정보 추가
				strncpy(chatRoom[ch].nicknames[chatRoom[ch].nTotalSockets], "", 21);
				strcpy(chatRoom[ch].nicknames[chatRoom[ch].nTotalSockets], name); // 사용자 추가
				AddSocketInfo(client_sock);
			}

		}

		// 소켓 셋 검사(2): 데이터 통신
		for (i = 0; i < chatRoom[ch].nTotalSockets; i++) {
			SOCKETINFO* ptr = chatRoom[ch].SocketInfoArray[i];
			if (FD_ISSET(ptr->sock, &rset)) {
				// 데이터 받기
				retval = recv(ptr->sock, ptr->buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR) {
					err_display(const_cast<char*>("recv()"));
					RemoveSocketInfo(i);
					continue;
				}
				else if (retval == 0) {
					RemoveSocketInfo(i);
					continue;
				}
				ptr->recvbytes = retval;
				// 받은 데이터 출력
				addrlen = sizeof(clientaddr);
				getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);
				ptr->buf[retval] = '\0';

				if (strcmp(ptr->buf, "접속자") == 0)
				{
					char people[BUFSIZE + 1] = "접속자 :";
					// 채팅방 1 접속자
					for (int k = 0; k < chatRoom[0].nTotalSockets; k++) {
						strcat(people, " ");
						strcat(people, chatRoom[0].nicknames[k]);
					}
					strcat(people, " /");
					// 채팅방 2 접속자
					for (int k = 0; k < chatRoom[1].nTotalSockets; k++) {
						strcat(people, " ");
						strcat(people, chatRoom[1].nicknames[k]);
					}
					retval = send(ptr->sock, people, strlen(people), 0);
					if (retval == SOCKET_ERROR) {
						err_display(const_cast<char*>("send()"));
					}
					strncpy(ptr->buf, "", strlen(ptr->buf));
					printf("%s\n", people);

				}
				else
				{
					printf("[TCP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr),
						ntohs(clientaddr.sin_port), ptr->buf);
				}
			}
			if (FD_ISSET(ptr->sock, &wset)) {
				// 데이터 보내기
				for (j = 0; j < chatRoom[ch].nTotalSockets; j++) {  // 여러 접속자에게 발송
					SOCKETINFO* sptr = chatRoom[ch].SocketInfoArray[j];
					retval = send(sptr->sock, ptr->buf + ptr->sendbytes,
						ptr->recvbytes - ptr->sendbytes, 0);

					if (retval == SOCKET_ERROR) {
						err_display(const_cast<char*>("send()"));
						RemoveSocketInfo(i);
						continue;
					}
				}
				ptr->sendbytes += retval;
				if (ptr->recvbytes == ptr->sendbytes) {
					ptr->recvbytes = ptr->sendbytes = 0;
				}



			}
		}
	}

	// 윈속 종료
	WSACleanup();
	return 0;
}

// 소켓 정보 추가
BOOL AddSocketInfo(SOCKET sock)
{
	if (chatRoom[ch].nTotalSockets >= FD_SETSIZE) {
		printf("[오류] 소켓 정보를 추가할 수 없습니다!\n");
		return FALSE;
	}

	SOCKETINFO* ptr = new SOCKETINFO;
	if (ptr == NULL) {
		printf("[오류] 메모리가 부족합니다!\n");
		return FALSE;
	}

	ptr->sock = sock;
	ptr->recvbytes = 0;
	ptr->sendbytes = 0;
	chatRoom[ch].SocketInfoArray[chatRoom[ch].nTotalSockets++] = ptr;

	return TRUE;
}

// 소켓 정보 삭제
void RemoveSocketInfo(int nIndex)
{
	SOCKETINFO* ptr = chatRoom[ch].SocketInfoArray[nIndex];

	// 클라이언트 정보 얻기
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);
	printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
		inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

	closesocket(ptr->sock);
	delete ptr;

	if (nIndex != (chatRoom[ch].nTotalSockets - 1))
		chatRoom[ch].SocketInfoArray[nIndex] = chatRoom[ch].SocketInfoArray[chatRoom[ch].nTotalSockets - 1];

	chatRoom[ch].nTotalSockets -= 1;

	for (int i = nIndex; i < chatRoom[ch].nTotalSockets; i++) {
		strcpy(chatRoom[ch].nicknames[i], chatRoom[ch].nicknames[i + 1]);
	}
	strncpy(chatRoom[ch].nicknames[chatRoom[ch].nTotalSockets], "", strlen(chatRoom[ch].nicknames[chatRoom[ch].nTotalSockets]));

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