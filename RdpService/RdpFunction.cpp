#include "RdpService.h"

#include "base.h"

std::string ServiceClient::get_drive_list() {
		TCHAR szDrives[MAX_PATH] = { 0 };
		DWORD szLength = GetLogicalDriveStrings(MAX_PATH, szDrives);
		if (szLength) {
			std::wstring wlist(szDrives, szLength);
			return std::string(wlist.begin(), wlist.end());
		}
		return {};
}

std::string ServiceClient::get_folder_list(std::string path) {	
	std::stringstream folder_list;
	boost::filesystem::path p(path);
	if (boost::filesystem::exists(p) && boost::filesystem::is_directory(p)) {
		boost::filesystem::directory_iterator end_iter;
		for (boost::filesystem::directory_iterator dir_itr(p); dir_itr != end_iter; ++dir_itr) {
			try {				
				// get timestamp string
				std::time_t timestamp = boost::filesystem::last_write_time(dir_itr->path());
				char timestrbuff[20];
				strftime(timestrbuff, 20, "%Y/%m/%d %H:%M:%S", localtime(&timestamp));				
				// get permission string
				std::string attr;
				boost::filesystem::perms perm = dir_itr->status().permissions();
				(perm & boost::filesystem::owner_read) ? attr.append("r") : attr.append("-");
				(perm & boost::filesystem::owner_write) ? attr.append("w") : attr.append("-");
				(perm & boost::filesystem::owner_exe) ? attr.append("x") : attr.append("-");
				// record direntry info
				folder_list << dir_itr->path().filename() << FILED_DELIMETER;					// name
				if (boost::filesystem::is_directory(dir_itr->status())) folder_list << "DIR" << FILED_DELIMETER << "0" << FILED_DELIMETER; // type & size
				else folder_list << "FIL" << FILED_DELIMETER << boost::filesystem::file_size(dir_itr->path()) << FILED_DELIMETER;
				folder_list << timestrbuff << FILED_DELIMETER << attr << HTTP_CRLF_MARK;		// date & attr
			}
			catch (const std::exception & ex) {
				NOTIFY_MESSAGE(ex.what());
			}
		}
	}	
	return folder_list.str();
}


// Add an Element to the Command Stack
struct CommandList* ServiceClient::Add_Command(struct CommandList* pNode, struct CommandDS Command) {
	// Add an Item at the End of the List
	if (pNode->pNext = (struct CommandList *)malloc(sizeof(struct CommandList))) {
		pNode = pNode->pNext;
		strcpy(pNode->Command.szElement, Command.szElement);
		pNode->pNext = NULL;
		return pNode;
	}

	return NULL;
}

// Completely clear the CommandList of Commands
void ServiceClient::Clear_Command(struct CommandList* pStart) {
	struct	CommandList	*pPrev;
	struct	CommandList	*pNode;
	while (pNode = pStart->pNext) {
		pPrev = pStart;
		pPrev->pNext = pNode->pNext;
		free(pNode);
	}
}

