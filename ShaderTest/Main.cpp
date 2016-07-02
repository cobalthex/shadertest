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
	wclass.hIcon = wclass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDR_MAIN_ICON));
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
HWND hCombo;

LRESULT CALLBACK EditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	bool isCtrl = (GetKeyState(VK_CONTROL) & 0x8000) > 0;

	switch (message)
	{
	case WM_CHAR:
		if (isCtrl && wParam == 1)
		{
			SendMessage(hWnd, EM_SETSEL, 0, -1);
			return 1;
		}
		else if (isCtrl && wParam == '\n')
		{
			SendMessage(hBtn, BM_CLICK, 0, 0);
			SetFocus(hWnd);
			return 0;
		}
		break;
	}

	return DefSubclassProc(hWnd, message, wParam, lParam);
}

#define PS_UPDATE_BUTTON_ID 0x8801
#define PS_PROFILE_COMBO_ID 0x8802

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
				HRSRC rc = FindResource(module, MAKEINTRESOURCE(IDR_PSH), MAKEINTRESOURCE(TEXTFILE));
				HGLOBAL rcd = LoadResource(module, rc);
				DWORD sz = SizeofResource(module, rc);
				auto data = LockResource(rcd);
				auto wstr = std::wstring((char*)data, (char*)data + sz);

				hEdit = CreateWindowEx
				(
					WS_EX_CLIENTEDGE,
					WC_EDIT,
					wstr.c_str(),
					WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
					0,
					0,
					0,
					0,
					hWnd,
					NULL,
					hinst,
					nullptr
				);
				FreeResource(rcd);

				auto font = CreateFont
				(
					16,
					0, 
					0,
					0,
					FW_NORMAL, 
					FALSE, 
					FALSE, 
					FALSE,
					DEFAULT_CHARSET, 
					OUT_DEFAULT_PRECIS,
					CLIP_DEFAULT_PRECIS,
					CLEARTYPE_QUALITY,
					DEFAULT_PITCH | FF_ROMAN,
					L"Consolas"
				);
				SendMessage(hEdit, WM_SETFONT, (WPARAM)font, MAKELPARAM(FALSE, 0));
				SetWindowSubclass(hEdit, EditProc, 0, 0);
			}

			{
				hBtn = CreateWindowEx
				(
					0,
					WC_BUTTON,
					L"Update",
					WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
					0,
					0,
					0,
					0,
					hWnd,
					(HMENU)PS_UPDATE_BUTTON_ID,
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
					0, 
					0,
					0, 
					0,
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

			{
				hCombo = CreateWindowEx
				(
					0,
					WC_COMBOBOX,
					L"",
					WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWN,
					0,
					0,
					0,
					0,
					hWnd,
					(HMENU)PS_PROFILE_COMBO_ID,
					hinst,
					nullptr
				);
				SendMessage(hCombo, WM_SETFONT, (WPARAM)hDefaultFont, MAKELPARAM(FALSE, 0));

				for (size_t i = 0; i < DXManager::NumPixelShaderProfiles; i++)
					SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)DXManager::PixelShaderProfiles[i]);

				SendMessage(hCombo, CB_SETCURSEL, 0, 0);
			}

			SetFocus(hEdit);

			break;
		}

	case WM_SIZE:
		{
			auto width = LOWORD(lParam);
			auto height = HIWORD(lParam);

			auto editHeight = height - (margin * 4) - (buttonHeight * 2);

			SetWindowPos(hEdit, NULL, margin, margin, sidebarWidth - (margin * 2), editHeight, SWP_NOOWNERZORDER);
			SetWindowPos(hBtn, NULL, margin, editHeight + (margin * 2), sidebarWidth - (margin * 2), buttonHeight, SWP_NOOWNERZORDER);
			SetWindowPos(hCombo, NULL, margin, editHeight + buttonHeight + (margin * 3), sidebarWidth - (margin * 2), buttonHeight, SWP_NOOWNERZORDER);

			SetWindowPos(hDx, NULL, sidebarWidth, 0, width - sidebarWidth, height, SWP_NOOWNERZORDER);
			break;
		}

	case WM_COMMAND:
		if (LOWORD(wParam) == PS_UPDATE_BUTTON_ID)
		{
			EnableWindow(hEdit, false);

			auto selection = (DWORD)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
			
			auto tLen = GetWindowTextLength(hEdit) + 1;
			std::vector<BYTE> data (tLen);
			size_t written = SendMessageA(hEdit, WM_GETTEXT, tLen, (LPARAM)data.data());
			ComPtr<ID3DBlob> errors;
			auto hr = app.UpdatePixelShader(data, selection, &errors);
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
