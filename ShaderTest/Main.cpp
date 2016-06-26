//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "pch.hpp"
#include "DXManager.hpp"
#include "App.h"
#include "Resources.hpp"

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

App app;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DXProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	// Parse the command line parameters
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	//pSample->ParseCommandLineArgs(argv, argc);
	LocalFree(argv);

	RECT windowRect = { 0, 0, 800, 600 }; //initial rect

	WNDCLASSEX wclass = { 0 };
	wclass.cbSize = sizeof(WNDCLASSEX);
	wclass.style = CS_HREDRAW | CS_VREDRAW;
	wclass.lpfnWndProc = WindowProc;
	wclass.hInstance = hInstance;
	wclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wclass.lpszClassName = L"AppWindowClass";
	wclass.hbrBackground = (HBRUSH)COLOR_WINDOW;
	RegisterClassEx(&wclass);

	HWND hwnd = CreateWindow
	(
		wclass.lpszClassName,
		L"Shader Tester",
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);

	ShowWindow(hwnd, nCmdShow);

	MSG msg = { };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		app.Update();

		DXManager::Begin();
		app.Draw();
		DXManager::End();
	}

	app.Destroy();
	DXManager::Destroy();

	return static_cast<char>(msg.wParam);
}

HWND hDx;
HWND hEdit;
HWND hBtn;

WNDPROC pEditProc;
LRESULT CALLBACK EditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_KEYDOWN)
	{
		bool isCtrl = (GetKeyState(VK_CONTROL) & 0x8000) > 0;
		
		if (isCtrl && wParam == 'A')
			SendMessage(hWnd, EM_SETSEL, 0, -1);
		else if (isCtrl && wParam == VK_RETURN)
		{
			SendMessage(hBtn, BM_CLICK, 0, 0);
			return 0;
		}
	}
	return CallWindowProc(pEditProc, hWnd, msg, wParam, lParam);
}
#define BUTTON_ID 0x8801

UINT sidebarWidth = 400;
UINT margin = 10;
auto buttonHeight = 24;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		{
			RECT wndRect;
			GetClientRect(hWnd, &wndRect);
			HINSTANCE hinst = GetModuleHandle(NULL);
			HGDIOBJ hDefaultFont = GetStockObject(DEFAULT_GUI_FONT);

			//initialize DX render window
			{
				WNDCLASSEX wclass = { 0 };
				wclass.cbSize = sizeof(WNDCLASSEX);
				wclass.style = CS_HREDRAW | CS_VREDRAW;
				wclass.lpfnWndProc = DXProc;
				wclass.hInstance = hinst;
				wclass.hCursor = LoadCursor(NULL, IDC_ARROW);
				wclass.lpszClassName = L"RenderWindowClass";
				RegisterClassEx(&wclass);

				hDx = CreateWindow
				(
					wclass.lpszClassName,
					L"Render Window",
					WS_CHILD | WS_VISIBLE | WS_TABSTOP,
					sidebarWidth,
					0,
					wndRect.right - wndRect.left - sidebarWidth,
					wndRect.bottom - wndRect.top,
					hWnd,
					NULL,
					hinst,
					nullptr
				);

				DXManager::Initialize(hDx);
				app.Initialize();
			}

			{
				HMODULE module = GetModuleHandle(NULL);
				HRSRC rc = FindResource(module, MAKEINTRESOURCE(IDR_TEXTFILE_PSH), MAKEINTRESOURCE(TEXTFILE));
				HGLOBAL rcd = LoadResource(module, rc);
				DWORD sz = SizeofResource(module, rc);
				auto data = LockResource(rcd);
				auto wstr = std::wstring((char*)data, (char*)data + sz);

				hEdit = CreateWindowEx
				(
					WS_EX_CLIENTEDGE,
					L"EDIT",
					wstr.c_str(),
					WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					hWnd,
					NULL,
					hinst,
					nullptr
				);
				FreeResource(rcd);

				//add custom window procedure for additional features
				pEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)&EditProc);

				SendMessage(hEdit, WM_SETFONT, (WPARAM)hDefaultFont, MAKELPARAM(FALSE, 0));
			}

			{
				hBtn = CreateWindowEx
				(
					0,
					L"BUTTON",
					L"Update",
					WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					hWnd,
					(HMENU)BUTTON_ID,
					hinst,
					nullptr
				);
				SendMessage(hBtn, WM_SETFONT, (WPARAM)hDefaultFont, MAKELPARAM(FALSE, 0));

				HWND hTip = CreateWindowEx
				(
					NULL,
					TOOLTIPS_CLASS,
					nullptr,
					WS_POPUP,
					CW_USEDEFAULT, 
					CW_USEDEFAULT,
					CW_USEDEFAULT, 
					CW_USEDEFAULT,
					hBtn,
					NULL,
					hinst,
					nullptr
				);

				TOOLINFO tool = { };
				tool.cbSize = sizeof(tool);
				tool.hwnd = hTip;
				tool.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
				tool.hinst = hinst;
				tool.lpszText = L"You can also use [CTRL+ENTER] in the Edit box to update the shader";
				tool.uId = (UINT_PTR)hBtn;
				
				SendMessage(hTip, TTM_ADDTOOL, 0, (LPARAM)&tool);
			}

			break;
		}

	case WM_SIZE:
		{
			auto width = LOWORD(lParam);
			auto height = HIWORD(lParam);

			auto editHeight = height - (margin * 3) - buttonHeight;

			SetWindowPos(hEdit, NULL, margin, margin, sidebarWidth - (margin * 2), editHeight, SWP_NOOWNERZORDER);
			SetWindowPos(hBtn, NULL, margin, editHeight + (margin * 2), sidebarWidth - (margin * 2), buttonHeight, SWP_NOOWNERZORDER);

			SetWindowPos(hDx, NULL, sidebarWidth, 0, width - sidebarWidth, height, SWP_NOOWNERZORDER);
			break;
		}

	case WM_COMMAND:
		if (LOWORD(wParam) == BUTTON_ID)
		{
			EnableWindow(hEdit, false);

			auto tLen = GetWindowTextLength(hEdit) + 1;
			std::vector<BYTE> data (tLen);
			size_t written = SendMessageA(hEdit, WM_GETTEXT, tLen, (LPARAM)data.data());
			ComPtr<ID3DBlob> errors;
			auto hr = app.UpdatePixelShader(data, &errors);
			if (!SUCCEEDED(hr))
			{
				if (errors != nullptr)
					MessageBoxA(hWnd, (char*)errors->GetBufferPointer(), "Error compiling shader", MB_OK | MB_ICONERROR);
				else
					MessageBox(hWnd, L"Unknown Error", L"Error compiling shader", MB_OK | MB_ICONERROR);
			}
			
			EnableWindow(hEdit, true);
		}
		break;


	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	// Handle any messages the switch statement didn't.
	return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK DXProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
		DXManager::Resize(LOWORD(lParam), HIWORD(lParam));
		break;
	}

	// Handle any messages the switch statement didn't.
	return DefWindowProc(hWnd, message, wParam, lParam);
}