//Keyboard and Mouse Control
void ServiceClient::DispatchWMMessage(char *szString) {
	// Structure to Hold String Rep's of WM_MOUSE MESSAGES
	struct { char *szWMMouseMsg; }
	WMMouseMsg[] = { "WM_MM","WM_LBD","WM_LBU","WM_LBK",
		"WM_MBD","WM_MBU","WM_MBK",
		"WM_RBD","WM_RBU","WM_RBK" };

	// Structure to Hold String Rep's of WM_KEY MESSAGES
	struct { char *szWMKeyBdMsg; }
	WMKeyBdMsg[] = { "WM_KD","WM_KU" };

	int		nWMMouseMsg;
	int		nWMKeyBdMsg;
	
	struct	CommandList	CommandStart;
	struct	CommandList	*pCommandNode;
	struct	CommandDS	Command;
	char	*pDest;
	int		iLoc, nChar;
	int		iLoop, iParms;
	char	szString2[2049];

	// Get the Number of Mouse Messages 
	nWMMouseMsg = (int)(sizeof(WMMouseMsg) / sizeof(WMMouseMsg[0]));

	// Get the Number of Keyboard Messages
	nWMKeyBdMsg = (int)(sizeof(WMKeyBdMsg) / sizeof(WMKeyBdMsg[0]));

	// Initialize the Command List
	CommandStart.pNext = NULL;
	pCommandNode = &CommandStart;

	// Parse the Command String
	iParms = 0;
	while (pDest = strchr(szString, ';')) {
		// Parse the Command
		iLoc = pDest - szString;
		nChar = iLoc;
		memset(Command.szElement, '\0', sizeof(Command.szElement));
		strncpy(Command.szElement, szString, nChar);

		// Add the Command to the Command Stack
		pCommandNode = Add_Command(pCommandNode, Command);

		// Update the Command String by Removing Processed Parameter
		memset(szString2, '\0', sizeof(szString2));
		strcpy(szString2, &szString[iLoc + 1]);
		strcpy(szString, szString2);

		// Increment the Parameters Processed
	}

	// Process the Command
	pCommandNode = CommandStart.pNext;
		// Mouse Messages
	
	int		iMessage;
	int		fWMMouseMsg;
	double	iX, iY , iWidth , iHeight , tWidth,tHeight;
	DWORD	dwX, dwY;

	// Key Board Messages
	int		fWMKeyBdMsg;
	UINT	vk , sk;

	DISPLAY_DEVICE display;
	DEVMODE devmode;

	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
	//Get the resolution of this display
	tWidth = devmode.dmPelsWidth;
	tHeight = devmode.dmPelsHeight;
	// Process the Mouse Window Events
		
	while (pCommandNode) {
		fWMMouseMsg = FALSE;

		for (iLoop = 0; iLoop < nWMMouseMsg; iLoop++) {
			// Check for a Mouse Message
			if (strcmp(pCommandNode->Command.szElement, WMMouseMsg[iLoop].szWMMouseMsg) == 0)
			{
				// Set the Mouse Message Flag
				fWMMouseMsg = TRUE;
				
				iMessage = iLoop + 1;
				// Move to the X Coordinate Element for a Mouse Message
				pCommandNode = pCommandNode->pNext;

				// Get the X Coordinate
				iX = atof(pCommandNode->Command.szElement);

				// Move to the Y Coordinate Element for a Mouse Message
				pCommandNode = pCommandNode->pNext;

				// Get the Y Coordinate
				iY = atof(pCommandNode->Command.szElement);

				// Move to the Width Element for a Mouse Message
				pCommandNode = pCommandNode->pNext;

				// Get the Width
				iWidth = atof(pCommandNode->Command.szElement);

				// Move to the Height Element for a Mouse Message
				pCommandNode = pCommandNode->pNext;

				// Get the Height 
				iHeight = atof(pCommandNode->Command.szElement);
			
				// Break out of the Loop
				break;
			}

		}

		// Check for Dispatching a Mouse Message
		if (fWMMouseMsg) {

			iX = iX * (tWidth / iWidth);
			iY = iY * (tHeight / iHeight);
			// Convert to DWORDS
			iX = iX * 65535.0 / (tWidth - 1);
			iY = iY * 65535.0 / (tHeight - 1);
			dwX = (DWORD)iX;
			dwY = (DWORD)iY;

			// Dispatch the Mouse Message
			
			if (iMessage == 1) // Mouse Move
			{
				NOTIFY_MESSAGE("WM_MM" + std::to_string(dwX) + ";" + std::to_string(dwY) + "\n");

				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, dwX, dwY, 0, 0);
			}
			else if (iMessage == 2) // Left Button Down
			{
				NOTIFY_MESSAGE("WM_LBD" + std::to_string(dwX) + ";" + std::to_string(dwY) + "\n");
				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, dwX, dwY, 0, 0);

				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, dwX, dwY, 0, 0);
			}
			else if (iMessage == 3) // Left Button Up
			{
				NOTIFY_MESSAGE("WM_LBU" + std::to_string(dwX) + ";" + std::to_string(dwY) + "\n");
				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, dwX, dwY, 0, 0);

				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP, dwX, dwY, 0, 0);
			}
			else if (iMessage == 4) // Left Button DoubleClick
			{
				NOTIFY_MESSAGE("WM_LBK" + std::to_string(dwX) + ";" + std::to_string(dwY) + "\n");
				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, dwX, dwY, 0, 0);

				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, dwX, dwY, 0, 0);
				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP, dwX, dwY, 0, 0);
				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, dwX, dwY, 0, 0);
				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP, dwX, dwY, 0, 0);
			}
			else if (iMessage == 5) // Middle Button Down
			{
				NOTIFY_MESSAGE("WM_MBD" + std::to_string(dwX) + ";" + std::to_string(dwY) + "\n");
				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, dwX, dwY, 0, 0);

				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MIDDLEDOWN, dwX, dwY, 0, 0);
			}
			else if (iMessage == 6) // Middle Button Up
			{
				NOTIFY_MESSAGE("WM_MBU" + std::to_string(dwX) + ";" + std::to_string(dwY) + "\n");
				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, dwX, dwY, 0, 0);

				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MIDDLEUP, dwX, dwY, 0, 0);
			}
			else if (iMessage == 7) // Middle Button DoubleClick
			{
				NOTIFY_MESSAGE("WM_MBK" + std::to_string(dwX) + ";" + std::to_string(dwY) + "\n");
				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, dwX, dwY, 0, 0);

				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MIDDLEDOWN, dwX, dwY, 0, 0);
				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MIDDLEUP, dwX, dwY, 0, 0);
				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MIDDLEDOWN, dwX, dwY, 0, 0);
				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MIDDLEUP, dwX, dwY, 0, 0);
			}
			else if (iMessage == 8) // Right Button Down
			{
				NOTIFY_MESSAGE("WM_RBD" + std::to_string(dwX) + ";" + std::to_string(dwY) + "\n");
				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, dwX, dwY, 0, 0);

				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTDOWN, dwX, dwY, 0, 0);
			}
			else if (iMessage == 9) // Right Button Up
			{
				NOTIFY_MESSAGE("WM_RBU" + std::to_string(dwX) + ";" + std::to_string(dwY) + "\n");
				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, dwX, dwY, 0, 0);

				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTUP, dwX, dwY, 0, 0);
			}
			else if (iMessage == 10) // Right Button DoubleClick
			{
				NOTIFY_MESSAGE("WM_RBK" + std::to_string(dwX) + ";" + std::to_string(dwY) + "\n");
				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, dwX, dwY, 0, 0);

				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTDOWN, dwX, dwY, 0, 0);
				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTUP, dwX, dwY, 0, 0);
				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTDOWN, dwX, dwY, 0, 0);
				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTUP, dwX, dwY, 0, 0);
			}
		}
		else
		{
			// Process the KeyBoard Events
			fWMKeyBdMsg = FALSE;
			for (iLoop = 0; iLoop < nWMKeyBdMsg; iLoop++)
			{
				// Check for a KeyBoard Message
				if (strcmp(pCommandNode->Command.szElement, WMKeyBdMsg[iLoop].szWMKeyBdMsg) == 0)
				{
					// Set the KeyBoard Message Flag
					fWMKeyBdMsg = TRUE;

					// Set the Message to Post
					if (strcmp(WMKeyBdMsg[iLoop].szWMKeyBdMsg, "WM_KD\0") == 0)
						iMessage = 1;
					else if (strcmp(WMKeyBdMsg[iLoop].szWMKeyBdMsg, "WM_KU\0") == 0)
						iMessage = 2;

					// Move to the VK Element for a Key Board Message
					pCommandNode = pCommandNode->pNext;

					// Get the Virtual Key Code
					vk = atoi(pCommandNode->Command.szElement);

					// Move to the SK Element for a Key Board Message
					pCommandNode = pCommandNode->pNext;

					// Get the Down Scan code
					sk = atoi(pCommandNode->Command.szElement);
				
					break;
				}
			}

			// Check for Dispatching a Key Board Message
			if (fWMKeyBdMsg)
			{
				// Dispatch the Key Board Message
				if (iMessage == 1) // Key Down
				{
					keybd_event((BYTE)vk, (BYTE)sk, 0, 0);
				}
				else if (iMessage == 2) // Key Up
				{
					keybd_event((BYTE)vk, (BYTE)sk, KEYEVENTF_KEYUP, 0);
				}
			}

		}
		pCommandNode = pCommandNode->pNext;
	}

	Clear_Command(&CommandStart);
}

