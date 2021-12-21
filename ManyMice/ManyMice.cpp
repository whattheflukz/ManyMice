#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#include <cstdio>
#include <iostream>
#include <vector>
#include <conio.h>

struct mouse {
	HANDLE device; //memory address to the device ?
	bool ignore;
	long x = 0;
	long y = 0;
	int wheel = 0;
};

std::vector<mouse> mcs; //vector containing all mousing devices
std::vector<char> m_RawInputMessageData; // Buffer

LRESULT CALLBACK EventHandler(HWND, unsigned, WPARAM, LPARAM); // Window message callback.

void ClearConsole() {
	static const COORD top_left = { 0, 0 };
	CONSOLE_SCREEN_BUFFER_INFO info;
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(console, &info);
	DWORD written, cells = info.dwSize.X * info.dwSize.Y;
	FillConsoleOutputCharacter(console, ' ', cells, top_left, &written);
	SetConsoleCursorPosition(console, top_left);
}

int main() {
	ClearConsole();
	HINSTANCE instance = GetModuleHandle(0);
	std::ios_base::sync_with_stdio(false);
	
	// Get console window:
	FILE* console_output;
	FILE* console_error;
	int x = FreeConsole();
	if (AllocConsole()) {
		freopen_s(&console_output, "CONOUT$", "w", stdout);
		freopen_s(&console_error, "CONERR$", "w", stderr);
	} else {
		DWORD d = GetLastError();
		std::cout << d << 'n';
		return -1;
	}
	const char* class_name = "SimpleEngine Class"; // Create message-only window:
	WNDCLASS window_class = {NULL,EventHandler,NULL,NULL,instance,NULL,NULL,NULL,NULL,class_name}; 
	if (!RegisterClass(&window_class)) return -1;
	HWND window = CreateWindow(class_name, "SimpleEngine", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0);
	if (window == nullptr) return -1;
	//Window done

	//register device and enter event loop
	RAWINPUTDEVICE rid = { 0x01, 0x02, RIDEV_INPUTSINK, window };
	RegisterRawInputDevices(&rid, 1, sizeof rid);
	std::vector<std::wstring> mice;
	printf("enter any key when ready, then move the mouse that will be ignored ");
	char temp_char = getchar();
	MSG event;
	while (true) {
		bool tmp_b = PeekMessage(&event, window, 0, 0, PM_REMOVE);//tmp_b determines if there are unread messages in the queue
		if (event.message == WM_QUIT) break;
		if (tmp_b != 0) {
			TranslateMessage(&event);
			DispatchMessage(&event);
		}
	}	
	//program finished
	fclose(console_output);
	fclose(console_error);
	return 0;
}

void addMouse(HANDLE h) {
	mouse m;
	char temp_c = NULL;
	ClearConsole();
	printf("new device detected, adding to list!\nWould you like to ignore this devices input? (y)es (n)o\n");
	while (((temp_c = getchar()) != EOF) && ((temp_c != 'y') && (temp_c != 'n'))) putchar(temp_c);
	m.ignore = (temp_c == 'y') ? true : false;
	m.device = h;
	mcs.push_back(m);
	printf("new device :  0x%08X added to list!\n", m.device);
	Sleep(2000);
	ClearConsole();
	return;
}

LRESULT CALLBACK EventHandler( HWND hwnd, unsigned event, WPARAM wparam, LPARAM lparam ) {
	switch (event) {
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_INPUT: {
			//getting rawinputdata from winders
			unsigned size;
			GetRawInputData((HRAWINPUT)lparam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
			static RAWINPUT raw[sizeof(RAWINPUT)];
			GetRawInputData((HRAWINPUT)lparam, RID_INPUT, raw, &size, sizeof(RAWINPUTHEADER));

			if (raw->header.dwType == RIM_TYPEMOUSE) {
				if (mcs.size() == 0) addMouse(raw->header.hDevice);
				else {
					for (int i = 0; i < mcs.size(); i++) {
						if (mcs[i].device == raw->header.hDevice) {
							//device found in list 
							if (mcs[i].ignore) break; //we want to ignore this mouse and let the OS handle it, it's likely the main mousing device
							else {
								mcs[i].x += raw->data.mouse.lLastX;
								mcs[i].y += raw->data.mouse.lLastY;
								//print xy of all mice, do stuff with event here
								for (int j = 0; j < mcs.size(); j++) wprintf(L"[0x%08X, x : %ld, y : %ld]", mcs[j].device, mcs[j].x, mcs[j].y); 
								printf("\n");
								break;
							}
						}
						else if (i == mcs.size() - 1) addMouse(raw->header.hDevice);
					}
				}
			}
		} 
		return 0;
	}
	// Run default message processor for any missed events:
	return DefWindowProc(hwnd, event, wparam, lparam);
}