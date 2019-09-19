#include "RdpService.h"
#include "base.h"

ServiceClient::ServiceClient(boost::asio::io_service& io_service, boost::asio::ssl::context& context, boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
		: m_socket(io_service, context),
	      m_interval(1), 
	      m_ping_timer(io_service, m_interval) ,
		  m_non_empty_output_queue_(io_service),
		  m_screen_send_timer(io_service, boost::posix_time::milliseconds(100))
{

	memset(&m_ping_buff[0], 0, SERVICE_PACKET_OVERHEAD);	

	// The m_non_empty_output_queue_ deadline_timer is set to pos_infin whenever
	// the output queue is empty. This ensures that the output actor stays
	// asleep until a message is put into the queue.
	m_non_empty_output_queue_.expires_at(boost::posix_time::pos_infin);

	boost::asio::async_connect(m_socket.lowest_layer(), endpoint_iterator
		, boost::bind(&ServiceClient::handle_connect, this, boost::asio::placeholders::error));
}

void ServiceClient::handle_connect(const boost::system::error_code& error) {
	if (!error) m_socket.async_handshake(boost::asio::ssl::stream_base::client
		, boost::bind(&ServiceClient::handle_handshake, this, boost::asio::placeholders::error));
	else stop();
}

void ServiceClient::handle_handshake(const boost::system::error_code& error) {
	if (!error) {
		// after handshaking established, send verification request 
		char hostname[128] = { 0 };
		gethostname(&hostname[0], 128);
		char request_buff[BUFFER_SIZE];
		sprintf_s(request_buff, BUFFER_SIZE, "GET /service?uuid=%s %s@%s HTTP/1.1\r\nHost: intelstar.rdp.com\r\n\r\n", hostname, __DATE__, __TIME__);
		
		m_output_queue_.push_back(request_buff);  // put request_buff into output_queue

		m_ping_timer.async_wait(boost::bind(&ServiceClient::handle_ping_timer, this)); // start ping timer
		
		start_read();  // start reading

		await_output(); // start writing

	}
	else stop();
}

void ServiceClient::deliver(const std::string& msg) {		// put message into output_queue and awake m_non_empty_output_queue
	m_output_queue_.push_back(msg);
	m_non_empty_output_queue_.expires_at(boost::posix_time::neg_infin);
}

void ServiceClient::handle_ping_timer() {		// ping timer
	NOTIFY_MESSAGE("Ping Send");
	deliver(m_ping_buff);
	m_ping_timer.expires_from_now(boost::posix_time::seconds(m_interval));
	m_ping_timer.async_wait(boost::bind(&ServiceClient::handle_ping_timer, this));
}

void ServiceClient::start_read() {
	// read data to m_read_buff
	m_socket.async_read_some(boost::asio::buffer(m_read_buff, BUFFER_SIZE),
		boost::bind(&ServiceClient::handle_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void ServiceClient::handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
	if (stopped())
		return;
	if (!error) {
		m_recv_buffer.insert(m_recv_buffer.end(), m_read_buff, m_read_buff + bytes_transferred);
		while (m_recv_buffer.size() >= SERVICE_PACKET_OVERHEAD) {
			char* pBuffer = (char *)m_recv_buffer.data();
			size_t length = m_recv_buffer.size();
			SERVICE_PACKET * request = reinterpret_cast<SERVICE_PACKET*> (pBuffer);
			int packetLength = request->service_data.packet_size + SERVICE_PACKET_OVERHEAD;
			if (length < request->service_data.packet_size + SERVICE_PACKET_OVERHEAD) {
				break;
			}
			// process one packet
			ProcessPacket(request);
			// remove packet from recv_buffer
			m_recv_buffer.erase(m_recv_buffer.begin(), m_recv_buffer.begin() + packetLength);
		}
		start_read();
	}
	else {
		NOTIFY_MESSAGE("handle_read error:" + error.message());
		stop();
	}
}

void ServiceClient::ProcessPacket(SERVICE_PACKET * request) {
	char* pContent = (char*)request + SERVICE_PACKET_OVERHEAD;
	
	if (request->service_data.packet_type == PACKET_TYPE_PNG_SVR) {} // PING packet arrives
	else if (request->service_data.packet_type == PACKET_TYPE_DRV_LST) {
		std::string list = get_drive_list();
		NOTIFY_MESSAGE("Drive List:" + list);
		handle_request(request->browser_sock, PACKET_TYPE_DRV_LST, list.c_str(), list.size());
	}
	else if (request->service_data.packet_type == PACKET_TYPE_FLD_LST) {
		std::string path(pContent, request->service_data.packet_size);
		std::string folders = get_folder_list(path);
		NOTIFY_MESSAGE("Folder List:" + path);
		path.append(HTTP_CRLF_MARK + folders);
		handle_request(request->browser_sock, PACKET_TYPE_FLD_LST, path.c_str(), path.size());
	}
	else if (request->service_data.packet_type == PACKET_TYPE_FLD_CRT) {
		std::string newfolder(pContent, request->service_data.packet_size);
		NOTIFY_MESSAGE("Folder Create:" + newfolder);
		size_t pos = newfolder.rfind("\\");								// split folder pathname
		std::string path = newfolder.substr(0, pos) + HTTP_CRLF_MARK;	// get parent folder name
		std::string folder = newfolder.substr(pos + 1);
		// create folder below the path
		boost::filesystem::path dir(newfolder);
		if (boost::filesystem::create_directory(dir)) path.append(folder + ";DIR;0;" + get_current_timestamp() + ";rw-\r\n");
		handle_request(request->browser_sock, PACKET_TYPE_FLD_CRT, path.c_str(), path.size());
	}
	else if (request->service_data.packet_type == PACKET_TYPE_FIL_GET) {
		// remote_path;local_path;offset
		std::string file_info(pContent, request->service_data.packet_size);
		std::string file_data = file_info + FILED_DELIMETER;
		size_t pos = file_info.find(FILED_DELIMETER);
		std::string remotepath = file_info.substr(0, pos);
		file_info = file_info.substr(pos + 1);
		pos = file_info.find(FILED_DELIMETER);
		std::string localpath = file_info.substr(0, pos);
		long offset = atol(file_info.substr(pos + 1).c_str());
		NOTIFY_MESSAGE("File Get: " + localpath + " to " + remotepath + ":" + std::to_string(offset));
		FILE * fp = fopen(localpath.c_str(), "rb");
		if (fp) {
			if (fseek(fp, offset, SEEK_SET) == -1) NOTIFY_MESSAGE("File Get Seek failed");
			else {
				char buffer[SERVICE_PACKET_MAX_DATA];
				memset(&buffer[0], 0, SERVICE_PACKET_MAX_DATA);
				size_t ret = fread(&buffer[0], 1, SERVICE_PACKET_MAX_DATA, fp);
				// remote_path;local_path;offset;size\r\n{file_data}
				if (ret > 0) file_data.append(std::to_string(ret) + HTTP_CRLF_MARK + std::string(&buffer[0], ret));
				else file_data.append(std::string("0") + HTTP_CRLF_MARK);
			}
			fclose(fp);
			handle_request(request->browser_sock, PACKET_TYPE_FIL_GET, file_data.c_str(), file_data.size());
		}
		else NOTIFY_MESSAGE("File Get:" + localpath + " failed");
	}
	else if (request->service_data.packet_type == PACKET_TYPE_FIL_PUT) {
		// remote_path;local_path;offset;size\r\nfile_data
		std::string file_info(reinterpret_cast<char*> (pContent, request->service_data.packet_size));
		size_t pos = file_info.find(HTTP_CRLF_MARK);
		std::string file_data = file_info.substr(pos + 2);
		file_info = file_info.substr(0, pos);
		pos = file_info.find(FILED_DELIMETER);
		std::string remotepath = file_info.substr(0, pos);
		file_info = file_info.substr(pos + 1);
		pos = file_info.find(FILED_DELIMETER);
		std::string localpath = file_info.substr(0, pos);
		file_info = file_info.substr(pos + 1);
		pos = file_info.find(FILED_DELIMETER);
		long offset = atol(file_info.substr(0, pos).c_str());
		long size = atol(file_info.substr(pos + 1).c_str());
		NOTIFY_MESSAGE("File Put: " + localpath + ":" + std::to_string(offset) + ":" + std::to_string(size));
		if (size > 0 && file_data.size() >= size) {
			if (offset == 0) remove(localpath.c_str());	// if file is already exists, remove it
			FILE *fp = fopen(localpath.c_str(), "ab");
			if (fp) {
				if (fseek(fp, offset, SEEK_SET) == -1) NOTIFY_MESSAGE("File Put Seek failed");
				else {
					size_t ret = fwrite(file_data.c_str(), 1, file_data.size(), fp);
					if (ret > 0) offset += ret;
				}
				fclose(fp);
				file_info = remotepath + FILED_DELIMETER + localpath + FILED_DELIMETER + std::to_string(offset);
				handle_request(request->browser_sock, PACKET_TYPE_FIL_PUT, file_info.c_str(), file_info.size());
			}
			else NOTIFY_MESSAGE("File Put open failed");
		}
		else NOTIFY_MESSAGE("File Put chunk imcompleted");
	}
	else if (request->service_data.packet_type == PACKET_TYPE_MUS_CTL || request->service_data.packet_type == PACKET_TYPE_KBD_CTL)	{
		NOTIFY_MESSAGE("Keyboard and Mouse control");
		DispatchWMMessage(pContent); //Keyboard and Mouse Control
	}
	else if (request->service_data.packet_type == PACKET_TYPE_SCR_GET) {
		NOTIFY_MESSAGE("Screen Capture Start");
		m_screen_capture.InitDisplay();  // split screen into iGridX * iGridY
		m_browserInfo.browser_sock = request->browser_sock;
		m_screen_send_timer.async_wait(boost::bind(&ServiceClient::handle_scr_send, this, boost::asio::placeholders::error));
	}
	else if (request->service_data.packet_type == PACKET_TYPE_SCR_OFF) {
		NOTIFY_MESSAGE("Screen Capture OFF");
		m_screen_send_timer.cancel();
		m_screen_capture.ClearDisplay();
	}
}

//////////////////////////////////////////////////////////////////////////
void ServiceClient::await_output() {
	if (stopped())
		return;
	if (m_output_queue_.empty()) {  	// asleep until a message is put into the queue.
		m_non_empty_output_queue_.expires_at(boost::posix_time::pos_infin);
		m_non_empty_output_queue_.async_wait(
			boost::bind(&ServiceClient::await_output, this));
	}
	else {
		start_write();
	}
}

void ServiceClient::start_write() {
	
	// Start an asynchronous operation to send a message.
	m_socket.async_write_some(boost::asio::buffer(m_output_queue_.front() , m_output_queue_.front().size()),
		boost::bind(&ServiceClient::handle_write, this , boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void ServiceClient::handle_write(const boost::system::error_code& error, size_t bytes_transferred) {
	if (stopped())
		return;
	if (!error) {
		NOTIFY_MESSAGE("handle_write: " + std::to_string(bytes_transferred));
		m_output_queue_.pop_front();
		await_output();
	}
	else {
		stop();
	}
}

void ServiceClient::stop() {
	m_non_empty_output_queue_.cancel();
	m_socket.get_io_service().stop();
}

bool ServiceClient::stopped() const {
	return !m_socket.lowest_layer().is_open();
}

void ServiceClient::handle_request(uint32_t browser_sock, uint16_t type, const char* raw_data, size_t raw_size) {
	if (browser_sock == 0) 
		NOTIFY_MESSAGE("Invalid browser socket");
	else {
		int buf_len = SERVICE_PACKET_OVERHEAD + raw_size;
		const char *buffer = (const char*)malloc(buf_len);
		if (!buffer) {
			NOTIFY_MESSAGE("Insufficient Memory");
			return;
		}
		memset((void*)buffer, 0, buf_len);
		SERVICE_PACKET * header = (SERVICE_PACKET*)buffer;
		header->browser_sock = browser_sock;
		header->service_data.packet_size = (uint32_t)raw_size;
		header->service_data.packet_type = type;
		if (raw_size > 0 && raw_data) 
			memcpy((void*)(buffer + SERVICE_PACKET_OVERHEAD), raw_data, raw_size);
// 		std::string str;
// 		for (int i = 0; i < buf_len; i++)
// 		{
// 			str.push_back(buffer[i]);
// 		}
		std::string str(buffer, buf_len);
		deliver(str);

		free((void*)buffer);
	}
}

std::string ServiceClient::get_current_timestamp() {
	std::time_t timestamp = time(NULL);
	char timestrbuff[20];
	strftime(timestrbuff, 20, "%Y/%m/%d %H:%M:%S", localtime(&timestamp));
	return std::string(timestrbuff);
}

void ServiceClient::handle_scr_send(const boost::system::error_code& error) {	//send screen information

	if (error)
		stop();

	int len;	//length of jpeg compressed image
	std::string file_data; 

	m_screen_capture.Reset();

	while (m_screen_capture.GetCurNode()) {
		GdiList* pGdiNode = m_screen_capture.GetCurNode();

		int fsend = m_screen_capture.GetRegionDisplay();

		if (fsend) {
			char* buffer = m_screen_capture.CaptureAnImage(len);

			file_data.clear();

			int offset = 0;
			while (len) {
				int bytes_to_send = len;

				if (len > SERVICE_PACKET_MAX_DATA) {
					bytes_to_send = SERVICE_PACKET_MAX_DATA;
				}

				file_data.clear();
				file_data.append(std::to_string(pGdiNode->Gdi.iGridX) + FILED_DELIMETER +
					std::to_string(pGdiNode->Gdi.iGridY) + FILED_DELIMETER +
					std::string(buffer + offset, bytes_to_send));
				handle_request(m_browserInfo.browser_sock, PACKET_TYPE_SCR_GET, file_data.c_str(), file_data.size());

				len -= bytes_to_send;
				offset += bytes_to_send;
			}

			delete[] buffer;				
		}

		m_screen_capture.Next();
	}

	m_screen_send_timer.expires_from_now(boost::posix_time::milliseconds(100));
	m_screen_send_timer.async_wait(boost::bind(&ServiceClient::handle_scr_send, this, boost::asio::placeholders::error));
}