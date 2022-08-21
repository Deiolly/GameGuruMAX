#include "stdafx.h"
#include "commdlg.h"

#include "imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"
#include "imgui_gg_extras.h"

#include <direct.h>


#include "CFileC.h"

#include "..\..\Guru-WickedMAX\wickedcalls.h"

preferences pref;

std::vector<SliderData> g_SliderData; // For storing max values for sliders.
std::vector<std::vector<std::string>> luadropdownlabels; // Storage for dropdown labels extracted from lua script.
std::vector<std::string> g_DLuaVariableNames;
std::vector<int> g_DLuaVariablesSet;

// Legacy WhiteList Globals
bool g_bCreateLegacyWhiteList = true;
std::vector<LPSTR> g_pLegacyWhiteList;

bool bImGuiGotFocus = false;
bool bImGuiFrameState = false;
bool bImGuiRenderTargetFocus = false;
bool bImGuiInitDone = false;
int ImGuiStatusBar_Size = 0;
bool g_bEnableAutoFlattenSystem = true;

// used by DX11 and DX12 implementations
bool bImGuiReadyToRender = false;
bool bImGuiInTestGame = false;
bool bBlockImGuiUntilNewFrame = false;
bool bImGuiRenderWithNoCustomTextures = false;
bool bBlockImGuiUntilFurtherNotice = false;
ImVec2 fImGuiScissorTopLeft = { 0, 0 };
ImVec2 fImGuiScissorBottomRight = { 0, 0 };
bool bForceRenderEverywhere = false;

ImVec2 vStartResolution = { 1280 , 800 }; // { 1024, 768 }; //1280x800
ImVec2 OldrenderTargetSize = { 0,0 };
ImVec2 OldrenderTargetPos = { 0,0 };
ImVec2 renderTargetAreaPos = { 0,0 };
ImVec2 renderTargetAreaSize = { 0,0 };
bool bCenterRenderView = false;

RECT rDigAHole = { 300,300,600,600 };
extern bool bDigAHoleToHWND;

bool IntersectsWith(RECT rect , RECT compare)
{

	if (rect.left <= compare.right && rect.right >= compare.left && rect.top <= compare.bottom)
	{
		return rect.bottom >= compare.top;
	}

	return false;
}

// Graphics API

void ImGui_ImplWicked_RenderCall_Direct( wiGraphics::CommandList cmd );
void ImGui_ImplWicked_RenderCall( wiGraphics::CommandList cmd );
//void ImGui_ImplWicked_RenderDrawData( ImDrawData* draw_data, wiGraphics::CommandList cmd );
bool ImGui_ImplWicked_Init();
void ImGui_ImplWicked_Shutdown();
void ImGui_ImplWicked_NewFrame();
void ImGui_ImplWicked_Update( wiGraphics::CommandList cmd );
bool ImGui_ImplWicked_CreateDeviceObjects();

void ImGui_RenderCall_Direct( wiGraphics::CommandList cmd )
{
	ImGui_ImplWicked_RenderCall_Direct( cmd );
}

void ImGui_RenderCall( wiGraphics::CommandList cmd )
{
	ImGui_ImplWicked_RenderCall( cmd );
}
/*
void ImGui_RenderDrawData( ImDrawData* draw_data, wiGraphics::CommandList cmd )
{
	ImGui_ImplWicked_RenderDrawData( draw_data, cmd );
}
*/
bool ImGui_Init()
{
	return ImGui_ImplWicked_Init();
}

void ImGui_Shutdown()
{
	ImGui_ImplWicked_Shutdown();
}

void ImGui_NewFrame()
{
	ImGui_ImplWicked_NewFrame();
}

void ImGui_Update( wiGraphics::CommandList cmd )
{
	ImGui_ImplWicked_Update( cmd );
}

bool ImGui_CreateDeviceObjects()
{
	return ImGui_ImplWicked_CreateDeviceObjects();
}

//#################################
//PE: Additional dialogs code here.
//#################################
#define GG_WINDOWS
#define AGK_WINDOWS

#include "globstruct.h"

extern GlobStruct*	g_pGlob;
#define g_agkhWnd g_pGlob->hWnd

#include "boxer.h"

//Noc File dialog cross platform.
//const char *noc_file_dialog_open(int flags,
//	const char *filters,
//	const char *default_path,
//	const char *default_name);

static char *g_noc_file_dialog_ret = NULL;


#ifdef GG_WINDOWS

#include <shlobj.h>
#include <conio.h>

int ImGui_Get_Allow_Undocking()
{
	return pref.iAllowUndocking;
}
int ImGui_Get_Multi_Viewports_Disabled()
{
	return pref.iDisableObjectLibraryViewport;
}

static int __stdcall BrowseCallbackProcW(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
	if (uMsg == BFFM_INITIALIZED)
	{
#ifdef UNICODE
		SendMessageW(hwnd, BFFM_SETSELECTIONW, TRUE, (LPARAM)pData);
#else
		SendMessageA(hwnd, BFFM_SETSELECTIONW, TRUE, (LPARAM)pData);
#endif
	}
	return 0;
}


// callback function
INT CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
	if (uMsg == BFFM_INITIALIZED)
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
	return 0;
}

const char *noc_file_dialog_open(int flags,
	const char *filters,
	const char *default_path,
	const char *default_name,
	bool bUseDefaultPath,
	const char *pTitle)
{
	OPENFILENAMEA ofn;       // common dialog box structure
	char szFile[MAX_PATH];       // buffer for file name
	int ret;
	szFile[0] = '\0';

	if (flags & NOC_FILE_DIALOG_DIR) 
	{
		static wchar_t lBuff[MAX_PATH];
		//wchar_t aTitle[MAX_PATH];
		BROWSEINFOW bInfo;
		LPITEMIDLIST lpItem;
		HRESULT lHResult;

		CoUninitialize();
		lHResult = CoInitialize(NULL);

		ZeroMemory(&bInfo, sizeof(BROWSEINFO));

		bInfo.hwndOwner = g_agkhWnd;
		bInfo.lpszTitle = L"Select folder";
		bInfo.lpfn = BrowseCallbackProc;

		if (lHResult == S_OK || lHResult == S_FALSE)
		{
			bInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE; //BIF_USENEWUI; //BIF_NEWDIALOGSTYLE

			if (bUseDefaultPath && default_path)
			{
				// ZJ: Added MultiByteToWideChar so cast to LPARAM is successful
				wchar_t szFolderPath[MAX_PATH];
				MultiByteToWideChar(CP_UTF8, 0, default_path, -1, szFolderPath, MAX_PATH);
				bInfo.lParam = (LPARAM)szFolderPath;

				//bInfo.lParam = (LPARAM)default_path;
			}
				
			else
				bInfo.lParam = (LPARAM)NULL;

			lpItem = SHBrowseForFolderW(&bInfo);
			if (lpItem)
			{
				SHGetPathFromIDListW(lpItem, lBuff);
			}

			if (lHResult == S_OK || lHResult == S_FALSE)
			{
				CoUninitialize();
				CoInitializeEx(NULL, COINIT_MULTITHREADED);

			}
		}
		sprintf(szFile, "%ws", lBuff);

		//Make sure ther blocking dialog did not skip some keys, reset.
		ImGuiIO& io = ImGui::GetIO();
		io.KeySuper = false;
		io.KeyCtrl = false;
		io.KeyAlt = false;
		io.KeyShift = false;

		io.KeysDown[13] = false; //also reset imgui keys.
		io.KeysDown[16] = false;
		io.KeysDown[17] = false;
		io.KeysDown[18] = false;
		io.KeysDown[19] = false;
		io.KeysDown[0x7B] = false; //F12
		io.KeysDown[78] = false;
		io.KeysDown[79] = false;
		io.KeysDown[83] = false;
		io.KeysDown[90] = false;
		io.KeysDown[73] = false; // I reset all system wide shortcuts used.

		io.KeysDown[69] = false; //New CTRL-Keys added. E-N-L-SPACE
		io.KeysDown[78] = false;
		io.KeysDown[76] = false;
		io.KeysDown[32] = false;

		io.MouseDown[0] = 0; //PE: Mouse (release) is also loast inside blocking dialogs. Reset!
		io.MouseDown[1] = 0;
		io.MouseDown[2] = 0;
		io.MouseDown[3] = 0;

		return strdup(szFile);
	}

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filters;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrTitle = pTitle;

	//PE: Why was default path removed ? added a flag instead bUseDefaultPath , need it :)
	if ((flags & NOC_FILE_DIALOG_OPEN || flags & NOC_FILE_DIALOG_SAVE) && default_path && bUseDefaultPath) {
		ofn.lpstrInitialDir = default_path;
	}
	else
		ofn.lpstrInitialDir = NULL;

	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.hwndOwner = g_agkhWnd;

	if (flags & NOC_FILE_DIALOG_DIR) 
	{
		ofn.Flags = OFN_CREATEPROMPT;
		ret = GetOpenFileNameA(&ofn);
	}
	else if (flags & NOC_FILE_DIALOG_OPEN) // || flags
		ret = GetOpenFileNameA(&ofn);
	else
		ret = GetSaveFileNameA(&ofn);

	//Make sure ther blocking dialog did not skip some keys, reset.
	ImGuiIO& io = ImGui::GetIO();
	io.KeySuper = false;
	io.KeyCtrl = false;
	io.KeyAlt = false;
	io.KeyShift = false;

	io.KeysDown[13] = false; //also reset imgui keys.
	io.KeysDown[16] = false;
	io.KeysDown[17] = false;
	io.KeysDown[18] = false;
	io.KeysDown[19] = false;
	io.KeysDown[0x7B] = false; //F12
	io.KeysDown[78] = false;
	io.KeysDown[79] = false;
	io.KeysDown[83] = false;
	io.KeysDown[90] = false;
	io.KeysDown[73] = false;
	io.KeysDown[69] = false; //New CTRL-Keys added. E-N-L-SPACE
	io.KeysDown[78] = false;
	io.KeysDown[76] = false;
	io.KeysDown[32] = false;

	io.MouseDown[0] = 0;
	io.MouseDown[1] = 0;
	io.MouseDown[2] = 0;
	io.MouseDown[3] = 0;

	if (g_noc_file_dialog_ret != NULL)
		free(g_noc_file_dialog_ret);
	g_noc_file_dialog_ret = ret ? strdup(szFile) : NULL;

	return g_noc_file_dialog_ret;
}

#else
#ifdef AGK_MACOS

#include <AppKit/AppKit.h>

const char *noc_file_dialog_open(int flags,
	const char *filters,
	const char *default_path,
	const char *default_name)
{
	NSURL *url;
	const char *utf8_path;
	NSSavePanel *panel;
	NSOpenPanel *open_panel;
	NSMutableArray *types_array;
	NSURL *default_url;
	char buf[256], *patterns;

	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	if (flags & NOC_FILE_DIALOG_OPEN) {
		panel = open_panel = [NSOpenPanel openPanel];
	}
	else {
		panel = [NSSavePanel savePanel];
	}

	if (flags & NOC_FILE_DIALOG_DIR) {
		[open_panel setCanChooseDirectories : YES];
		[open_panel setCanChooseFiles : NO];
	}

	if (default_path) {
		default_url = [NSURL fileURLWithPath :
		[NSString stringWithUTF8String : default_path]];
		[panel setDirectoryURL : default_url];
		[panel setNameFieldStringValue : default_url.lastPathComponent];
	}

	if (filters) {
		types_array = [NSMutableArray array];
		while (*filters) {
			filters += strlen(filters) + 1; // skip the name
			strcpy(buf, filters);
			buf[strlen(buf) + 1] = '\0';
			for (patterns = buf; *patterns; patterns++)
				if (*patterns == ';') *patterns = '\0';
			patterns = buf;
			while (*patterns) {
				assert(strncmp(patterns, "*.", 2) == 0);
				patterns += 2; // Skip the "*."
				[types_array addObject : [NSString stringWithUTF8String : patterns]];
				patterns += strlen(patterns) + 1;
			}
			filters += strlen(filters) + 1;
		}
		[panel setAllowedFileTypes : types_array];
	}

	if (g_noc_file_dialog_ret != NULL)
		free(g_noc_file_dialog_ret);

	g_noc_file_dialog_ret = NULL;
	if ([panel runModal] == NSModalResponseOK) {
		url = [panel URL];
		utf8_path = [[url path] UTF8String];
		g_noc_file_dialog_ret = strdup(utf8_path);
	}

	[pool release];
	return g_noc_file_dialog_ret;
}

#else
//Linux.
#include <gtk/gtk.h>

static char selected_char[1024];


const char *noc_file_dialog_open(int flags,
	const char *filters,
	const char *default_path,
	const char *default_name)
{
	GtkWidget *dialog;
	GtkFileFilter *filter;
	GtkFileChooser *chooser;
	GtkFileChooserAction action;
	gint res;
	char buf[128], *patterns;

	if (flags & NOC_FILE_DIALOG_DIR)
	{
		strcpy(selected_char, "");
		FILE *f = popen("zenity --file-selection --directory", "r");
		fgets(selected_char, 1024, f);
		pclose(f);
		int length = strlen(selected_char);
		if (length < 2)
		{
			return NULL;
		}
		if (selected_char[length - 1] == '\n' || selected_char[length - 1] == '\r') selected_char[length - 1] = 0;

		return &selected_char[0];
	}

	if (flags & NOC_FILE_DIALOG_OPEN) {
		char cmd[1024], cmd1[1024];
		//
		if (default_path) {
			if (default_name)
				sprintf(cmd, "zenity --title \"Open File\" --file-selection --filename=\"%s/%s\"", default_path, default_name);
			else
				sprintf(cmd, "zenity --title \"Open File\" --file-selection --filename=\"%s\"", default_path);
		}
		else {
			strcpy(cmd, "zenity --title \"Open File\" --file-selection");
		}


		if (filters) {
			sprintf(cmd1, " --file-filter='(%s) | *.%s'", filters, filters);
			strcat(cmd, cmd1);
		}

		strcpy(selected_char, "");
		FILE *f = popen(cmd, "r");
		fgets(selected_char, 1024, f);
		pclose(f);
		int length = strlen(selected_char);
		if (length < 2)
		{
			return NULL;
		}
		if (selected_char[length - 1] == '\n' || selected_char[length - 1] == '\r') selected_char[length - 1] = 0;

		return &selected_char[0];
	}


	if (flags & NOC_FILE_DIALOG_SAVE) {
		char cmd[1024], cmd1[1024];
		//
		if (default_path) {
			if (default_name)
				sprintf(cmd, "zenity --save --title \"Save File\" --file-selection --filename=\"%s/%s\"", default_path, default_name);
			else {
				if (default_path[strlen(default_path) - 1] == '/')
					sprintf(cmd, "zenity --save --title \"Save File\" --file-selection --filename=\"%s\"", default_path);
				else
					sprintf(cmd, "zenity --save --title \"Save File\" --file-selection --filename=\"%s/\"", default_path);
			}
		}
		else {
			strcpy(cmd, "zenity --save --title \"Save File\" --file-selection");
		}


		if (filters) {
			sprintf(cmd1, " --file-filter='(%s) | *.%s'", filters, filters);
			strcat(cmd, cmd1);
		}

		strcpy(selected_char, "");
		FILE *f = popen(cmd, "r");
		fgets(selected_char, 1024, f);
		pclose(f);
		int length = strlen(selected_char);
		if (length < 2)
		{
			return NULL;
		}
		if (selected_char[length - 1] == '\n' || selected_char[length - 1] == '\r') selected_char[length - 1] = 0;

		return &selected_char[0];
	}

	//zenity --file-selection --file-filter='PDF files (pdf) | *.pdf'
#ifdef USEGTKDIRECTLY
	action = flags & NOC_FILE_DIALOG_SAVE ? GTK_FILE_CHOOSER_ACTION_SAVE :
		GTK_FILE_CHOOSER_ACTION_OPEN;
	if (flags & NOC_FILE_DIALOG_DIR)
		action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;

	gtk_init_check(NULL, NULL);
	dialog = gtk_file_chooser_dialog_new(
		flags & NOC_FILE_DIALOG_SAVE ? "Save File" : "Open File",
		NULL,
		action,
		"_Cancel", GTK_RESPONSE_CANCEL,
		flags & NOC_FILE_DIALOG_SAVE ? "_Save" : "_Open", GTK_RESPONSE_ACCEPT,
		NULL);
	chooser = GTK_FILE_CHOOSER(dialog);
	if (flags & NOC_FILE_DIALOG_OVERWRITE_CONFIRMATION)
		gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);

	if (default_path)
		gtk_file_chooser_set_filename(chooser, default_path);
	if (default_name)
		gtk_file_chooser_set_current_name(chooser, default_name);

	while (filters && *filters) {
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, filters);
		filters += strlen(filters) + 1;

		// Split the filter pattern with ';'.
		strcpy(buf, filters);
		buf[strlen(buf)] = '\0';
		for (patterns = buf; *patterns; patterns++)
			if (*patterns == ';') *patterns = '\0';
		patterns = buf;
		while (*patterns) {
			gtk_file_filter_add_pattern(filter, patterns);
			patterns += strlen(patterns) + 1;
		}

		gtk_file_chooser_add_filter(chooser, filter);
		filters += strlen(filters) + 1;
	}

	res = gtk_dialog_run(GTK_DIALOG(dialog));

	free(g_noc_file_dialog_ret);
	g_noc_file_dialog_ret = NULL;

	if (res == GTK_RESPONSE_ACCEPT)
		g_noc_file_dialog_ret = gtk_file_chooser_get_filename(chooser);
	gtk_widget_destroy(dialog);
	while (gtk_events_pending()) gtk_main_iteration();
	return g_noc_file_dialog_ret;
#endif

}

#endif
#endif


bool CancelQuit()
{
	boxer::Selection selection;
	selection = boxer::show("Are you sure you want to quit ?", " Warning!", boxer::Style::Question, boxer::Buttons::OKCancel);

	//Make sure ther blocking dialog did not skip some keys, reset.
	ImGuiIO& io = ImGui::GetIO();
	io.KeySuper = false;
	io.KeyCtrl = false;
	io.KeyAlt = false;
	io.KeyShift = false;

	io.KeysDown[13] = false;
	io.KeysDown[17] = false;
	io.KeysDown[18] = false;
	io.KeysDown[19] = false;
	io.KeysDown[0x7B] = false; //F12
	io.KeysDown[78] = false;
	io.KeysDown[79] = false;
	io.KeysDown[83] = false;
	io.KeysDown[90] = false;
	io.KeysDown[73] = false;
	io.KeysDown[69] = false; //New CTRL-Keys added. E-N-L-SPACE
	io.KeysDown[78] = false;
	io.KeysDown[76] = false;
	io.KeysDown[32] = false;


	io.MouseDown[0] = 0;
	io.MouseDown[1] = 0;
	io.MouseDown[2] = 0;
	io.MouseDown[3] = 0;

	if (selection == boxer::Selection::Cancel) {
		return true;
	}

	return false;
}

bool overWriteFileBox(char * file)
{
	boxer::Selection selection;
	selection = boxer::show(" File exists, do you want to overwrite file?", " Warning!", boxer::Style::Question, boxer::Buttons::YesNo);

	//Make sure ther blocking dialog did not skip some keys, reset.
	ImGuiIO& io = ImGui::GetIO();
	io.KeySuper = false;
	io.KeyCtrl = false;
	io.KeyAlt = false;
	io.KeyShift = false;
	io.KeysDown[13] = false;
	io.KeysDown[16] = false;
	io.KeysDown[17] = false;
	io.KeysDown[18] = false;
	io.KeysDown[19] = false;
	io.MouseDown[0] = 0;
	io.MouseDown[1] = 0;
	io.MouseDown[2] = 0;
	io.MouseDown[3] = 0;
	io.KeysDown[0x7B] = false; //F12
	io.KeysDown[78] = false;
	io.KeysDown[79] = false;
	io.KeysDown[83] = false;
	io.KeysDown[90] = false;
	io.KeysDown[73] = false;
	io.KeysDown[69] = false; //New CTRL-Keys added. E-N-L-SPACE
	io.KeysDown[78] = false;
	io.KeysDown[76] = false;
	io.KeysDown[32] = false;

	if (selection == boxer::Selection::Yes) return(true);

	return(false);
}


int askBoxCancel(const char * ask, const char *title)
{
	boxer::Selection selection;
	selection = boxer::show(ask, title, boxer::Style::Question, boxer::Buttons::YesNoCancel);

	//Make sure ther blocking dialog did not skip some keys, reset.
	ImGuiIO& io = ImGui::GetIO();
	io.KeySuper = false;
	io.KeyCtrl = false;
	io.KeyAlt = false;
	io.KeyShift = false;

	io.KeysDown[13] = false; //also reset imgui keys.
	io.KeysDown[16] = false;
	io.KeysDown[17] = false;
	io.KeysDown[18] = false;
	io.KeysDown[19] = false;
	io.KeysDown[0x7B] = false; //F12
	io.KeysDown[78] = false;
	io.KeysDown[79] = false;
	io.KeysDown[83] = false;
	io.KeysDown[90] = false;
	io.KeysDown[73] = false;
	io.KeysDown[69] = false; //New CTRL-Keys added. E-N-L-SPACE
	io.KeysDown[78] = false;
	io.KeysDown[76] = false;
	io.KeysDown[32] = false;

	io.MouseDown[0] = 0;
	io.MouseDown[1] = 0;
	io.MouseDown[2] = 0;
	io.MouseDown[3] = 0;

	if (selection == boxer::Selection::Yes) return(1);
	if (selection == boxer::Selection::Cancel) return(2);
	return(0);
}

bool askBox(char * ask, char *title)
{
	boxer::Selection selection;
	selection = boxer::show(ask, title, boxer::Style::Question, boxer::Buttons::YesNo);

	//Make sure ther blocking dialog did not skip some keys, reset.
	ImGuiIO& io = ImGui::GetIO();
	io.KeySuper = false;
	io.KeyCtrl = false;
	io.KeyAlt = false;
	io.KeyShift = false;

	io.KeysDown[13] = false; //also reset imgui keys.
	io.KeysDown[16] = false;
	io.KeysDown[17] = false;
	io.KeysDown[18] = false;
	io.KeysDown[19] = false;
	io.KeysDown[0x7B] = false; //F12
	io.KeysDown[78] = false;
	io.KeysDown[79] = false;
	io.KeysDown[83] = false;
	io.KeysDown[90] = false;
	io.KeysDown[73] = false;
	io.KeysDown[69] = false; //New CTRL-Keys added. E-N-L-SPACE
	io.KeysDown[78] = false;
	io.KeysDown[76] = false;
	io.KeysDown[32] = false;

	io.MouseDown[0] = 0;
	io.MouseDown[1] = 0;
	io.MouseDown[2] = 0;
	io.MouseDown[3] = 0;

	if (selection == boxer::Selection::Yes) return(true);

	return(false);
}

bool changedFileBox(char * file)
{
	boxer::Selection selection;
	char msg[1024];

	sprintf(msg, "The file has unsaved changes. Do you want to save it?\n%s", file);
	selection = boxer::show(msg, " Warning!", boxer::Style::Question, boxer::Buttons::YesNo);

	//Make sure ther blocking dialog did not skip some keys, reset.
	ImGuiIO& io = ImGui::GetIO();
	io.KeySuper = false;
	io.KeyCtrl = false;
	io.KeyAlt = false;
	io.KeyShift = false;

	io.KeysDown[13] = false;
	io.KeysDown[16] = false;
	io.KeysDown[17] = false;
	io.KeysDown[18] = false;
	io.KeysDown[19] = false;
	io.KeysDown[0x7B] = false; //F12
	io.KeysDown[78] = false;
	io.KeysDown[79] = false;
	io.KeysDown[83] = false;
	io.KeysDown[90] = false;
	io.KeysDown[73] = false;
	io.KeysDown[69] = false; //New CTRL-Keys added. E-N-L-SPACE
	io.KeysDown[78] = false;
	io.KeysDown[76] = false;
	io.KeysDown[32] = false;

	io.MouseDown[0] = 0;
	io.MouseDown[1] = 0;
	io.MouseDown[2] = 0;
	io.MouseDown[3] = 0;

	if (selection == boxer::Selection::Yes) return(true);

	return(false);
}

void BoxerInfo(char * text, const char *heading)
{
	boxer::show(text, heading);
	//Make sure ther blocking dialog did not skip some keys, reset.
	ImGuiIO& io = ImGui::GetIO();
	io.KeySuper = false;
	io.KeyCtrl = false;
	io.KeyAlt = false;
	io.KeyShift = false;

	io.KeysDown[13] = false;
	io.KeysDown[16] = false;
	io.KeysDown[17] = false;
	io.KeysDown[18] = false;
	io.KeysDown[19] = false;
	//PE: Need to reset any system wide hotkey that execute a blocking dialog.
	io.KeysDown[0x7B] = false; //F12
	io.KeysDown[78] = false;
	io.KeysDown[79] = false;
	io.KeysDown[83] = false;
	io.KeysDown[90] = false;
	io.KeysDown[73] = false;
	io.KeysDown[69] = false; //New CTRL-Keys added. E-N-L-SPACE
	io.KeysDown[78] = false;
	io.KeysDown[76] = false;
	io.KeysDown[32] = false;

	io.MouseDown[0] = 0;
	io.MouseDown[1] = 0;
	io.MouseDown[2] = 0;
	io.MouseDown[3] = 0;


}

void DebugInfo(char * text, const char *heading)
{
#ifndef DEVVERSION
	return;
#else
	boxer::show(text, heading);
	//Make sure ther blocking dialog did not skip some keys, reset.
	ImGuiIO& io = ImGui::GetIO();
	io.KeySuper = false;
	io.KeyCtrl = false;
	io.KeyAlt = false;
	io.KeyShift = false;
#endif
}



//################################################################
//#### PE: ImgBtn                                             ####
//#### Used to add image buttons directly from a GG image id. ####
//################################################################

#include "CImageC.h"

namespace ImGui {

	const char* CalcWordWrapPositionB(float scale, const char* textorig, const char* text_end, float wrap_width, float line_start)
	{
		// Simple word-wrapping for English, not full-featured. Please submit failing cases!
		// FIXME: Much possible improvements (don't cut things like "word !", "word!!!" but cut within "word,,,,", more sensible support for punctuations, support for Unicode punctuations, etc.)

		// For references, possible wrap point marked with ^
		//  "aaa bbb, ccc,ddd. eee   fff. ggg!"
		//      ^    ^    ^   ^   ^__    ^    ^

		// List of hardcoded separators: .,;!?'"

		// Skip extra blanks after a line returns (that includes not counting them in width computation)
		// e.g. "Hello    world" --> "Hello" "World"

		// Cut words that cannot possibly fit within one line.
		// e.g.: "The tropical fish" with ~5 characters worth of width --> "The tr" "opical" "fish"

		float line_width = line_start / scale;
		float word_width = 0.0f;
		float blank_width = 0.0f;
		wrap_width /= scale; // We work with unscaled widths to avoid scaling every characters

		// can mess up with text starts with spaces!
		const char* text = textorig;
		while ((*text == ' ' || *text == '/t') && text<text_end) text++;

		const char* word_end = text;
		//const char* prev_word_end = NULL;
		const char* prev_word_end = line_start > 0.0f ? word_end : NULL;
		bool inside_word = true;

		const char* s = text;
		while (s < text_end)
		{
			unsigned int c = (unsigned int)*s;
			const char* next_s;
			if (c < 0x80)
				next_s = s + 1;
			else
				next_s = s + ImTextCharFromUtf8(&c, s, text_end);
			if (c == 0)
				break;

			if (c < 32)
			{
				if (c == '\n')
				{
					line_width = word_width = blank_width = 0.0f;
					inside_word = true;
					s = next_s;
					continue;
				}
				if (c == '\r')
				{
					s = next_s;
					continue;
				}
			}

			const float char_width = ImGui::GetFont()->GetCharAdvance(c); //((int)c < IndexAdvanceX.Size ? IndexAdvanceX.Data[c] : FallbackAdvanceX);
			if (ImCharIsBlankW(c))
			{
				if (inside_word)
				{
					line_width += blank_width;
					blank_width = 0.0f;
					word_end = s;
				}
				blank_width += char_width;
				inside_word = false;
			}
			else
			{
				word_width += char_width;
				if (inside_word)
				{
					word_end = next_s;
				}
				else
				{
					prev_word_end = word_end;
					line_width += word_width + blank_width;
					word_width = blank_width = 0.0f;
				}

				// Allow wrapping after punctuation.
				inside_word = !(c == '.' || c == ',' || c == ';' || c == '!' || c == '?' || c == '\"');
			}

			// We ignore blank width at the end of the line (they can be skipped)
			if (line_width + word_width > wrap_width)
			{
				// Words that cannot possibly fit within an entire line will be cut anywhere.
				if (word_width < wrap_width)
					s = prev_word_end ? prev_word_end : word_end;
				break;
			}

			s = next_s;
		}

		return s;
	}


	void TextCenter(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGuiContext& g = *GImGui;
		const char* text_end = g.TempBuffer + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);

		
		float textwidth = ImGui::CalcTextSize(g.TempBuffer).x;
		//PE: Change - Account for indent.
		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2( (GetContentRegionAvail().x*0.5) - (textwidth*0.5) - (window->DC.Indent.x*0.5) , 0.0f));

		TextEx(g.TempBuffer, text_end, ImGuiTextFlags_NoWidthForLargeClippedText);
		va_end(args);
	}

	bool StyleCollapsingHeader(const char* label, ImGuiTreeNodeFlags flags)
	{
		if (pref.current_style == 25)
		{
			//ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, ImVec2(0, 0));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.43f, 0.57f, 1.00f));
			auto *style = &ImGui::GetStyle();
			style->FrameBorderSize = 0.0f;
		}

		bool bret = CollapsingHeader(label, flags);
		
		if (pref.current_style == 25)
		{
			ImGui::PopStyleColor();
			auto *style = &ImGui::GetStyle();
			style->FrameBorderSize = 1.0f;
			//ImGui::PopStyleVar();
		}

		return bret;
	}

	bool HyberlinkButton(const char* label, const ImVec2& size_arg)
	{
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.43f, 0.57f, 0.00f)); //no border.
		auto *style = &ImGui::GetStyle();
		ImVec4* colors = style->Colors;
		
		ImVec4 backImGuiCol_Button = colors[ImGuiCol_Button];
		ImVec4 backImGuiCol_ButtonHovered = colors[ImGuiCol_ButtonHovered];
		ImVec4 backImGuiCol_ButtonActive = colors[ImGuiCol_ButtonActive];

		colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

		style->FrameBorderSize = 0.0f;

		ImVec2 cpos = ImGui::GetCursorPos();

		bool bret = ImGui::Button(label, size_arg);
		
		ImGuiWindow* window = GetCurrentWindow();

		//window->DC.CursorPosPrevLine.x
		float yoffset = 8.0;
		float xpadding = 4.0;
		ImRect image_bb(ImVec2(window->Pos.x + window->Scroll.x + cpos.x + xpadding, window->DC.CursorPos.y - yoffset), ImVec2(window->DC.CursorPosPrevLine.x- xpadding, (window->DC.CursorPos.y - yoffset) + 2.0f));

		ImVec4 col = backImGuiCol_Button;
		if (IsItemHovered())
		{
			col = backImGuiCol_ButtonHovered;
		}
		window->DrawList->AddRectFilled(image_bb.Min, image_bb.Max, GetColorU32(col));

		ImGui::PopStyleColor();
		style->FrameBorderSize = 1.0f;

		colors[ImGuiCol_Button] = backImGuiCol_Button;
		colors[ImGuiCol_ButtonHovered] = backImGuiCol_ButtonHovered;
		colors[ImGuiCol_ButtonActive] = backImGuiCol_ButtonActive;

		return bret;
	}

	bool MinMaxButtonEx(const char* str_id, ImGuiDir dir)
	{
		float sz = GetFrameHeight();
		ImVec2 size = ImVec2(sz, sz);

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiID id = window->GetID(str_id);
		const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
		const float default_size = GetFrameHeight();
		ItemSize(size, (size.y >= default_size) ? g.Style.FramePadding.y : 0.0f);
		if (!ItemAdd(bb, id))
			return false;

		bool hovered, held;
		bool pressed = ButtonBehavior(bb, id, &hovered, &held, 0);

		// Render
		const ImU32 bg_col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
		const ImU32 text_col = GetColorU32(ImGuiCol_Text);
		RenderNavHighlight(bb, id);
		RenderFrame(bb.Min, bb.Max, bg_col, true, g.Style.FrameRounding);
		RenderArrowOutLine(window->DrawList, bb.Min + ImVec2(ImMax(0.0f, (size.x - g.FontSize) * 0.5f), ImMax(0.0f, (size.y - g.FontSize) * 0.5f)), text_col, dir , 1.0f);

		return pressed;
	}

	void AddTriangleNotClosed(ImDrawList* draw_list,const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col, float thickness)
	{
		if ((col & IM_COL32_A_MASK) == 0)
			return;

		draw_list->PathLineTo(p2);
		draw_list->PathLineTo(p1);
		draw_list->PathLineTo(p3);
		draw_list->PathStroke(col, false, thickness);
	}

	void RenderArrowOutLine(ImDrawList* draw_list, ImVec2 pos, ImU32 col, ImGuiDir dir, float scale)
	{
		const float h = draw_list->_Data->FontSize * 1.00f;
		float r = h * 0.40f * scale;
		ImVec2 center = pos + ImVec2(h * 0.50f, h * 0.50f * scale);

		ImVec2 a, b, c;
		switch (dir)
		{
		case ImGuiDir_Up:
		case ImGuiDir_Down:
			if (dir == ImGuiDir_Up) r = -r;
			a = ImVec2(+0.000f, +0.750f) * r;
			b = ImVec2(-0.866f, -0.750f) * r;
			c = ImVec2(+0.866f, -0.750f) * r;
			break;
		case ImGuiDir_Left:
		case ImGuiDir_Right:
			if (dir == ImGuiDir_Left) r = -r;
			a = ImVec2(+0.750f, +0.000f) * r;
			b = ImVec2(-0.750f, +0.866f) * r;
			c = ImVec2(-0.750f, -0.866f) * r;
			break;
		case ImGuiDir_None:
		case ImGuiDir_COUNT:
			IM_ASSERT(0);
			break;
		}
		AddTriangleNotClosed(draw_list,center + a, center + b, center + c, col,1.75f);
	}

	bool StyleButtonEx(const char* label, const ImVec2& size_arg, bool bDisabled)
	{
		ImGuiButtonFlags_ flags = ImGuiButtonFlags_None;
		if (bDisabled == true) flags = ImGuiButtonFlags_Disabled;
		if (pref.current_style == 25)
		{
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.43f, 0.57f, 1.00f));
			auto *style = &ImGui::GetStyle();
			style->FrameBorderSize = 0.0f;
		}
		if (bDisabled == true) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.22f, 0.22f, 0.5f));
		bool bret = ImGui::ButtonEx(label, size_arg, flags);
		if (bDisabled == true) ImGui::PopStyleColor();
		if (pref.current_style == 25)
		{
			ImGui::PopStyleColor();
			auto *style = &ImGui::GetStyle();
			style->FrameBorderSize = 1.0f;
		}
		return bret;
	}

	bool StyleButton(const char* label, const ImVec2& size_arg)
	{
		if (pref.current_style == 25)
		{
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.43f, 0.57f, 1.00f));
			auto *style = &ImGui::GetStyle();
			style->FrameBorderSize = 0.0f;
		}
		bool bret = ImGui::Button(label, size_arg);
		if (pref.current_style == 25)
		{
			ImGui::PopStyleColor();
			auto *style = &ImGui::GetStyle();
			style->FrameBorderSize = 1.0f;
			//ImGui::PopStyleVar();
		}
		return bret;
	}

	bool StyleButtonDark(const char* label, const ImVec2& size_arg)
	{
		auto* style = &ImGui::GetStyle();
		style->FrameBorderSize = 0.0f;
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.33f, 0.5f, 1.0f));
		bool bret = ImGui::Button(label, size_arg);
		ImGui::PopStyleColor();
		style->FrameBorderSize = 1.0f;
	
		return bret;
	}

	//PE: Max gadget types.
	bool bLastSliderHovered = false;
	bool IsLastSliderHovered(void)
	{
		return(bLastSliderHovered);
	}

#ifdef WICKEDENGINE
	bool MaxSliderInputInt(const char* label, int* v, int v_min, int v_max, const char* tooltip, int boxWidth)
	{
		bLastSliderHovered = false;
		if (!label) return(false);
		char cUniqueLabel[256];
		strncpy(cUniqueLabel, label, 240);
		cUniqueLabel[240] = 0;

		int iCounter = g_SliderData.size() - 1;
		// Check to see if this slider has a non-default max value.
		for (int i = 0; i < g_SliderData.size(); i++)
		{
			if (v_max == g_SliderData[i].fDefaultMaxValue)
			{
				if (strcmp(cUniqueLabel, g_SliderData[i].cID) == 0)
				{
					// This slider has a non-default max value, so use it instead of the value passed in.
					if (*v > g_SliderData[i].fHighestMaxValue)
						g_SliderData[i].fHighestMaxValue = *v;

					v_max = g_SliderData[i].fHighestMaxValue;

					break;
				}
			}
			
			iCounter--;
		}

		if (iCounter <= 0)
		{
			// This slider does not have a custom max value set yet.
			if (*v > v_max)
			{
				SliderData data;
				data.fDefaultMaxValue = v_max;
				data.fHighestMaxValue = *v;
				strcpy(data.cID, cUniqueLabel);
				g_SliderData.push_back(data);

				v_max = *v;
			}
		}

		int iInput = *v;
		bool bRet = false;
		ImGui::PushItemWidth(-10 - boxWidth - 10);

		if (ImGui::SliderInt(cUniqueLabel, &iInput, v_min, v_max, " "))
		{
			bRet = true;
		}
		if (tooltip && ImGui::IsItemHovered()) {
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::PushItemWidth(boxWidth);
		
		if (strstr(cUniqueLabel, "##"))
			strcat(cUniqueLabel, "i");
		else
			strcat(cUniqueLabel, "##i");

		if (ImGui::InputInt(cUniqueLabel, &iInput, 0, 0))
		{

			bRet = true;
		}
		if (!pref.iTurnOffEditboxTooltip && tooltip && ImGui::IsItemHovered()) {
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();

		if (iInput < v_min)
			iInput = v_min;

		*v = iInput;
		return(bRet);
	}

	bool MaxSliderInputFloat(const char* label, float* v, float v_min, float v_max, const char* tooltip, int startval, float maxval, int numericboxwidth)
	{
		bLastSliderHovered = false;
		if (!label) return(false);
		char cUniqueLabel[256];
		strncpy(cUniqueLabel, label, 240);
		cUniqueLabel[240] = 0;

		int iCounter = g_SliderData.size() - 1;
		// Check to see if this slider has a non-default max value.
		for (int i = 0; i < g_SliderData.size(); i++)
		{
			// Compare the default values first, to save doing lots of string compares.
			if (v_max == g_SliderData[i].fDefaultMaxValue)  
			{
				if (strcmp(label, g_SliderData[i].cID) == 0)
				{
					// This slider has a non-default max value, so use it instead of the value passed in.
					if (*v > g_SliderData[i].fHighestMaxValue)
						g_SliderData[i].fHighestMaxValue = *v;

					v_max = g_SliderData[i].fHighestMaxValue;
					maxval = v_max;

					break;
				}
			}

			iCounter--;
		}

		if (iCounter <= 0)
		{
			// This slider does not have a custom max value set yet.
			if (*v > v_max)
			{
				SliderData data;
				data.fDefaultMaxValue = v_max;
				data.fHighestMaxValue = *v;
				strcpy(data.cID, label);
				g_SliderData.push_back(data);

				v_max = *v;
				maxval = v_max;
			}
		}

		float fInput = *v - v_min;
		
		float fRange = v_max - v_min;
		bool bRet = false;
		int iControlInput, iMaxInt = 100;
		ImGui::PushItemWidth(-10 - 40);
		if(v_max <= 10.0)
			iControlInput = maxval / fRange * (fInput + 0.009f); // Added +0.009 to eliminate floating point error which resulted in iControlInput dropping 1
		else
			iControlInput = maxval / fRange * ceil(fInput);

		if (iControlInput < startval)
			iControlInput = startval;

		iMaxInt = maxval;

		if (ImGui::SliderInt (cUniqueLabel, &iControlInput, startval, iMaxInt, " "))
		{
			fInput = (float)iControlInput * (fRange / maxval);
			bRet = true;
		}
		if (tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushItemWidth(numericboxwidth);
		strcat(cUniqueLabel, "##i");

		if (ImGui::InputInt(cUniqueLabel, &iControlInput, 0, 0))
		{
			if (v_max <= 10.0)
				fInput = (float)iControlInput * (fRange / maxval);
			else
				fInput = ceil((float)iControlInput * (fRange / maxval));
			bRet = true;
		}
		if (!pref.iTurnOffEditboxTooltip && tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();

		*v = v_min + fInput;
		return(bRet);
	}

	bool MaxSliderInputFloat2(const char* label, float* v, float v_min, float v_max, const char* tooltip, int startval, float maxval, int numericboxwidth)
	{
		bLastSliderHovered = false;
		if (!label) return(false);
		char cUniqueLabel[256];
		strncpy(cUniqueLabel, label, 240);
		cUniqueLabel[240] = 0;

		int iCounter = g_SliderData.size() - 1;
		// Check to see if this slider has a non-default max value.
		for (int i = 0; i < g_SliderData.size(); i++)
		{
			// Compare the default values first, to save doing lots of string compares.
			if (v_max == g_SliderData[i].fDefaultMaxValue)
			{
				if (strcmp(label, g_SliderData[i].cID) == 0)
				{
					// This slider has a non-default max value, so use it instead of the value passed in.
					if (*v > g_SliderData[i].fHighestMaxValue)
						g_SliderData[i].fHighestMaxValue = *v;

					v_max = g_SliderData[i].fHighestMaxValue;
					maxval = v_max;

					break;
				}
			}

			iCounter--;
		}

		if (iCounter <= 0)
		{
			// This slider does not have a custom max value set yet.
			if (*v > v_max)
			{
				SliderData data;
				data.fDefaultMaxValue = v_max;
				data.fHighestMaxValue = *v;
				strcpy(data.cID, label);
				g_SliderData.push_back(data);

				v_max = *v;
				maxval = v_max;
			}
		}

		float fInput = *v - v_min;
		float fRange = v_max - v_min;
		bool bRet = false;
		float fControlInput, iMaxInt = 100;
		ImGui::PushItemWidth(-10 - 10 - numericboxwidth);
		fControlInput = maxval / fRange * fInput;
		if (fControlInput < startval)
			fControlInput = startval;

		iMaxInt = maxval;

		if (ImGui::SliderFloat(cUniqueLabel, &fControlInput, startval, iMaxInt, " ")) //"%.2f"
		{
			fInput = (float)fControlInput * (fRange / maxval);
			bRet = true;
		}
		if (tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushItemWidth(numericboxwidth);
		strcat(cUniqueLabel, "##i");
		if (ImGui::InputFloat(cUniqueLabel, &fControlInput, 0, 0, 2))
		{
			fInput = (float)fControlInput * (fRange / maxval);
			bRet = true;
		}
		if (tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();

		*v = v_min + fInput;
		return(bRet);
	}

	// Use for sliders with large ranges, where fine control over the slider values is required.
	bool MaxSliderInputFloatPower(const char* label, float* v, float v_min, float v_max, const char* tooltip, int startval, float maxval, int numericboxwidth, float power, int precision)
	{
		bLastSliderHovered = false;
		if (!label) return(false);
		char cUniqueLabel[256];
		strncpy(cUniqueLabel, label, 240);
		cUniqueLabel[240] = 0;

		int iCounter = g_SliderData.size() - 1;
		// Check to see if this slider has a non-default max value.
		for (int i = 0; i < g_SliderData.size(); i++)
		{ 
			// Compare the default values first, to save doing lots of string compares.
			if (v_max == g_SliderData[i].fDefaultMaxValue)
			{
				if (strcmp(label, g_SliderData[i].cID) == 0)
				{
					// This slider has a non-default max value, so use it instead of the value passed in.
					if (*v > g_SliderData[i].fHighestMaxValue)
						g_SliderData[i].fHighestMaxValue = *v;

					v_max = g_SliderData[i].fHighestMaxValue;
					maxval = v_max;

					break;
				}
			}

			iCounter--;
		}

		if (iCounter <= 0)
		{
			// This slider does not have a custom max value set yet.
			if (*v > v_max)
			{
				SliderData data;
				data.fDefaultMaxValue = v_max;
				data.fHighestMaxValue = *v;
				strcpy(data.cID, label);
				g_SliderData.push_back(data);

				v_max = *v;
				maxval = v_max;
			}
		}

		float fInput = *v - v_min;
		float fRange = v_max - v_min;
		bool bRet = false;
		float fControlInput, fMax = 100;
		ImGui::PushItemWidth(-10 - 10 - numericboxwidth);
		fControlInput = maxval / fRange * fInput;
		if (fControlInput < startval)
			fControlInput = startval;

		fMax = maxval;

		if (ImGui::SliderFloat(cUniqueLabel, &fControlInput, startval, fMax, "", power))
		{
			fInput = fControlInput * (fRange / maxval);
			bRet = true;
		}
		if (tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushItemWidth(numericboxwidth);
		strcat(cUniqueLabel, "##i");
		//if (ImGui::InputInt(cUniqueLabel, &fControlInput, 0, 0))
		if (ImGui::InputFloat(cUniqueLabel, &fControlInput, 0, 0, precision))
		{
			fInput = fControlInput * (fRange / maxval);
			bRet = true;
		}
		if (!pref.iTurnOffEditboxTooltip && tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();

		*v = v_min + fInput;
		return(bRet);
	}

	bool MaxSliderInputRangeFloat(const char* label, float* v, float* v2, float v_min, float v_max, const char* tooltip)
	{
		bLastSliderHovered = false;
		if (!label) return(false);

		char cUniqueLabel[256];
		strncpy(cUniqueLabel, label, 240);
		cUniqueLabel[240] = 0;

		//int iCounter = g_SliderData.size() - 1;
		//// Check to see if this slider has a non-default max value.
		//for (int i = 0; i < g_SliderData.size(); i++)
		//{
		//	// Compare the default values first, to save doing lots of string compares.
		//	if (v_max == g_SliderData[i].fDefaultMaxValue)
		//	{
		//		if (strcmp(label, g_SliderData[i].cID) == 0)
		//		{
		//			// This slider has a non-default max value, so use it instead of the value passed in.
		//			if (*v2 > g_SliderData[i].fHighestMaxValue)
		//				g_SliderData[i].fHighestMaxValue = *v2;
		//
		//			v_max = g_SliderData[i].fHighestMaxValue;
		//
		//			break;
		//		}
		//	}
		//
		//	iCounter--;
		//}
		//
		//if (iCounter <= 0)
		//{
		//	// This slider does not have a custom max value set yet.
		//	if (*v2 > v_max)
		//	{
		//		SliderData data;
		//		data.fDefaultMaxValue = v_max;
		//		data.fHighestMaxValue = *v2;
		//		strcpy(data.cID, label);
		//		g_SliderData.push_back(data);
		//
		//		v_max = *v2;
		//	}
		//}

		float fInput = *v - v_min;
		float fInput2 = *v2;

		//		if (fInput > fInput2)
		//		{
		//			float ftmp = fInput;
		//			fInput = fInput2;
		//			fInput2 = ftmp;
		//		}

		float fRange = v_max - v_min;
		bool bRet = false;
		int iControlInput, iControlInput2;
		iControlInput = 100.0 / fRange * fInput;
		iControlInput2 = 100.0 / fRange * fInput2;
		float fTmp1 = iControlInput, fTmp2 = iControlInput2;

		//ImGui::SetCursorPos( ImVec2(ImGui::GetCursorPosX(), end_post.y) );
		ImGui::PushItemWidth(30);
		strcat(cUniqueLabel, "##i");
		if (ImGui::InputInt(cUniqueLabel, &iControlInput, 0, 0))
		{
			fInput = ceil((float)iControlInput * (fRange / 100.0));
			bRet = true;
		}
		if (!pref.iTurnOffEditboxTooltip && tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushItemWidth(-10 - 40); //77
		ImVec2 end_post = ImGui::GetCursorPos();
		strcat(cUniqueLabel, "slider");
		if (ImGui::RangeSlider(cUniqueLabel, fTmp1, fTmp2, 100.0f, false))
		{
			fInput = fTmp1 * (fRange / 100.0);
			fInput2 = fTmp2 * (fRange / 100.0);
			bRet = true;
		}
		if (tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushItemWidth(30);
		strcat(cUniqueLabel, "t");
		if (ImGui::InputInt(cUniqueLabel, &iControlInput2, 0, 0))
		{
			fInput2 = ceil((float)iControlInput2 * (fRange / 100.0));
			bRet = true;
		}
		if (!pref.iTurnOffEditboxTooltip && tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();

		//		if (fInput > fInput2)
		//		{
		//			float ftmp = fInput;
		//			fInput = fInput2;
		//			fInput2 = ftmp;
		//		}

		*v = v_min + fInput;
		*v2 = fInput2;
		return(bRet);
	}


	bool MaxSliderInputRangeFloatDirect(const char* label, float* v, float* v2, float v_min, float v_max, const char* tooltip)
	{
		bLastSliderHovered = false;
		if (!label) return(false);

		char cUniqueLabel[256];
		strncpy(cUniqueLabel, label, 240);
		cUniqueLabel[240] = 0;

		float fInput = *v - v_min;
		float fInput2 = *v2;

		float fRange = v_max - v_min;
		bool bRet = false;
		int iControlInput, iControlInput2;
		iControlInput = 100.0 / fRange * fInput;
		iControlInput2 = 100.0 / fRange * fInput2;
		//float fTmp1 = iControlInput, fTmp2 = iControlInput2;
		float fTmp1 = fInput, fTmp2 = fInput2;

		//ImGui::SetCursorPos( ImVec2(ImGui::GetCursorPosX(), end_post.y) );
		ImGui::PushItemWidth(30);
		strcat(cUniqueLabel, "##i");
		if (ImGui::InputInt(cUniqueLabel, &iControlInput, 0, 0))
		{
			fInput = ceil((float)iControlInput * (fRange / 100.0));
			bRet = true;
		}
		if (!pref.iTurnOffEditboxTooltip && tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushItemWidth(-10 - 40); //77
		ImVec2 end_post = ImGui::GetCursorPos();
		strcat(cUniqueLabel, "slider");
		if (ImGui::RangeSlider(cUniqueLabel, fTmp1, fTmp2, v_max, false))
		{
			fInput = fTmp1; // *(fRange / 100.0);
			fInput2 = fTmp2; // *(fRange / 100.0);
			bRet = true;
		}
		if (tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushItemWidth(30);
		strcat(cUniqueLabel, "t");
		if (ImGui::InputInt(cUniqueLabel, &iControlInput2, 0, 0))
		{
			fInput2 = ceil((float)iControlInput2 * (fRange / 100.0));
			bRet = true;
		}
		if (!pref.iTurnOffEditboxTooltip && tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();

		//		if (fInput > fInput2)
		//		{
		//			float ftmp = fInput;
		//			fInput = fInput2;
		//			fInput2 = ftmp;
		//		}

		*v = v_min + fInput;
		*v2 = fInput2;
		return(bRet);
	}

#else

	bool MaxSliderInputInt(const char* label, int* v, int v_min, int v_max, const char* tooltip)
	{
		bLastSliderHovered = false;
		if (!label) return(false);
		char cUniqueLabel[256];
		strncpy(cUniqueLabel, label, 240);
		cUniqueLabel[240] = 0;

		int iInput = *v;
		bool bRet = false;
		ImGui::PushItemWidth(-10 - 40);

		if (ImGui::SliderInt(cUniqueLabel, &iInput, v_min, v_max, " "))
		{
			bRet = true;
		}
		if (tooltip && ImGui::IsItemHovered()) {
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::PushItemWidth(30);
		strcat(cUniqueLabel, "##i");

		if (ImGui::InputInt(cUniqueLabel, &iInput, 0, 0))
		{
			bRet = true;
		}
		if (!pref.iTurnOffEditboxTooltip && tooltip && ImGui::IsItemHovered()) {
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();

		if (iInput < v_min)
			iInput = v_min;
		if (iInput > v_max)
			iInput = v_max;

		*v = iInput;
		return(bRet);
	}

	bool MaxSliderInputFloat(const char* label, float* v, float v_min, float v_max, const char* tooltip, int startval, float maxval, int numericboxwidth)
	{
		bLastSliderHovered = false;
		if (!label) return(false);
		char cUniqueLabel[256];
		strncpy(cUniqueLabel, label, 240);
		cUniqueLabel[240] = 0;

		float fInput = *v - v_min;
		float fRange = v_max - v_min;
		bool bRet = false;
		int iControlInput, iMaxInt = 100;
		ImGui::PushItemWidth(-10 - 40);
		iControlInput = maxval / fRange * fInput;
		if (iControlInput < startval)
			iControlInput = startval;

		iMaxInt = maxval;

		if (ImGui::SliderInt(cUniqueLabel, &iControlInput, startval, iMaxInt, " "))
		{
			fInput = (float)iControlInput * (fRange / maxval);
			bRet = true;
		}
		if (tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushItemWidth(numericboxwidth);
		strcat(cUniqueLabel, "##i");
		if (ImGui::InputInt(cUniqueLabel, &iControlInput, 0, 0))
		{
			fInput = (float)iControlInput * (fRange / maxval);
			bRet = true;
		}
		if (!pref.iTurnOffEditboxTooltip && tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();

		*v = v_min + fInput;
		return(bRet);
	}

	bool MaxSliderInputFloat2(const char* label, float* v, float v_min, float v_max, const char* tooltip, int startval, float maxval, int numericboxwidth)
	{
		bLastSliderHovered = false;
		if (!label) return(false);
		char cUniqueLabel[256];
		strncpy(cUniqueLabel, label, 240);
		cUniqueLabel[240] = 0;

		float fInput = *v - v_min;
		float fRange = v_max - v_min;
		bool bRet = false;
		float fControlInput, iMaxInt = 100;
		ImGui::PushItemWidth(-10 - 10 - numericboxwidth);
		fControlInput = maxval / fRange * fInput;
		if (fControlInput < startval)
			fControlInput = startval;

		iMaxInt = maxval;

		if (ImGui::SliderFloat(cUniqueLabel, &fControlInput, startval, iMaxInt, " ")) //"%.2f"
		{
			fInput = (float)fControlInput * (fRange / maxval);
			bRet = true;
		}
		if (tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushItemWidth(numericboxwidth);
		strcat(cUniqueLabel, "##i");
		if (ImGui::InputFloat(cUniqueLabel, &fControlInput, 0, 0, 2))
		{
			fInput = (float)fControlInput * (fRange / maxval);
			bRet = true;
		}
		if (tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();

		*v = v_min + fInput;
		return(bRet);
	}

	bool MaxSliderInputRangeFloat(const char* label, float* v, float* v2, float v_min, float v_max, const char* tooltip)
	{
		bLastSliderHovered = false;
		if (!label) return(false);

		char cUniqueLabel[256];
		strncpy(cUniqueLabel, label, 240);
		cUniqueLabel[240] = 0;

		float fInput = *v - v_min;
		float fInput2 = *v2;

		//		if (fInput > fInput2)
		//		{
		//			float ftmp = fInput;
		//			fInput = fInput2;
		//			fInput2 = ftmp;
		//		}

		float fRange = v_max - v_min;
		bool bRet = false;
		int iControlInput, iControlInput2;
		iControlInput = 100.0 / fRange * fInput;
		iControlInput2 = 100.0 / fRange * fInput2;
		float fTmp1 = iControlInput, fTmp2 = iControlInput2;

		//ImGui::SetCursorPos( ImVec2(ImGui::GetCursorPosX(), end_post.y) );
		ImGui::PushItemWidth(30);
		strcat(cUniqueLabel, "##i");
		if (ImGui::InputInt(cUniqueLabel, &iControlInput, 0, 0))
		{
			fInput = (float)iControlInput * (fRange / 100.0);
			bRet = true;
		}
		if (!pref.iTurnOffEditboxTooltip && tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushItemWidth(-10 - 40); //77
		ImVec2 end_post = ImGui::GetCursorPos();
		strcat(cUniqueLabel, "slider");
		if (ImGui::RangeSlider(cUniqueLabel, fTmp1, fTmp2, 100.0f, false))
		{
			fInput = fTmp1 * (fRange / 100.0);
			fInput2 = fTmp2 * (fRange / 100.0);
			bRet = true;
		}
		if (tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushItemWidth(30);
		strcat(cUniqueLabel, "t");
		if (ImGui::InputInt(cUniqueLabel, &iControlInput2, 0, 0))
		{
			fInput2 = (float)iControlInput2 * (fRange / 100.0);
			bRet = true;
		}
		if (!pref.iTurnOffEditboxTooltip && tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();

		//		if (fInput > fInput2)
		//		{
		//			float ftmp = fInput;
		//			fInput = fInput2;
		//			fInput2 = ftmp;
		//		}

		*v = v_min + fInput;
		*v2 = fInput2;
		return(bRet);
	}
#endif

	bool BeginPopupContextItemAGK(const char* str_id, int mouse_button)
	{
		ImGuiWindow* window = GImGui->CurrentWindow;
		ImGuiID id = str_id ? window->GetID(str_id) : window->DC.LastItemId; // If user hasn't passed an ID, we can use the LastItemID. Using LastItemID as a Popup ID won't conflict!
		IM_ASSERT(id != 0);                                                  // You cannot pass a NULL str_id if the last item has no identifier (e.g. a Text() item)
		if (IsMouseReleased(mouse_button) && IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
			OpenPopupEx(id);
		return BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking);
	}


	bool windowTabVisible(void)
	{
		if (!ImGui::IsWindowDocked()) return(true);
		ImGuiWindow* window = GetCurrentWindow();
		if (!window->DockNode) return(true);
		return window->DockTabIsVisible;
	}
	int windowTabFlags(void)
	{
		//DockTabItemStatusFlags
		ImGuiWindow* window = GetCurrentWindow();
		return (int)window->DockTabItemStatusFlags;
	}
	int windowDockNodeId(void)
	{
		//DockTabItemStatusFlags
		ImGuiWindow* window = GetCurrentWindow();
		if (!window->DockNode) return(0);
		return (int)window->DockNode->ID;
	}

	bool bBlurMode = false;
	void SetBlurMode(bool blur)
	{
		bBlurMode = blur;
	}

	bool ImgBtn(int iImageID, const ImVec2& btn_size, const ImVec4& bg_col,
		const ImVec4& drawCol_normal,
		const ImVec4& drawCol_hover,
		const ImVec4& drawCol_Down, int frame_padding, int atlasindex, int atlasrows, int atlascolumns , bool nowhite , bool gratiant ,bool center_image, bool noalpha, bool useownid, bool boost25)
	{

		ImTextureID lpTexture = (ImTextureID) GetImagePointer(iImageID);

		if (!lpTexture) return false;
		int iTexID = iImageID;

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;
		ImVec2 size = ImVec2(btn_size.x, btn_size.y);
		if (btn_size.x == 0 && btn_size.y == 0) 
		{
			size.x = (float)ImageWidth(iImageID);
			size.y = (float)ImageHeight(iImageID);
		}

		ImVec2 uv0 = ImVec2(0, 0);
		ImVec2 uv1 = ImVec2(1, 1);
		if (atlasindex > 0) {
			//atlasrows
			//atlascolumns
			float asx = (float)ImageWidth(iImageID);
			float asy = (float)ImageHeight(iImageID);

			float uvratiox = 1.0f / (asx);
			float uvratioy = 1.0f / (asy);
			float imgsizex = asx / atlasrows;
			float imgsizey = asy / atlascolumns;

			int index_x = (int)fmod(atlasindex - 1, atlasrows);
			int index_y = (atlasindex - 1) / atlasrows; //atlascolumns;

			float uvborderx = uvratiox;
			float uvbordery = uvratioy;
			uvborderx *= (imgsizex / 32);
			uvbordery *= (imgsizey / 32);

			float atlasstartx = (index_x * (imgsizex)) * uvratiox + (uvborderx*0.5f);
			float atlasstarty = (index_y * (imgsizey)) * uvratioy + (uvbordery*0.5f);
			float atlassizex = (imgsizex)* uvratiox - (uvborderx); //0.987
			float atlassizey = (imgsizey)* uvratioy - (uvbordery);

			uv0 = ImVec2(atlasstartx, atlasstarty);
			uv1 = ImVec2(atlasstartx + atlassizex, atlasstarty + atlassizey);
		}
		if (atlascolumns == 9999)
		{
			uv0 = ImVec2(0, 0);
			uv1 = ImVec2(0.5, 1.0);
		}
		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		if(!useownid) PushID(iTexID);
		const ImGuiID id = window->GetID("#image");
		if (!useownid) PopID();

		const ImVec2 padding = (frame_padding >= 0) ? ImVec2((float)frame_padding, (float)frame_padding) : style.FramePadding;
		const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding * 2);
		ImRect image_bb(window->DC.CursorPos + padding, window->DC.CursorPos + padding + size);

		image_bb.Floor(); // Fix Small graphical issue with the THUMBNAIL if non-exact-pixel cords was used.

		//PE: Add the background color. not really needed as most buttons are transparent.
		if (bg_col.w > 0.0f && !nowhite) {

			if (gratiant) {
				ImVec4 bg_fade = bg_col;
				for (int i = (int)image_bb.Min.y; i <= (int)image_bb.Max.y; i++)
				{
					window->DrawList->AddRectFilled(ImVec2(image_bb.Min.x, i), ImVec2(image_bb.Max.x, i + 1), GetColorU32(bg_fade));
					bg_fade = bg_fade + ImVec4(0.0055f, 0.0055f, 0.0055f, 0.0055f);
				}
			}
			else
				window->DrawList->AddRectFilled(image_bb.Min, image_bb.Max, GetColorU32(bg_col));
		}

		ItemSize(bb);
		if (!ItemAdd(bb, id))
			return false;

		bool hovered, held;
		bool pressed = ButtonBehavior(bb, id, &hovered, &held);

		// ZJ: Moved this to behind the image so it doesn't darken it.
		////PE: Add the background color. not really needed as most buttons are transparent.
		//if (bg_col.w > 0.0f && !nowhite) {

		//	if (gratiant) {
		//		ImVec4 bg_fade = bg_col;
		//		for (int i = (int)image_bb.Min.y; i <= (int)image_bb.Max.y; i ++ ) 
		//		{
		//			window->DrawList->AddRectFilled( ImVec2(image_bb.Min.x, i), ImVec2(image_bb.Max.x, i+1), GetColorU32(bg_fade));
		//			bg_fade = bg_fade + ImVec4(0.0055f, 0.0055f, 0.0055f, 0.0055f);
		//		}
		//	} else
		//		window->DrawList->AddRectFilled(image_bb.Min, image_bb.Max, GetColorU32(bg_col));
		//}

		//if (pref.current_style != 3 && nowhite)
		if(pref.current_style > 2 && pref.current_style < 10 && nowhite)
			window->DrawList->AddCallback( (ImDrawCallback) 1 , NULL );

		if (noalpha) {
			window->DrawList->AddCallback((ImDrawCallback)3, NULL);
		}

		if (boost25)
			window->DrawList->AddCallback((ImDrawCallback)5, NULL); //new shader , boost colors.

		if(bBlurMode)
			window->DrawList->AddCallback((ImDrawCallback)6, NULL); //new shader , blur.


		window->DrawList->AddImage(lpTexture, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(
			(hovered && held) ? drawCol_Down : hovered ? drawCol_hover : drawCol_normal));

		if ( pref.current_style > 2 && pref.current_style < 10 && nowhite)
			window->DrawList->AddCallback((ImDrawCallback)2, NULL);
		if (noalpha) {
			window->DrawList->AddCallback((ImDrawCallback)4, NULL);
		}
		if (boost25)
			window->DrawList->AddCallback((ImDrawCallback)2, NULL); //switch shader back to normal shader.

		if (bBlurMode)
			window->DrawList->AddCallback((ImDrawCallback)2, NULL); //switch shader back to normal shader.

		if (pref.current_style == 0 && nowhite) {
			ImTextureID lpTextureRound = (ImTextureID) GetImagePointer(ROUNDING_OVERLAY); //ROUNDING_OVERLAY
			ImVec4 back = ImVec4(1.0, 1.0, 1.0, 1.0);
			window->DrawList->AddImage(lpTextureRound, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(back));
		}
		if (pref.current_style == 1 && nowhite) {
			ImTextureID lpTextureRound = (ImTextureID) GetImagePointer(ROUNDING_OVERLAY); //ROUNDING_OVERLAY
			ImVec4 back = ImVec4(0.0, 0.0, 0.0, 1.0);
			window->DrawList->AddImage(lpTextureRound, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(back));
		}
		if (pref.current_style == 2 && nowhite) {
			ImTextureID lpTextureRound = (ImTextureID) GetImagePointer(ROUNDING_OVERLAY); //ROUNDING_OVERLAY
			ImVec4 back = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
			window->DrawList->AddImage(lpTextureRound, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(back));
		}
		//

		if (pressed) {
			return(true);
		}

		return(pressed);
	}
#ifdef WICKEDENGINE
	bool ImgBtnWicked(void* pMaterial, const ImVec2& btn_size, const ImVec4& bg_col,
		const ImVec4& drawCol_normal,
		const ImVec4& drawCol_hover,
		const ImVec4& drawCol_Down, int frame_padding, int atlasindex, int atlasrows, int atlascolumns, bool nowhite, bool gratiant, bool center_image, bool noalpha, bool useownid)
	{

		ImTextureID lpTexture = (ImTextureID) pMaterial;

		if( (void*) lpTexture == (void*) 1)  return false;
		if (!lpTexture) return false;
		int iTexID = (int) lpTexture; //Just need to be unique for each image.

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;
		ImVec2 size = ImVec2(btn_size.x, btn_size.y);
		if (btn_size.x == 0 && btn_size.y == 0)
		{
			size.x = (float) WickedCall_GetTextureWidth(pMaterial);
			size.y = (float) WickedCall_GetTextureHeight(pMaterial);
		}

		ImVec2 uv0 = ImVec2(0, 0);
		ImVec2 uv1 = ImVec2(1, 1);
		if (atlasindex > 0) {
			//atlasrows
			//atlascolumns
			float asx = (float) WickedCall_GetTextureWidth(pMaterial);
			float asy = (float) WickedCall_GetTextureHeight(pMaterial);

			float uvratiox = 1.0f / (asx);
			float uvratioy = 1.0f / (asy);
			float imgsizex = asx / atlasrows;
			float imgsizey = asy / atlascolumns;

			int index_x = (int)fmod(atlasindex - 1, atlasrows);
			int index_y = (atlasindex - 1) / atlasrows; //atlascolumns;

			float uvborderx = uvratiox;
			float uvbordery = uvratioy;
			uvborderx *= (imgsizex / 32);
			uvbordery *= (imgsizey / 32);

			float atlasstartx = (index_x * (imgsizex)) * uvratiox + (uvborderx*0.5f);
			float atlasstarty = (index_y * (imgsizey)) * uvratioy + (uvbordery*0.5f);
			float atlassizex = (imgsizex)* uvratiox - (uvborderx); //0.987
			float atlassizey = (imgsizey)* uvratioy - (uvbordery);

			uv0 = ImVec2(atlasstartx, atlasstarty);
			uv1 = ImVec2(atlasstartx + atlassizex, atlasstarty + atlassizey);
		}
		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		if (!useownid) PushID(iTexID);
		const ImGuiID id = window->GetID("#image");
		if (!useownid) PopID();

		const ImVec2 padding = (frame_padding >= 0) ? ImVec2((float)frame_padding, (float)frame_padding) : style.FramePadding;
		const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding * 2);
		ImRect image_bb(window->DC.CursorPos + padding, window->DC.CursorPos + padding + size);

		image_bb.Floor(); // Fix Small graphical issue with the THUMBNAIL if non-exact-pixel cords was used.

		ItemSize(bb);
		if (!ItemAdd(bb, id))
			return false;

		bool hovered, held;
		bool pressed = ButtonBehavior(bb, id, &hovered, &held);

		//PE: Add the background color. not really needed as most buttons are transparent.
		if (bg_col.w > 0.0f && !nowhite) {

			if (gratiant) {
				ImVec4 bg_fade = bg_col;
				for (int i = (int)image_bb.Min.y; i <= (int)image_bb.Max.y; i++)
				{
					window->DrawList->AddRectFilled(ImVec2(image_bb.Min.x, i), ImVec2(image_bb.Max.x, i + 1), GetColorU32(bg_fade));
					bg_fade = bg_fade + ImVec4(0.0055f, 0.0055f, 0.0055f, 0.0055f);
				}
			}
			else
				window->DrawList->AddRectFilled(image_bb.Min, image_bb.Max, GetColorU32(bg_col));
		}

		//if (pref.current_style != 3 && nowhite)
		if (pref.current_style > 2 && pref.current_style < 10 && nowhite)
			window->DrawList->AddCallback((ImDrawCallback)1, NULL);

		if (noalpha) {
			window->DrawList->AddCallback((ImDrawCallback)3, NULL);
		}

		window->DrawList->AddImage((ImTextureID)lpTexture, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(
			(hovered && held) ? drawCol_Down : hovered ? drawCol_hover : drawCol_normal));

		if (pref.current_style > 2 && pref.current_style < 10 && nowhite)
			window->DrawList->AddCallback((ImDrawCallback)2, NULL);
		if (noalpha) {
			window->DrawList->AddCallback((ImDrawCallback)4, NULL);
		}

		if (pref.current_style == 0 && nowhite) {
			ImTextureID lpTextureRound = (ImTextureID) GetImagePointer(ROUNDING_OVERLAY); //ROUNDING_OVERLAY
			ImVec4 back = ImVec4(1.0, 1.0, 1.0, 1.0);
			window->DrawList->AddImage(lpTextureRound, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(back));
		}
		if (pref.current_style == 1 && nowhite) {
			ImTextureID lpTextureRound = (ImTextureID) GetImagePointer(ROUNDING_OVERLAY); //ROUNDING_OVERLAY
			ImVec4 back = ImVec4(0.0, 0.0, 0.0, 1.0);
			window->DrawList->AddImage(lpTextureRound, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(back));
		}
		if (pref.current_style == 2 && nowhite) {
			ImTextureID lpTextureRound = (ImTextureID) GetImagePointer(ROUNDING_OVERLAY); //ROUNDING_OVERLAY
			ImVec4 back = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
			window->DrawList->AddImage(lpTextureRound, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(back));
		}
		//

		if (pressed) {
			return(true);
		}

		return(pressed);
	}
#endif

	struct ImGuiViewportDataWin32
	{
		HWND    Hwnd;
		bool    HwndOwned;
		DWORD   DwStyle;
		DWORD   DwExStyle;

		ImGuiViewportDataWin32() { Hwnd = NULL; HwndOwned = false;  DwStyle = DwExStyle = 0; }
		~ImGuiViewportDataWin32() { IM_ASSERT(Hwnd == NULL); }
	};

	//PE: function needed when we do test game.
	void ImGui_GG_HideWindow(ImGuiViewport* viewport)
	{
		ImGuiViewportDataWin32* data = (ImGuiViewportDataWin32*)viewport->PlatformUserData;
		if (data)
		{
			IM_ASSERT(data->Hwnd != 0);
			if (viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing)
				::ShowWindow(data->Hwnd, SW_HIDE);
			else
				::ShowWindow(data->Hwnd, SW_HIDE);
		}
	}
	void ImGui_GG_ShowWindow(ImGuiViewport* viewport)
	{
		ImGuiViewportDataWin32* data = (ImGuiViewportDataWin32*)viewport->PlatformUserData;
		if (data)
		{
			IM_ASSERT(data->Hwnd != 0);
			if (viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing)
				::ShowWindow(data->Hwnd, SW_SHOWNA);
			else
				::ShowWindow(data->Hwnd, SW_SHOW);
		}
	}

	void HideAllViewPortWindows(void)
	{
		ImGuiContext& g = *GImGui;
		ImGuiViewport* main_viewport = GetMainViewport();
		for (int i = 0; i < g.Viewports.Size; i++)
			//PE: not main viewport.
			if (main_viewport != g.Viewports[i]) {
				ImGui_GG_HideWindow(g.Viewports[i]);
			}

	}
	void ShowAllViewPortWindows(void)
	{
		ImGuiContext& g = *GImGui;
		ImGuiViewport* main_viewport = GetMainViewport();
		for (int i = 0; i < g.Viewports.Size; i++)
			//PE: not main viewport.
			if (main_viewport != g.Viewports[i]) {
				ImGui_GG_ShowWindow(g.Viewports[i]);
			}

	}

	void ToggleButton(const char* str_id, bool* v)
	{
		ImVec2 p = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec4 *style_colors = ImGui::GetStyle().Colors;

		float height = ImGui::GetFrameHeight() * 0.8f;
		float bordery = ImGui::GetFrameHeight() * 0.12f;
		float width = height * 1.7f;
		float radius = height * 0.50f;

		p.y += bordery;

		ImGui::InvisibleButton(str_id, ImVec2(width, height));
		if (ImGui::IsItemClicked())
			*v = !*v;

		float t = *v ? 1.0f : 0.0f;

		ImGuiContext& g = *GImGui;
		float ANIM_SPEED = 0.08f;
		if (g.LastActiveId == g.CurrentWindow->GetID(str_id))// && g.LastActiveIdTimer < ANIM_SPEED)
		{
			float t_anim = ImSaturate(g.LastActiveIdTimer / ANIM_SPEED);
			t = *v ? (t_anim) : (1.0f - t_anim);
		}

		ImU32 col_bg;
		if (ImGui::IsItemHovered())
			col_bg = ImGui::GetColorU32(ImLerp(ImVec4(style_colors[ImGuiCol_ButtonHovered]), ImVec4(style_colors[ImGuiCol_PlotHistogram]), t));
		else
			col_bg = ImGui::GetColorU32(ImLerp(ImVec4(style_colors[ImGuiCol_FrameBg]), ImVec4(style_colors[ImGuiCol_PlotHistogram]), t));

		draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);
		draw_list->AddCircleFilled(ImVec2(p.x + radius + t * (width - radius * 2.0f), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));
	}

	bool RoundButton(const char* str_id, ImVec2 size, float radius)
	{
		ImVec2 p = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec4 *style_colors = ImGui::GetStyle().Colors;

		float height = size.y;
		float bordery = ImGui::GetFrameHeight() * 0.12f;
		float width = size.x;
		//radius *= 0.5;
		if (radius > height)
			radius = height;

		bool ret = ImGui::InvisibleButton(str_id, ImVec2(width, height));

		//p.y += bordery;
		if (height > (radius*2.0))
			p.y += ((height - (radius*2.0)) * 0.5);
		if (width > (radius*2.0))
			p.x += ((width - (radius*2.0)) * 0.5);

		ImU32 col_bg;

		if (pref.current_style == 3) {
			col_bg = ImGui::GetColorU32(ImVec4(style_colors[ImGuiCol_Button]));
			if (!ImGui::IsItemHovered())
				col_bg = ImGui::GetColorU32(ImLerp(ImVec4(style_colors[ImGuiCol_ButtonHovered]), ImVec4(style_colors[ImGuiCol_Button]), 0.5));
		}
		else {
			col_bg = ImGui::GetColorU32(ImVec4(style_colors[ImGuiCol_PlotHistogram]));
			if (ImGui::IsItemHovered())
				col_bg = ImGui::GetColorU32(ImLerp(ImVec4(style_colors[ImGuiCol_ButtonHovered]), ImVec4(style_colors[ImGuiCol_PlotHistogram]), 0.5));
		}

		draw_list->AddCircleFilled(ImVec2(p.x + radius, p.y + radius), radius, IM_COL32(0, 0, 0, 128));
		draw_list->AddCircleFilled(ImVec2(p.x + radius, p.y + radius), radius - 1.5f, col_bg);


		return ret;

	}
	//PE: Currently only to be used for InputText , always return true if we have a blinking cursor.
	bool MaxIsItemFocused(void)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;

		//PE: Check if clicked outside item.
		if (g.ActiveId != g.NavId)
			return false;

		//PE: dont consider g.NavDisableHighlight.
		if (g.NavId == 0 || g.NavId != window->DC.LastItemId)
			return false;

		// Special handling for the dummy item after Begin() which represent the title bar or tab. 
		// When the window is collapsed (SkipItems==true) that last item will never be overwritten so we need to detect the case.
		if (window->DC.LastItemId == window->ID && window->WriteAccessed)
			return false;

		return true;
	}


	int rotation_start_index;
	void ImRotateStart()
	{
		rotation_start_index = ImGui::GetWindowDrawList()->VtxBuffer.Size;
	}

	ImVec2 ImRotationCenter()
	{
		ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX); // bounds

		const auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
		for (int i = rotation_start_index; i < buf.Size; i++)
			l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);

		return ImVec2((l.x + u.x) / 2, (l.y + u.y) / 2); // or use _ClipRectStack?
	}

	void ImRotateEnd(float rad, ImVec2 center)
	{
		float s = sin(rad), c = cos(rad);
		center = ImRotate(center, s, c) - center;

		auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
		for (int i = rotation_start_index; i < buf.Size; i++)
			buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
	}



	//PE: https://github.com/nem0/LumixEngine/blob/timeline_gui/external/imgui/imgui_user.inl#L814

	static float s_max_timeline_value = 100.0f;

	bool BeginTimeline(const char* str_id, float max_value)
	{
		s_max_timeline_value = max_value;
		return BeginChild(str_id);
	}


	static const float TIMELINE_RADIUS = 6;


	bool TimelineEvent(const char* str_id, float* values)
	{
		ImGuiWindow* win = GetCurrentWindow();
		const ImU32 inactive_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Button]);
		const ImU32 active_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_ButtonHovered]);
		const ImU32 line_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_ButtonActive]); //ImGuiCol_ColumnActive
		bool changed = false;
		ImVec2 cursor_pos = win->DC.CursorPos;

		for (int i = 0; i < 2; ++i)
		{
			ImVec2 pos = cursor_pos;
			pos.x += win->Size.x * values[i] / s_max_timeline_value + TIMELINE_RADIUS;
			pos.y += TIMELINE_RADIUS;

			SetCursorScreenPos(pos - ImVec2(TIMELINE_RADIUS, TIMELINE_RADIUS));
			PushID(i);
			InvisibleButton(str_id, ImVec2(2 * TIMELINE_RADIUS, 2 * TIMELINE_RADIUS));
			if (IsItemActive() || IsItemHovered())
			{
				ImGui::SetTooltip("%f", values[i]);
				ImVec2 a(pos.x, GetWindowContentRegionMin().y + win->Pos.y);
				ImVec2 b(pos.x, GetWindowContentRegionMax().y + win->Pos.y);
				win->DrawList->AddLine(a, b, line_color);
			}
			if (IsItemActive() && IsMouseDragging(0))
			{
				values[i] += GetIO().MouseDelta.x / win->Size.x * s_max_timeline_value;
				changed = true;
			}
			PopID();
			win->DrawList->AddCircleFilled(
				pos, TIMELINE_RADIUS, IsItemActive() || IsItemHovered() ? active_color : inactive_color);
		}

		ImVec2 start = cursor_pos;
		start.x += win->Size.x * values[0] / s_max_timeline_value + 2 * TIMELINE_RADIUS;
		start.y += TIMELINE_RADIUS * 0.5f;
		ImVec2 end = start + ImVec2(win->Size.x * (values[1] - values[0]) / s_max_timeline_value - 2 * TIMELINE_RADIUS,
			TIMELINE_RADIUS);

		PushID(-1);
		SetCursorScreenPos(start);
		InvisibleButton(str_id, end - start);
		if (IsItemActive() && IsMouseDragging(0))
		{
			values[0] += GetIO().MouseDelta.x / win->Size.x * s_max_timeline_value;
			values[1] += GetIO().MouseDelta.x / win->Size.x * s_max_timeline_value;
			changed = true;
		}
		PopID();

		SetCursorScreenPos(cursor_pos + ImVec2(0, GetTextLineHeightWithSpacing()));

		win->DrawList->AddRectFilled(start, end, IsItemActive() || IsItemHovered() ? active_color : inactive_color);

		if (values[0] > values[1])
		{
			float tmp = values[0];
			values[0] = values[1];
			values[1] = tmp;
		}
		if (values[1] > s_max_timeline_value) values[1] = s_max_timeline_value;
		if (values[0] < 0) values[0] = 0;
		return changed;
	}

	static double s_time_scale = 1;
	static double s_time_offset = 0;


	bool RangeSlider(const char* str_id, float & val1, float & val2, float max_value, bool bDisplayValues)
	{
		float values[2] = { val1 , val2 };

		s_max_timeline_value = max_value;
		ImGuiWindow* win = GetCurrentWindow();
		const ImU32 inactive_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Button]);
		const ImU32 active_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_ButtonHovered]);
		const ImU32 line_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_ButtonActive]);
		bool changed = false;
		ImVec2 cursor_pos = win->DC.CursorPos;
		ImVec2 vContentSize = win->Size;
		//vContentSize.x -= 60.0f;
		float fMinimumButtonSpace = 10.0f;
		vContentSize.x = CalcItemWidth() - 10.0f; //GetContentRegionAvail();
		vContentSize.y = win->Size.y;

		const ImVec2 label_size = CalcTextSize(str_id, NULL, true);
		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = win->GetID(str_id);

		ImVec2 size = CalcItemSize(ImVec2(-10,0), label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);
		vContentSize.y = size.y;
		const ImRect bb(cursor_pos, cursor_pos + ImVec2(vContentSize.x + fMinimumButtonSpace, vContentSize.y) );

		vContentSize.x -= fMinimumButtonSpace;

		if (IsMouseHoveringRect(bb.Min, bb.Max)) {
			//Avtive dont look so good.
			//if (IsMouseDown(0))
			//	win->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBgActive), style.FrameRounding, ImDrawCornerFlags_Bot);
			//else
			win->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBgHovered), style.FrameRounding, ImDrawCornerFlags_Bot);
		}
		else {
			win->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg), style.FrameRounding, ImDrawCornerFlags_Bot);
		}

		const float border_size = g.Style.FrameBorderSize;
		if (border_size > 0.0f)
		{
			win->DrawList->AddRect(bb.Min + ImVec2(1, 1), bb.Max + ImVec2(1, 1), GetColorU32(ImGuiCol_BorderShadow), style.FrameRounding, ImDrawCornerFlags_All, border_size);
			win->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_Border), style.FrameRounding, ImDrawCornerFlags_All, border_size);
		}

		bool bButInvHover = false;
		ImGuiID gad_id[2];

		//PE: Middle bar.
		int iMiddleBarY = 6,iSideBordersX = 1; //2

		iMiddleBarY = vContentSize.y - 4.0;
		ImVec2 start = cursor_pos;
		start.x += vContentSize.x * values[0] / s_max_timeline_value + iSideBordersX * TIMELINE_RADIUS;
		start.y += (vContentSize.y*0.5) - (iMiddleBarY*0.5);
		ImVec2 end = start + ImVec2((vContentSize.x * (values[1] - values[0]) / s_max_timeline_value - iSideBordersX * TIMELINE_RADIUS) + fMinimumButtonSpace - iSideBordersX, iMiddleBarY);
		win->DrawList->AddRectFilled(start, end, IsItemActive() || IsItemHovered() ? active_color : inactive_color,0.0f);

		for (int i = 0; i < 2; ++i)
		{
			ImVec2 pos = cursor_pos;
			float ysize = size.y - (style.FramePadding.y * 2.0f);
			pos.x += vContentSize.x * values[i] / s_max_timeline_value + TIMELINE_RADIUS;
			pos.y += ysize;
			
			if (i == 1) pos.x += fMinimumButtonSpace;

			SetCursorScreenPos(pos - ImVec2(TIMELINE_RADIUS, ysize));
			//PushID(i);
			PushOverrideID(id+i);
			InvisibleButton(str_id, ImVec2(10.0, size.y ));

			gad_id[i] = win->GetID(str_id);

			ImGuiWindow* window = GetCurrentWindow();

			//PE: Disabled the hover values , as we now have a input box.
			if (bDisplayValues)
			{
				if (IsItemActive() || IsItemHovered())
				{
					ImGui::SetTooltip("%.2f", values[i]);
					bButInvHover = true;
					SetHoveredID(0);
					window->DC.LastItemStatusFlags &= ~ImGuiItemStatusFlags_HoveredRect;
				}
			}

			if (IsItemActive() && IsMouseDragging(0))
			{
				//values[i] += (GetIO().MouseDelta.x / (vContentSize.x) ) * s_max_timeline_value;

				//PE: This system works way better, more easy to slide.
				ImVec2 mousepos = ImGui::GetMousePos();
				float range = (bb.Max.x - cursor_pos.x) - (bb.Min.x - cursor_pos.x);
				float val = mousepos.x - cursor_pos.x;
				float newval = (s_max_timeline_value / range) * val;
				values[i] = newval;
				if (values[i] > max_value)
					values[i] = max_value;
				if (values[i] < 0)
					values[i] = 0;
				changed = true;
			}
			if (i == 1 && !bButInvHover)
			{
				if (IsMouseHoveringRect(bb.Min, bb.Max)) {
					SetHoveredID(window->DC.LastItemId);
					window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_HoveredRect;
				}
			}
			PopID();

			pos -= ImVec2(TIMELINE_RADIUS, ysize);
			//win->DrawList->AddCircleFilled(pos, TIMELINE_RADIUS, IsItemActive() || IsItemHovered() ? active_color : inactive_color);
			if(IsItemActive())
				win->DrawList->AddRectFilled(pos, pos + ImVec2(10.0, size.y), GetColorU32(ImGuiCol_SliderGrabActive), style.GrabRounding);
			else
				win->DrawList->AddRectFilled(pos, pos + ImVec2(10.0, size.y), GetColorU32(ImGuiCol_SliderGrab), style.GrabRounding);

		}


		SetCursorScreenPos(start);
		//PushID(-1);
		if (values[1] - values[0] > 2)
		{
			PushOverrideID(id + 3);
			ImGui::SetItemAllowOverlap();
			InvisibleButton(str_id, end - start - ImVec2(2,0) );
			static float fMiddleBarMouseXDown = -1;
			if (IsItemActive())
			{
				if (ImGui::IsMouseDown(0)) // IsMouseDragging(0)
				{
					//float deltax = GetIO().MouseDelta.x / vContentSize.x * s_max_timeline_value;
					ImVec2 mousepos = ImGui::GetMousePos();
					float range = (bb.Max.x - cursor_pos.x) - (bb.Min.x - cursor_pos.x);
					float val = mousepos.x - cursor_pos.x;
					float newval = (s_max_timeline_value / range) * val;
					if (fMiddleBarMouseXDown == -1)
						fMiddleBarMouseXDown = newval;

					float deltax = newval - fMiddleBarMouseXDown;
					fMiddleBarMouseXDown = newval;
					if (values[0] + deltax >= 0 && values[1] + deltax <= (s_max_timeline_value + 1.0))
					{
						values[0] += deltax;
						values[1] += deltax;
						changed = true;
					}
				}
			}
			else {
				if (!ImGui::IsMouseDown(0))
				{
					fMiddleBarMouseXDown = -1;
				}
			}
			PopID();
		}
		//win->DrawList->AddRectFilled(start, end, IsItemActive() || IsItemHovered() ? active_color : inactive_color,2.0f);


		if (values[0] > values[1])
		{
			//Swap active id (and gadget).
			ImGuiContext& g = *GImGui;
			if (g.ActiveId)
			{
				if (g.ActiveId == gad_id[0])
				{
					g.ActiveId = gad_id[1];
					SetActiveID(g.ActiveId, win);
				}
				else if (g.ActiveId == gad_id[1])
				{
					g.ActiveId = gad_id[0];
					SetActiveID(g.ActiveId, win);
				}
			}

			float tmp = values[0];
			values[0] = values[1];
			values[1] = tmp;
		}

		if (values[1] > s_max_timeline_value) values[1] = s_max_timeline_value;
		if (values[0] < 0) values[0] = 0;

//		if(values[1]- values[0] > 2)
//			win->DrawList->AddRectFilled(start, end, IsItemActive() || IsItemHovered() ? active_color : inactive_color, 0.0f);

		char value_buf[64];
		sprintf(value_buf, "%.2f - %.2f", values[0], values[1]);
		if(bDisplayValues)
			RenderTextClipped(bb.Min, bb.Max, value_buf, value_buf+strlen(value_buf), NULL, ImVec2(0.5f, 0.5f));

		SetCursorScreenPos(cursor_pos);
		vContentSize.x += fMinimumButtonSpace;
		vContentSize.x += fMinimumButtonSpace;
		ItemSize(vContentSize, 0);
		SetCursorScreenPos(cursor_pos + ImVec2(0, g.FontSize + g.Style.ItemSpacing.y ));

		val1 = values[0];
		val2 = values[1];
		return changed;
	}



	void EndTimeline()
	{
		ImGuiWindow* win = GetCurrentWindow();

		ImU32 color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Button]);
		ImU32 line_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Border]);
		ImU32 text_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Text]);
		float rounding = GImGui->Style.ScrollbarRounding;
		ImVec2 start(GetWindowContentRegionMin().x + win->Pos.x,
			GetWindowContentRegionMax().y - GetTextLineHeightWithSpacing() + win->Pos.y);
		ImVec2 end = GetWindowContentRegionMax() + win->Pos;

		win->DrawList->AddRectFilled(start, end, color, rounding);

		const int LINE_COUNT = 5;
		const ImVec2 text_offset(0, GetTextLineHeightWithSpacing());
		for (int i = 0; i < LINE_COUNT; ++i)
		{
			ImVec2 a = GetWindowContentRegionMin() + win->Pos + ImVec2(TIMELINE_RADIUS, 0);
			a.x += i * GetWindowContentRegionWidth() / LINE_COUNT;
			ImVec2 b = a;
			b.y = start.y;
			win->DrawList->AddLine(a, b, line_color);
			char tmp[256];
			ImFormatString(tmp, sizeof(tmp), "%.2f", i * s_max_timeline_value / LINE_COUNT);
			win->DrawList->AddText(b, text_color, tmp);
		}

		EndChild();
	}

}


//PE: Styles from AGKS
void myDarkStyle(ImGuiStyle* dst)
{
	ImGui::StyleColorsDark();
	//Small overwrites to dark style.
	ImGuiStyle &st = ImGui::GetStyle();
	st.WindowBorderSize = 2.0f;
	st.WindowPadding = { 4.0f,4.0f };
	st.ScrollbarSize = 18.0;
	st.Colors[ImGuiCol_Separator] = { 0.16f, 0.29f, 0.48f, 0.60f };
	st.Colors[ImGuiCol_Tab] = { 0.29f, 0.29f, 0.29f, 0.86f };
	st.Colors[ImGuiCol_DockingPreview] = ImVec4(0.36f, 0.49f, 0.68f, 0.80f);
	st.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.46f, 0.59f, 0.78f, 0.90f);
	st.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.247f, 0.353f, 0.507f, 0.90f);
	st.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	st.Colors[ImGuiCol_PopupBg] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);

	st.WindowRounding = 3.0f;
	st.ChildRounding = 3.0f;
	st.FrameRounding = 3.0f;

	//st.TabBorderSize = 0.0f;
	//st.TabRounding = 8.0f;

	st.FramePadding = ImVec2(4.0f, 4.0f);

	st.Colors[ImGuiCol_Tab] = { 0.161f, 0.290f, 0.478f, 1.000f };
	st.Colors[ImGuiCol_TabUnfocused] = { 0.161f, 0.290f, 0.478f, 1.000f };
	st.Colors[ImGuiCol_TabUnfocusedActive] = { 0.200f, 0.410f, 0.680f, 1.000f };
	st.Colors[ImGuiCol_TitleBg] = { 0.160f, 0.290f, 0.480f, 1.000f };
	TintCurrentStyle();
}

void myLightStyle(ImGuiStyle* dst)
{
	ImGui::StyleColorsLight();
	//Small overwrites to light style.
	ImGuiStyle &st = ImGui::GetStyle();
	ImVec4* Colors = st.Colors;
	st.WindowBorderSize = 2.0f;
	st.WindowPadding = { 4.0f,4.0f };
	st.ScrollbarSize = 18.0;

	st.WindowRounding = 3.0f;
	st.ChildRounding = 3.0f;
	st.FrameRounding = 3.0f;

	Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.20f, 0.00f, 1.00f); //Also <h1> tags in help.
	TintCurrentStyle();
}

void myStyle(ImGuiStyle* dst)
{
	ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
	ImVec4* Colors = style->Colors;

	Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	//Colors[ImGuiCol_TextHovered] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	//Colors[ImGuiCol_TextActive] = ImVec4(1.00f, 1.00f, 0.00f, 1.00f);
	Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
	Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
	Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
	Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
	Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
	Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
	Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
	Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
	Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
	Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
	//Colors[ImGuiCol_ComboBg] = ImVec4(0.86f, 0.86f, 0.86f, 0.99f);
	Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	Colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
	Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

//	Colors[ImGuiCol_Column] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
//	Colors[ImGuiCol_ColumnHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
//	Colors[ImGuiCol_ColumnActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

	Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
	Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	//Colors[ImGuiCol_CloseButton] = ImVec4(0.59f, 0.59f, 0.59f, 0.50f);
	//Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
	//Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
	Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f); //Also <h1> tags in help.
	Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	//Colors[ImGuiCol_TooltipBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
	Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

	style->Colors[ImGuiCol_DragDropTarget] = ImVec4(0.58f, 0.58f, 0.58f, 0.90f);
	TintCurrentStyle();
}

void myStyleBlue(ImGuiStyle* dst)
{
	myStyle2(dst);
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_TextDisabled] = ImVec4(0.39f, 0.56f, 0.68f, 1.00f);
	colors[ImGuiCol_Text] = ImVec4(0.95f, 0.98f, 1.00f, 1.00f);
	//colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.16f, 0.22f, 0.97f); //Add a little transparent.
	colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.16f, 0.22f, 0.9f); //Add a little transparent.
	colors[ImGuiCol_ChildBg] = ImVec4(0.11f, 0.16f, 0.22f, 1.0f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.16f, 0.22f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.58f, 0.58f, 0.58f, 1.00f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.11f, 0.16f, 0.23f, 1.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.11f, 0.16f, 0.23f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.23f, 0.33f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.11f, 0.16f, 0.23f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.11f, 0.16f, 0.23f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.11f, 0.16f, 0.23f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.26f, 0.35f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.11f, 0.16f, 0.23f, 1.00f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.95f, 0.98f, 1.00f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.36f, 0.48f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.11f, 0.16f, 0.23f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.61f, 0.63f, 0.69f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.61f, 0.63f, 0.69f, 1.00f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.61f, 0.63f, 0.69f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.16f, 0.22f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.38f, 0.51f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);

	// Wicked renders first, IMGUI last, let Wicked renderings through! 
#ifdef WICKEDENGINE
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);
#else
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
#endif

	colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 1.0f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.6f);

	colors[ImGuiCol_DragDropTarget] = ImVec4(0.58f, 0.58f, 0.58f, 0.90f);

	//PE: Darker tabs so they dont look like buttons.
	float transparent = 0.55;
	colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.16f, 0.22f, transparent);
	colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.38f, 0.51f, transparent);
	colors[ImGuiCol_TabActive] = ImVec4(0.22f, 0.43f, 0.57f, transparent);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.22f, 0.43f, 0.57f, transparent);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.22f, 0.43f, 0.57f, transparent);

	//ImGui::GetStyle().TabBorderSize = 1.0f;
	//ImGui::GetStyle().TabRounding = 1.0f;


	TintCurrentStyle();

}


void myStyle2(ImGuiStyle* dst)
{
	auto *style = (dst ? dst : &ImGui::GetStyle());
	style->WindowRounding = 5.3f;
	style->GrabRounding = style->FrameRounding = 2.3f;
	style->ScrollbarRounding = 5.0f;
	style->FrameBorderSize = 1.0f;
	style->ItemSpacing.y = 6.5f;

	style->ScrollbarSize = 16.0;

	style->Colors[ImGuiCol_Text] = { 0.78f, 0.78f, 0.78f, 1.00f };
	style->Colors[ImGuiCol_TextDisabled] = { 0.55f, 0.55f, 0.55f, 1.00f };

	style->Colors[ImGuiCol_WindowBg] = { 0.23f, 0.23f, 0.23f, 0.75f }; //0.98f
	style->Colors[ImGuiCol_ChildBg] = { 0.23529413f, 0.24705884f, 0.25490198f, 0.00f };

	style->Colors[ImGuiCol_PopupBg] = { 0.31f, 0.32f, 0.34f, 1.0f }; //0.94

	style->Colors[ImGuiCol_Border] = { 0.33333334f, 0.33333334f, 0.33333334f, 0.50f };
	style->Colors[ImGuiCol_BorderShadow] = { 0.15686275f, 0.15686275f, 0.15686275f, 0.00f };

	style->Colors[ImGuiCol_FrameBg] = { 0.16862746f, 0.16862746f, 0.16862746f, 0.64f };

	style->Colors[ImGuiCol_FrameBgHovered] = { 0.453125f, 0.67578125f, 0.99609375f, 0.77f };
	style->Colors[ImGuiCol_FrameBgActive] = { 0.47058827f, 0.47058827f, 0.47058827f, 0.77f };

	style->Colors[ImGuiCol_TitleBg] = { 0.04f, 0.04f, 0.04f, 1.00f };
	style->Colors[ImGuiCol_TitleBgCollapsed] = { 0.16f, 0.29f, 0.48f, 1.00f };
	style->Colors[ImGuiCol_TitleBgActive] = { 0.00f, 0.00f, 0.00f, 1.0f };

	style->Colors[ImGuiCol_MenuBarBg] = { 0.27058825f, 0.28627452f, 0.2901961f, 0.92f };

	style->Colors[ImGuiCol_ScrollbarBg] = { 0.195f, 0.195f, 0.195f, 0.60f };
	style->Colors[ImGuiCol_ScrollbarGrab] = { 0.39f, 0.39f, 0.39f, 0.51f };

	style->Colors[ImGuiCol_ScrollbarGrabHovered] = { 0.21960786f, 0.30980393f, 0.41960788f, 1.00f };
	style->Colors[ImGuiCol_ScrollbarGrabActive] = { 0.13725491f, 0.19215688f, 0.2627451f, 0.91f };
	// style->Colors[ImGuiCol_ComboBg]               = {0.1f, 0.1f, 0.1f, 0.99f};
	style->Colors[ImGuiCol_CheckMark] = { 0.90f, 0.90f, 0.90f, 0.83f };
	style->Colors[ImGuiCol_SliderGrab] = { 0.70f, 0.70f, 0.70f, 0.62f };
	style->Colors[ImGuiCol_SliderGrabActive] = { 0.30f, 0.30f, 0.30f, 0.84f };
	style->Colors[ImGuiCol_Button] = { 0.33333334f, 0.3529412f, 0.36078432f, 0.49f };
	style->Colors[ImGuiCol_ButtonHovered] = { 0.21960786f, 0.30980393f, 0.41960788f, 1.00f };
	style->Colors[ImGuiCol_ButtonActive] = { 0.13725491f, 0.19215688f, 0.2627451f, 1.00f };
	style->Colors[ImGuiCol_Header] = { 0.33333334f, 0.3529412f, 0.36078432f, 0.53f };
	style->Colors[ImGuiCol_HeaderHovered] = { 0.453125f, 0.67578125f, 0.99609375f, 0.67f };
	style->Colors[ImGuiCol_HeaderActive] = { 0.47058827f, 0.47058827f, 0.47058827f, 0.67f };
	style->Colors[ImGuiCol_Separator] = { 0.31640625f, 0.31640625f, 0.31640625f, 1.00f };
	style->Colors[ImGuiCol_SeparatorHovered] = { 0.31640625f, 0.31640625f, 0.31640625f, 1.00f };
	style->Colors[ImGuiCol_SeparatorActive] = { 0.31640625f, 0.31640625f, 0.31640625f, 1.00f };
	style->Colors[ImGuiCol_ResizeGrip] = { 1.00f, 1.00f, 1.00f, 0.85f };
	style->Colors[ImGuiCol_ResizeGripHovered] = { 1.00f, 1.00f, 1.00f, 0.60f };
	style->Colors[ImGuiCol_ResizeGripActive] = { 1.00f, 1.00f, 1.00f, 0.90f };
	style->Colors[ImGuiCol_PlotLines] = { 0.61f, 0.61f, 0.61f, 1.00f };
	style->Colors[ImGuiCol_PlotLinesHovered] = { 1.00f, 0.43f, 0.35f, 1.00f };
	style->Colors[ImGuiCol_PlotHistogram] = { 0.90f, 0.70f, 0.00f, 1.00f }; //Also <h1> tags in help.
	style->Colors[ImGuiCol_PlotHistogramHovered] = { 1.00f, 0.60f, 0.00f, 1.00f };
	style->Colors[ImGuiCol_TextSelectedBg] = { 0.18431373f, 0.39607847f, 0.79215693f, 0.90f };

	//Still need to be set.
	style->Colors[ImGuiCol_Tab] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	style->Colors[ImGuiCol_TabHovered] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_TabActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	style->Colors[ImGuiCol_TabUnfocused] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	style->Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	style->Colors[ImGuiCol_DockingPreview] = ImVec4(0.38f, 0.48f, 0.60f, 1.00f);

	// Wicked renders first, IMGUI last, let Wicked renderings through! 
	#ifdef WICKEDENGINE
	 style->Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);
	#else
	 style->Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	#endif

	style->Colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	style->Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
	style->Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 1.0f);
	style->Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.6f);


	style->Colors[ImGuiCol_DragDropTarget] = ImVec4(0.58f, 0.58f, 0.58f, 0.90f);
	TintCurrentStyle();
}


void myStyle2_colors_only(void)
{
	auto *style = &ImGui::GetStyle();

	style->Colors[ImGuiCol_Text] = { 0.78f, 0.78f, 0.78f, 1.00f };
	style->Colors[ImGuiCol_TextDisabled] = { 0.55f, 0.55f, 0.55f, 1.00f };
	style->Colors[ImGuiCol_WindowBg] = { 0.23f, 0.23f, 0.23f, 0.75f }; //0.98f
	style->Colors[ImGuiCol_ChildBg] = { 0.23529413f, 0.24705884f, 0.25490198f, 0.00f };
	style->Colors[ImGuiCol_PopupBg] = { 0.31f, 0.32f, 0.34f, 1.0f }; //0.94
	style->Colors[ImGuiCol_Border] = { 0.33333334f, 0.33333334f, 0.33333334f, 0.50f };
	style->Colors[ImGuiCol_BorderShadow] = { 0.15686275f, 0.15686275f, 0.15686275f, 0.00f };
	style->Colors[ImGuiCol_FrameBg] = { 0.16862746f, 0.16862746f, 0.16862746f, 0.64f };
	style->Colors[ImGuiCol_FrameBgHovered] = { 0.453125f, 0.67578125f, 0.99609375f, 0.77f };
	style->Colors[ImGuiCol_FrameBgActive] = { 0.47058827f, 0.47058827f, 0.47058827f, 0.77f };
	style->Colors[ImGuiCol_TitleBg] = { 0.04f, 0.04f, 0.04f, 1.00f };
	style->Colors[ImGuiCol_TitleBgCollapsed] = { 0.16f, 0.29f, 0.48f, 1.00f };
	style->Colors[ImGuiCol_TitleBgActive] = { 0.00f, 0.00f, 0.00f, 1.0f };
	style->Colors[ImGuiCol_MenuBarBg] = { 0.27058825f, 0.28627452f, 0.2901961f, 0.92f };
	style->Colors[ImGuiCol_ScrollbarBg] = { 0.195f, 0.195f, 0.195f, 0.60f };
	style->Colors[ImGuiCol_ScrollbarGrab] = { 0.39f, 0.39f, 0.39f, 0.51f };
	style->Colors[ImGuiCol_ScrollbarGrabHovered] = { 0.21960786f, 0.30980393f, 0.41960788f, 1.00f };
	style->Colors[ImGuiCol_ScrollbarGrabActive] = { 0.13725491f, 0.19215688f, 0.2627451f, 0.91f };
	style->Colors[ImGuiCol_CheckMark] = { 0.90f, 0.90f, 0.90f, 0.83f };
	style->Colors[ImGuiCol_SliderGrab] = { 0.70f, 0.70f, 0.70f, 0.62f };
	style->Colors[ImGuiCol_SliderGrabActive] = { 0.30f, 0.30f, 0.30f, 0.84f };
	style->Colors[ImGuiCol_Button] = { 0.33333334f, 0.3529412f, 0.36078432f, 0.49f };
	style->Colors[ImGuiCol_ButtonHovered] = { 0.21960786f, 0.30980393f, 0.41960788f, 1.00f };
	style->Colors[ImGuiCol_ButtonActive] = { 0.13725491f, 0.19215688f, 0.2627451f, 1.00f };
	style->Colors[ImGuiCol_Header] = { 0.33333334f, 0.3529412f, 0.36078432f, 0.53f };
	style->Colors[ImGuiCol_HeaderHovered] = { 0.453125f, 0.67578125f, 0.99609375f, 0.67f };
	style->Colors[ImGuiCol_HeaderActive] = { 0.47058827f, 0.47058827f, 0.47058827f, 0.67f };
	style->Colors[ImGuiCol_Separator] = { 0.31640625f, 0.31640625f, 0.31640625f, 1.00f };
	style->Colors[ImGuiCol_SeparatorHovered] = { 0.31640625f, 0.31640625f, 0.31640625f, 1.00f };
	style->Colors[ImGuiCol_SeparatorActive] = { 0.31640625f, 0.31640625f, 0.31640625f, 1.00f };
	style->Colors[ImGuiCol_ResizeGrip] = { 1.00f, 1.00f, 1.00f, 0.85f };
	style->Colors[ImGuiCol_ResizeGripHovered] = { 1.00f, 1.00f, 1.00f, 0.60f };
	style->Colors[ImGuiCol_ResizeGripActive] = { 1.00f, 1.00f, 1.00f, 0.90f };
	style->Colors[ImGuiCol_PlotLines] = { 0.61f, 0.61f, 0.61f, 1.00f };
	style->Colors[ImGuiCol_PlotLinesHovered] = { 1.00f, 0.43f, 0.35f, 1.00f };
	style->Colors[ImGuiCol_PlotHistogram] = { 0.90f, 0.70f, 0.00f, 1.00f }; //Also <h1> tags in help.
	style->Colors[ImGuiCol_PlotHistogramHovered] = { 1.00f, 0.60f, 0.00f, 1.00f };
	style->Colors[ImGuiCol_TextSelectedBg] = { 0.18431373f, 0.39607847f, 0.79215693f, 0.90f };
	style->Colors[ImGuiCol_Tab] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	style->Colors[ImGuiCol_TabHovered] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_TabActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	style->Colors[ImGuiCol_TabUnfocused] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	style->Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	style->Colors[ImGuiCol_DockingPreview] = ImVec4(0.38f, 0.48f, 0.60f, 1.00f);

	// Wicked renders first, IMGUI last, let Wicked renderings through! 
#ifdef WICKEDENGINE
//	style->Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);
#else
//	style->Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
#endif

	style->Colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	style->Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
	style->Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 1.0f);
	style->Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.6f);
	style->Colors[ImGuiCol_DragDropTarget] = ImVec4(0.58f, 0.58f, 0.58f, 0.90f);

	//TintCurrentStyle();
}

void myStyle3(ImGuiStyle* dst)
{
	ImGuiStyle &st = ImGui::GetStyle();
	st.FrameBorderSize = 1.0f;
	st.FramePadding = ImVec2(4.0f, 2.0f);
	st.ItemSpacing = ImVec2(8.0f, 2.0f);
	st.WindowBorderSize = 2.0f;
	//	st.TabBorderSize = 1.0f;
	st.WindowRounding = 1.0f;
	st.ChildRounding = 1.0f;
	st.FrameRounding = 1.0f;
	st.ScrollbarRounding = 1.0f;
	st.ScrollbarSize = 18.0;
	st.GrabRounding = 1.0f;
	//	st.TabRounding = 1.0f;

	//	st.TabBorderSize = 5.0f;
	//	st.TabRounding = 2.0f;


	// Setup style
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 0.95f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.12f, 0.12f, 0.941f);
	colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.031f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.94f);
	colors[ImGuiCol_Border] = ImVec4(0.53f, 0.53f, 0.53f, 0.25f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.0f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.22f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 0.53f);

	colors[ImGuiCol_TitleBg] = ImVec4(0.114f, 0.191f, 0.199f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.203f, 0.335f, 0.348f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.0f);

	colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.48f, 0.48f, 0.48f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.79f, 0.79f, 0.79f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.48f, 0.47f, 0.47f, 0.91f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.55f, 0.55f, 0.62f);
	colors[ImGuiCol_Button] = ImVec4(0.50f, 0.50f, 0.50f, 0.63f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.67f, 0.67f, 0.68f, 0.63f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.26f, 0.26f, 0.26f, 0.63f);
	colors[ImGuiCol_Header] = ImVec4(0.54f, 0.54f, 0.54f, 0.58f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.64f, 0.65f, 0.65f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.25f, 0.80f);
	colors[ImGuiCol_Separator] = ImVec4(0.58f, 0.58f, 0.58f, 0.50f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.64f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.81f, 0.81f, 0.81f, 0.64f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.5f, 0.5f, 0.5f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.87f, 0.87f, 0.87f, 0.74f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.87f, 0.87f, 0.87f, 0.74f);
	colors[ImGuiCol_Tab] = ImVec4(0.114f, 0.161f, 0.200f, 0.86f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.134f, 0.181f, 0.220f, 0.86f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.114f, 0.161f, 0.200f, 0.86f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.124f, 0.171f, 0.210f, 1.0f);
	colors[ImGuiCol_DockingPreview] = ImVec4(0.38f, 0.48f, 0.60f, 1.00f);
	#ifdef WICKEDENGINE
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);
	#else
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	#endif
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.68f, 0.68f, 0.68f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.77f, 0.33f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.87f, 0.55f, 0.08f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.47f, 0.60f, 0.76f, 0.47f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.58f, 0.58f, 0.58f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	TintCurrentStyle();
}


void TintColor(ImVec4 & color, int iModeColor = 0)
{
	ImVec4 tint = pref.tint_style;
	ImVec4 shade = pref.shade_style;
	ImVec4 title = pref.title_style;

	if (pref.iEnableCustomColors || (pref.current_style >= 10 && pref.current_style != 25 ))
	{
		//PE: pref.current_style == 25 (Blue) is a little special as it is used as a new "main" color setup.
		bool bUseGrayScale = true;
		float grayscale = (color.x + color.y + color.z) / 3.0;
		if (iModeColor == 2)
		{
			bool bNoTitle = false;
			if (title.x <= 0.008 && title.y <= 0.008 && title.z <= 0.008 && pref.current_style == 25)
				bNoTitle = true;
			if (!bNoTitle)
			{
				if (grayscale > 0.5)
				{
					//Darken.
					color.x = grayscale * (title.x);
					color.y = grayscale * (title.y);
					color.z = grayscale * (title.z);
				}
				else {
					//Lighting
					color.x = grayscale + title.x + title.x / 3;
					color.y = grayscale + title.y + title.y / 3;
					color.z = grayscale + title.z + title.z / 3;
				}
			}
		}
		else if (iModeColor == 1)
		{

			bool bNoTint = false;
			if (tint.x >= 0.998 && tint.y >= 0.998 && tint.z >= 0.998 && pref.current_style == 25)
				bNoTint = true;

			if(!bNoTint)
			{
				//PE: Dont grayscale "text" we need the yellow/highlight colors.
				if (grayscale > 0.5)
				{
					//Darken.
					color.x = color.x * (tint.x);
					color.y = color.y * (tint.y);
					color.z = color.z * (tint.z);
				}
				else {
					//Lighting
					color.x = color.x + tint.x + tint.x / 3;
					color.y = color.y + tint.y + tint.y / 3;
					color.z = color.z + tint.z + tint.z / 3;
				}
			}
		}
		else
		{

			bool bNoShade = false;
			if (shade.x <= 0.008 && shade.y <= 0.008 && shade.z <= 0.008 && pref.current_style == 25)
				bNoShade = true;

			if (!bNoShade)
			{
				if (bUseGrayScale)
				{
					if (grayscale > 0.5)
					{
						//Darken.
						color.x = grayscale * (shade.x);
						color.y = grayscale * (shade.y);
						color.z = grayscale * (shade.z);
					}
					else {
						//Lighting
						color.x = grayscale + shade.x + shade.x / 3;
						color.y = grayscale + shade.y + shade.y / 3;
						color.z = grayscale + shade.z + shade.z / 3;
					}
				}
				else
				{
					if (grayscale > 0.5)
					{
						//Darken.
						color.x = color.x * (shade.x);
						color.y = color.y * (shade.y);
						color.z = color.z * (shade.z);
					}
					else {
						//Lighting
						color.x = color.x + shade.x + shade.x / 3;
						color.y = color.y + shade.y + shade.y / 3;
						color.z = color.z + shade.z + shade.z / 3;
					}
				}
			}
		}
	}

	if (pref.iTurnOffUITransparent != 0)
		color.w = 1.0f;

}

//Text
//CheckMark
//ResizeGrip
//NavWindow ?
//PlotHistogram

void TintCurrentStyle( void )
{
	auto *style = &ImGui::GetStyle();

	//Light
	TintColor(style->Colors[ImGuiCol_Text],1);
	TintColor(style->Colors[ImGuiCol_CheckMark],1);
	TintColor(style->Colors[ImGuiCol_ResizeGrip],1);
	TintColor(style->Colors[ImGuiCol_PlotHistogram],1);
	TintColor(style->Colors[ImGuiCol_SliderGrab],1);


	//Highlight , title and tabs.
	TintColor(style->Colors[ImGuiCol_TitleBg],2);
	TintColor(style->Colors[ImGuiCol_TitleBgCollapsed], 2);
	TintColor(style->Colors[ImGuiCol_TitleBgActive], 2);
	TintColor(style->Colors[ImGuiCol_Tab], 2);
	TintColor(style->Colors[ImGuiCol_TabHovered], 2);
	TintColor(style->Colors[ImGuiCol_TabActive], 2);
	TintColor(style->Colors[ImGuiCol_TabUnfocused], 2);
	TintColor(style->Colors[ImGuiCol_TabUnfocusedActive], 2);


	//Dark
	TintColor(style->Colors[ImGuiCol_TextDisabled]);
	TintColor(style->Colors[ImGuiCol_WindowBg]);
	TintColor(style->Colors[ImGuiCol_ChildBg]);
	TintColor(style->Colors[ImGuiCol_PopupBg]);
	TintColor(style->Colors[ImGuiCol_Border]);
	TintColor(style->Colors[ImGuiCol_BorderShadow]);
	TintColor(style->Colors[ImGuiCol_FrameBg]);
	TintColor(style->Colors[ImGuiCol_FrameBgHovered]);
	TintColor(style->Colors[ImGuiCol_FrameBgActive]);
	TintColor(style->Colors[ImGuiCol_MenuBarBg]);
	TintColor(style->Colors[ImGuiCol_ScrollbarBg]);
	TintColor(style->Colors[ImGuiCol_ScrollbarGrab]);
	TintColor(style->Colors[ImGuiCol_ScrollbarGrabHovered]);
	TintColor(style->Colors[ImGuiCol_ScrollbarGrabActive]);

	TintColor(style->Colors[ImGuiCol_SliderGrabActive]);
	TintColor(style->Colors[ImGuiCol_Button]);
	TintColor(style->Colors[ImGuiCol_ButtonHovered]);
	TintColor(style->Colors[ImGuiCol_ButtonActive]);
	TintColor(style->Colors[ImGuiCol_Header]);
	TintColor(style->Colors[ImGuiCol_HeaderHovered]);
	TintColor(style->Colors[ImGuiCol_HeaderActive]);
	TintColor(style->Colors[ImGuiCol_Separator]);
	TintColor(style->Colors[ImGuiCol_SeparatorHovered]);
	TintColor(style->Colors[ImGuiCol_SeparatorActive]);
	TintColor(style->Colors[ImGuiCol_ResizeGripHovered]);
	TintColor(style->Colors[ImGuiCol_ResizeGripActive]);
	TintColor(style->Colors[ImGuiCol_PlotLines]);
	TintColor(style->Colors[ImGuiCol_PlotLinesHovered]);
	TintColor(style->Colors[ImGuiCol_PlotHistogramHovered]);
	TintColor(style->Colors[ImGuiCol_TextSelectedBg]);

	//Still need to be set.
	TintColor(style->Colors[ImGuiCol_DockingPreview]);

	// Wicked renders first, IMGUI last, let Wicked renderings through! 
#ifdef WICKEDENGINE
	//style->Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
#else
	TintColor(style->Colors[ImGuiCol_DockingEmptyBg]);
#endif

	TintColor(style->Colors[ImGuiCol_NavHighlight]);
	TintColor(style->Colors[ImGuiCol_NavWindowingHighlight]);
	TintColor(style->Colors[ImGuiCol_NavWindowingDimBg]);
	TintColor(style->Colors[ImGuiCol_ModalWindowDimBg]);
	TintColor(style->Colors[ImGuiCol_DragDropTarget]);

#ifdef WICKEDENGINE
	//PE: To prevent flicker when changing tint, this code has been added to the main loop.
	ImGui::GetStyle().Colors[ImGuiCol_ChildBg] = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
#endif
}



ImFont* defaultfont;
ImFont* customfont;
ImFont* customfontlarge;
ImFont* tmpfont;

void ChangeGGFont(const char *cpcustomfont, int iIDEFontSize)
{
	//MessageBoxA(NULL, "ChangeGGFont", "ChangeGGFont", 0);

	
	static const ImWchar Generic_ranges_all[] =
	{
		0x0020, 0x00FF, // Basic Latin + Latin Supplement
		0x0100, 0x017F,	//0100 � 017F  	Latin Extended-A
		0x0180, 0x024F,	//0180 � 024F  	Latin Extended-B
		0,
	};

	//PE: Add all lang.
//	static const ImWchar Generic_ranges_everything[] =
//	{
//	   0x0020, 0xFFFF, // Everything test.
//	   0,
//	};

	//PE: Optimize exclude Phonetic Extensions (1D00 � 1D7F) , phonetic lettering (japan,china), Greak/Latin Extended, block elements,Dingbats,...
	static const ImWchar Generic_ranges_everything[] =
	{
	   0x0020, 0x07FF, // unicode 0x800-0x900 not defined so exclude from here.
	   0,
	};


	float FONTUPSCALE = 2.0; //Font upscaling.

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.Fonts->Clear();

	if (FileExist((char*)cpcustomfont))
	{

		customfont = io.Fonts->AddFontFromFileTTF(cpcustomfont, iIDEFontSize*FONTUPSCALE, NULL, &Generic_ranges_everything[0]); //Set as default font.
		customfontlarge = io.Fonts->AddFontFromFileTTF(cpcustomfont, 60, NULL, &Generic_ranges_everything[0]); //Set as default font.

#ifdef IMGUIAL_FONTS_MATERIAL_DESIGN
		int ttf_size;
		const void* ttf_data = ImGuiAl::Fonts::GetCompressedData(ImGuiAl::Fonts::kMaterialDesign, &ttf_size);

		if (ttf_data) {
			static const ImWchar ranges[] = { ICON_MIN_MD, ICON_MAX_MD, 0 };
			ImFontConfig config;
			config.MergeMode = true;
			config.PixelSnapH = true;
			if (pref.bUseUpscaling)
				config.GlyphOffset.y += 7.0f;
			else
				config.GlyphOffset.y += 3.5f;
			customfont = io.Fonts->AddFontFromMemoryCompressedTTF(ttf_data, ttf_size, iIDEFontSize*FONTUPSCALE, &config, ranges);
		}

#endif
#ifdef IMGUIAL_FONTS_FONT_AWESOME
		//Not used yet.
		static const ImWchar ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		ImFontConfig config;
		config.MergeMode = true;
		config.PixelSnapH = true;
		config.GlyphOffset.y += 0.5f;
		ttf_data = ImGuiAl::Fonts::GetCompressedData(ImGuiAl::Fonts::kFontAwesome, &ttf_size);
		if (ttf_data)
			io.Fonts->AddFontFromMemoryCompressedTTF(ttf_data, ttf_size, iIDEFontSize*FONTUPSCALE, &config, ranges);
#endif
		io.FontGlobalScale = 1.0f / FONTUPSCALE;
	}
	else
	{
		customfont = io.Fonts->AddFontDefault();
	}
	defaultfont = io.Fonts->AddFontDefault();

	#ifdef STORYBOARD
	//Add all fonts from:

	extern std::vector< std::pair<ImFont*, std::string>> StoryboardFonts;

	LPSTR pOldDir = GetDir();
	char destination[MAX_PATH];
	strcpy(destination, "editors\\templates\\fonts\\");
	SetDir(destination);
	ChecklistForFiles();
	SetDir(pOldDir);
	DARKSDK LPSTR ChecklistString(int iIndex);
	DARKSDK int ChecklistQuantity(void);
	for (int c = 1; c <= ChecklistQuantity(); c++)
	{
		char *file = ChecklistString(c);
		if (file)
		{
			if (strlen(file) > 4)
			{
				if (strnicmp(file + strlen(file) - 4, ".ttf", 4) == NULL || strnicmp(file + strlen(file) - 4, ".otf", 4) == NULL)
				{
					//Add font.
					char path[MAX_PATH];
					strcpy(path, destination);
					strcat(path, file);
					const char *pestrcasestr(const char *arg1, const char *arg2);
					if( pestrcasestr(file,"arial"))
						tmpfont = io.Fonts->AddFontFromFileTTF(path, 60, NULL, &Generic_ranges_everything[0]); //Add font
					else
						tmpfont = io.Fonts->AddFontFromFileTTF(path, 60 , NULL, &Generic_ranges_all[0]); //Add font
					StoryboardFonts.push_back(std::make_pair(tmpfont,file));
				}
			}
		}
	}
	SetDir(pOldDir);
	#endif


	ImGui_CreateDeviceObjects();

}

struct case_insensitive_less : public std::binary_function< char, char, bool >
{
	bool operator () (char x, char y) const
	{
		return toupper(static_cast< unsigned char >(x)) <
			toupper(static_cast< unsigned char >(y));
	}
};

bool NoCaseLess(const std::string &a, const std::string &b)
{
	return std::lexicographical_compare(a.begin(), a.end(),
		b.begin(), b.end(), case_insensitive_less());
}


/* done in cStr
void replaceAll(std::string& str, const std::string& from, const std::string& to)
{
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}
*/

//
const char *pestrcasestr(const char *arg1, const char *arg2);

//##########################################################
//#### Recursive get all files/folders inside a folder. ####
//##########################################################

#include "cStr.h"
class cFolderItem
{
public:
	struct sFolderFiles {
		sFolderFiles * m_dropptr; //Need to be the first entry for drag/drop.
		cStr m_sName;
		cStr m_sNameFinal;
		cStr m_sPath;
		cStr m_sFolder;
		UINT iFlags;
		int iPreview; //Preview image.
		int iBigPreview; //Preview image.
		int id;
		int iAnimationFrom = 0;
		bool bPreviewProcessed;
		long last_used;
		bool bSorted;
		bool bFavorite;
		bool bAvailableInFreeTrial;
		time_t m_tFileModify;
		cstr m_Backdrop;
		sFolderFiles * m_pNext;
		sFolderFiles * m_pNextTime;
		sFolderFiles * m_pCustomSort;
		cFolderItem *pNewFolder;
		cStr m_sBetterSearch;
		cStr m_sPhoneticSearch;
		int uniqueId;
		#ifdef WICKEDENGINE
		bool m_bFPELoaded;
		cstr m_sFPEModel;
		cstr m_sFPETextured;
		cstr m_sFPEKeywords;
		cstr m_sFPEDBOFile;
		cstr m_sDLuaDescription;
		float m_fDLuaHeight;
		int m_iFPEDBOFileSize;
		bool m_bIsCharacterCreator;
		bool m_bIsGroupObject;
		int iType;
		#endif
	};
	cStr m_sFolder;
	cStr m_sFolderFullPath;
	int m_iEntityOffset;
	cFolderItem *m_pNext;
	cFolderItem *m_pSubFolder;
	sFolderFiles * m_pFirstFile;
	bool m_bFilesRead;
	bool visible;
	bool alwaysvisible;
	bool deletethisentry;
	bool bIsCustomFolder;
	char cfolder[256]; //PE: Only for faster sorting.
	time_t m_tFolderModify;
	float m_fLastTimeUpdate;
	UINT iFlags;
	int count;
	int iType;
	cFolderItem() { m_pNext = 0; iType = 0; iFlags = 0; m_bFilesRead = false; m_pFirstFile = NULL; m_pNext = NULL; m_pSubFolder = NULL; m_fLastTimeUpdate = 0; m_iEntityOffset = 0; }
	~cFolderItem() { }
};


//#########################################
//#### PE: Sort for files and folders. ####
//#########################################

int cstring_cmp_folder(const void *a, const void *b)
{
	struct cFolderItem * ia = *(struct cFolderItem **) a;
	struct cFolderItem * ib = *(struct cFolderItem **) b;
	if (!ia) return(0);
	if (!ib) return(0);
	return _stricmp(ia->cfolder, ib->cfolder);
}

cFolderItem MainEntityList;
int iTotalFolders = 0;
int iTotalFiles = 0;
#include "cStr.h"

// seemingly not used!
//void Clear_MainEntityList(void)
//{
//	MainEntityList.m_pNext = NULL;
//	extern bool bExternal_Entities_Init;
//	bExternal_Entities_Init = false;
//}

#ifdef WICKEDENGINE

std::vector< std::pair<std::string, time_t> > files_time_stamp;
std::vector<std::string> files_favorite;
std::vector<std::string> files_availableinfreetrial;
std::vector<std::string> files_pinned_categories;

std::string GetLineFromVectorFile(char *str, std::vector<std::string> & vecOfStrs, bool bIgnoreSpaces)
{
	for (int i = 0; i < vecOfStrs.size(); i++)
	{
		std::string entry = vecOfStrs[i];
		if (entry.length() > 0)
		{
			if (bIgnoreSpaces)
			{
				replaceAll(entry, " ", "");
				replaceAll(entry, "\t", "");
			}
			if (pestrcasestr(entry.c_str(), str))
			{
				return(vecOfStrs[i]);
			}
		}
	}
	return "";
}

std::string GetLineParameterFromVectorFile(char *str, std::vector<std::string> & vecOfStrs, bool bIgnoreSpaces)
{
	std::string Line = GetLineFromVectorFile(str, vecOfStrs, bIgnoreSpaces);
	if (Line.length() > 0)
	{
		//replaceAll(Line, " ", "");
		//replaceAll(Line, "\t", "");
		char *find = (char * ) pestrcasestr(Line.c_str(), "=");
		if (find)
		{
			find++;
			while (*find == ' ' || *find == '\t' && *find != 0)
				find++;
			int len = strlen(find);
			if (len > 1)
			{
				int i = len-1;
				while (find[i] == ' ' || find[i] == '\t' && find[i]!= 0)
				{
					find[i]= 0;
					i--;
				}
			}
			return(std::string(find));
		}
	}
	return "";
}

void RemoveStrStrFromVectorFile(char *str, std::vector<std::string> & vecOfStrs, bool bIgnoreSpaces)
{
	for (int i = 0; i < vecOfStrs.size(); i++)
	{
		std::string entry = vecOfStrs[i];
		if (entry.length() > 0)
		{
			if (bIgnoreSpaces)
			{
				replaceAll(entry, " ", "");
				replaceAll(entry, "\t", "");
			}
			if (pestrcasestr(entry.c_str(), str))
			{
				auto itr = std::find(vecOfStrs.begin(), vecOfStrs.end(), vecOfStrs[i]);
				if (itr != vecOfStrs.end())
				{
					vecOfStrs.erase(itr);
					RemoveStrStrFromVectorFile(str, vecOfStrs, bIgnoreSpaces);
					return;
				}
			}
		}
	}
}


bool getVectorFileContent(std::string fileName, std::vector<std::string> & vecOfStrs, bool bAllowEmpty)
{
	char szRealFilename[MAX_PATH];
	strcpy_s(szRealFilename, MAX_PATH, fileName.c_str());
	GG_GetRealPath(szRealFilename, 0);

	// Open the File
	std::ifstream in(szRealFilename);
	// Check if object is valid
	if (!in)
	{
		return false;
	}
	std::string str;
	while (std::getline(in, str))
	{
		if (str.size() > 0 || bAllowEmpty)
			vecOfStrs.push_back(str);
	}
	in.close();
	return true;
}

bool saveVectorFileContent(std::string fileName, std::vector<std::string> & vecOfStrs)
{
	char szRealFilename[MAX_PATH];
	strcpy_s(szRealFilename, MAX_PATH, fileName.c_str());
	GG_GetRealPath(szRealFilename, 1);

	std::ofstream output_file(szRealFilename);
	std::ostream_iterator<std::string> output_iterator(output_file, "\n");
	std::copy(vecOfStrs.begin(), vecOfStrs.end(), output_iterator);
	return true;
}


//Thread to collect fpe informations.
std::vector<cFolderItem::sFolderFiles *> g_ScanFpeFiles;
bool g_bFpeScanning = false;
int g_iScannedFiles = 0;
std::thread* g_pFPEScan = NULL;
void fpe_thread_function(void)
{
	g_bFpeScanning = true;

	for (int n = 0; n < g_ScanFpeFiles.size(); n++)
	{
		g_iScannedFiles++;
		cFolderItem::sFolderFiles * item = g_ScanFpeFiles[n];
		if (item)
		{
			char fpe_file[MAX_PATH];
			strcpy(fpe_file, item->m_sPath.Get());
			strcat(fpe_file, "\\");
			strcat(fpe_file, item->m_sName.Get());
			//HANDLE hfile = GG_CreateFile(fpe_file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			//We already have the full path so.
			HANDLE hfile = CreateFileA(fpe_file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hfile != INVALID_HANDLE_VALUE)
			{
				CloseHandle(hfile);

				std::vector<std::string> fpe_vector;
				if (getVectorFileContent(fpe_file, fpe_vector, true))
				{
					bool ischaracter = false;

					std::string s_isgroupobject = GetLineParameterFromVectorFile("isgroupobject=", fpe_vector, true);
					item->m_bIsGroupObject = 0;
					if (s_isgroupobject.length() > 0 && atoi(s_isgroupobject.c_str()) == 1)
						item->m_bIsGroupObject = 1;

					//Get info.
					std::string model = GetLineParameterFromVectorFile("model=", fpe_vector, true);
					if (model.length() > 0)
						item->m_sFPEModel = model.c_str();
					else
						item->m_sFPEModel = "";

					std::string textured = GetLineParameterFromVectorFile("textured=", fpe_vector, true); //Not used yet.
					if (textured.length() > 0)
						item->m_sFPETextured = textured.c_str();
					else
						item->m_sFPETextured = "";

					//Ignore there , and dont display thumbs, dont work anyway.
					std::string s_ischaractercreator = GetLineParameterFromVectorFile("charactercreator=", fpe_vector, true);
					item->m_bIsCharacterCreator = 0;
					if (s_ischaractercreator.length() > 0 && atoi(s_ischaractercreator.c_str()) == 1)
						item->m_bIsCharacterCreator = 1;

					#ifdef INCLUDEPOLYGONSORT
					std::string s_ischaracter = GetLineParameterFromVectorFile("ischaracter=", fpe_vector, true);
					ischaracter = false;
					if (s_ischaracter.length() > 0 && atoi(s_ischaracter.c_str()) == 1)
						ischaracter = true;

					bool isAnimated = false;
					std::string s_animmax = GetLineParameterFromVectorFile("animmax=", fpe_vector, true);
					if (s_animmax.length() > 0 && atoi(s_animmax.c_str()) > 0)
						isAnimated = true;
					#endif

					std::string keywords = GetLineParameterFromVectorFile("keywords=", fpe_vector, true); //Not used yet.
					if (keywords.length() > 0)
						item->m_sFPEKeywords = keywords.c_str();
					else
						item->m_sFPEKeywords = "";

					item->m_sFPEDBOFile = "";
					item->m_iFPEDBOFileSize = 0;

					#ifdef INCLUDEPOLYGONSORT
					//PE: This step is not needed when we dont have polygon sort.
					if (model.length() > 0)
					{
						//Get DBO info.
						char sEntityBank[MAX_PATH];
						strcpy(sEntityBank, "entitybank\\");

						char * find = (char *) pestrcasestr(fpe_file, sEntityBank);
						if (find)
						{
							find += strlen(sEntityBank);
							char * find2 = (char *)pestrcasestr(find, "ebebank\\");
							if (find2) strcpy(sEntityBank,"");
							
							char epath[MAX_PATH];
							strcpy(epath, find);
							bool bNotFound = true;
							for (int i = strlen(epath); i > 1; i--)
							{
								if (epath[i-1] == '\\' || epath[i-1] == '/')
								{
									bNotFound = false;
									epath[i] = 0;
									break;
								}
							}
							if(bNotFound) strcpy(epath, "");

							char sFile[MAX_PATH];
							strcpy(sFile, "");
							if (item->m_bIsCharacterCreator == 0)
							{
								strcpy(sFile, sEntityBank);
								strcat(sFile, epath);
								strcat(sFile, item->m_sFPEModel.Get());
							}
							else
							{
								strcpy(sFile, item->m_sFPEModel.Get());
							}

							//PE: Todo remove spaces at end sFile="entitybank\Purchased\Oldpman\ARTIST PACK\Scenery\block66.x                         "
							//Make sure we use full path to files.

							int iSrcFormat = 0;
							if (pestrcasestr(sFile,".x")) iSrcFormat = 1;
							if (pestrcasestr(sFile, ".fbx")) iSrcFormat = 2;
							if (iSrcFormat > 0)
							{
								char sDboFile[MAX_PATH];
								if (iSrcFormat == 1)
								{
									// X File Format
									strcpy(sDboFile, sFile);
									sDboFile[strlen(sDboFile) - 2] = 0;
									strcat(sDboFile, ".dbo");
									GG_GetRealPath(sDboFile, false);
									if (FileExist(sDboFile) == 1) strcpy(sFile,sDboFile);
								}
								if (iSrcFormat == 2)
								{
									// FBX File Format
									strcpy(sDboFile, sFile);
									sDboFile[strlen(sDboFile) - 4] = 0;
									strcat(sDboFile, ".dbo");
									GG_GetRealPath(sDboFile, false);
									if (FileExist(sDboFile) == 1) strcpy(sFile,sDboFile);
								}
							}

							GG_GetRealPath(sFile, false);
							HANDLE hfile = GG_CreateFile(sFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
							if (hfile != INVALID_HANDLE_VALUE)
							{
								item->m_sFPEDBOFile = sFile;
								item->m_iFPEDBOFileSize = GetFileSize(hfile, NULL);
								#ifdef INCLUDEPOLYGONSORT
								if (ischaracter)
								{
									if (item->m_iFPEDBOFileSize > 14000000) // zoombies are really large.
										item->m_iFPEDBOFileSize = 4500*9+ g_iScannedFiles;
									else if (item->m_iFPEDBOFileSize > 7000000) // uber soldier is large.
										item->m_iFPEDBOFileSize = 17400 * 9 + g_iScannedFiles;
									else if (item->m_iFPEDBOFileSize > 4000000) // Shotgun soldier.dbo
										item->m_iFPEDBOFileSize = 14500 * 9 + g_iScannedFiles;
									else if (item->m_iFPEDBOFileSize > 3000000) // masked soldier.dbo
										item->m_iFPEDBOFileSize = 17400 * 9 + g_iScannedFiles;
									else if (item->m_iFPEDBOFileSize > 1800000) // charactercreatorplus
										item->m_iFPEDBOFileSize = 20000 * 9 + g_iScannedFiles;
									else if (item->m_iFPEDBOFileSize > 900000) // wizard
										item->m_iFPEDBOFileSize = 10000 * 9 + g_iScannedFiles;
									else //Chars have a large amount of animations, so... This is REALLY not precise.
										item->m_iFPEDBOFileSize = item->m_iFPEDBOFileSize & 0x1ffff;

								}
								else
								{
									if (isAnimated)
									{
										//Cant trust file when animated. This is REALLY not precise.
										item->m_iFPEDBOFileSize = item->m_iFPEDBOFileSize & 0x1ffff;
									}
								}
								#endif

								CloseHandle(hfile);
							}
						}
					}
					#endif

					item->m_bFPELoaded = true; //Mark that its now safe to use all FPE settings from other threads.
				}

				fpe_vector.clear();
			}
		}
	}
	g_ScanFpeFiles.clear();
	g_bFpeScanning = false;
}

void fpe_thread_start(void)
{
//	g_pFPEScan = new std::thread(fpe_thread_function, std::ref(g_object_list));
	g_pFPEScan = new std::thread(fpe_thread_function);
}
bool fpe_thread_in_progress(void)
{
	return g_bFpeScanning;
}

#endif

std::vector<std::string> duplicate_files_check;


void CustomSortFiles(int iSortBy, cFolderItem::sFolderFiles * m_pFileSortStart)
{
	if (!m_pFileSortStart) return;
	std::vector<std::string> sorted_files;

	cFolderItem::sFolderFiles * m_pFiles = m_pFileSortStart;
	//PE: Make a list here thats sorted by file time.
	m_pFiles->bSorted = false;
	if (m_pFiles) m_pFiles = m_pFiles->m_pNext; //PE: Skip "..." entry.

	cFolderItem::sFolderFiles * m_pFileSort = m_pFiles;
	cFolderItem::sFolderFiles * m_pScanSort = m_pFiles;
	while (m_pFileSort)
	{
		m_pFileSort->bSorted = false;
		if (iSortBy == 1)
		{
			cstr sort_s = m_pFileSort->m_sNameFinal;
			sorted_files.push_back(sort_s.Get());
		}
		m_pFileSort = m_pFileSort->m_pNext;
	}

	m_pFileSort = m_pFileSortStart;
	cFolderItem::sFolderFiles * m_pSortBest = NULL;
	cFolderItem::sFolderFiles * m_pPrevBest = NULL;

	while (m_pFileSort)
	{
		bool bFoundBest = false;
		time_t m_tFileModifySort = 0;
		m_pScanSort = m_pFiles;
		while (m_pScanSort)
		{
			if (!m_pScanSort->bSorted)
			{
				if (iSortBy == 0)
				{
					if (m_pScanSort->m_tFileModify >= m_tFileModifySort)
					{
						m_tFileModifySort = m_pScanSort->m_tFileModify;
						m_pSortBest = m_pScanSort;
						bFoundBest = true;
					}
				}
				else
				{
					//Default just accept all.
					m_pSortBest = m_pScanSort;
					bFoundBest = true;
				}
			}
			m_pScanSort = m_pScanSort->m_pNext;
		}

		if (bFoundBest) {
			if (iSortBy == 0)
			{
				if (m_pPrevBest)
				{
					m_pPrevBest->m_pNextTime = m_pSortBest;
				}
				else
				{
					m_pFileSort->m_pNextTime = m_pSortBest;
				}
				m_pPrevBest = m_pSortBest;
				m_pSortBest->m_pNextTime = NULL;
				//printf("%s", m_pSortBest->m_sName);
				m_pSortBest->bSorted = true;
			}
			else
			{
				if (m_pPrevBest)
				{
					m_pPrevBest->m_pCustomSort = m_pSortBest;
				}
				else
				{
					m_pFileSort->m_pCustomSort = m_pSortBest;
				}
				m_pPrevBest = m_pSortBest;
				m_pSortBest->m_pCustomSort = NULL;
				//printf("%s", m_pSortBest->m_sName);
				m_pSortBest->bSorted = true;

				//Default just add.
			}
		}
		else {
			if (iSortBy == 0)
			{
				if (m_pPrevBest)
					m_pPrevBest->m_pNextTime = NULL;
			}
			else
			{
				if (m_pPrevBest)
					m_pPrevBest->m_pCustomSort = NULL;
			}
		}

		m_pFileSort = m_pFileSort->m_pNext;
	}

	sorted_files.clear();
	//PE: DEBUG ONLY
	/*
	if (m_pSortBest->m_pNextTime != NULL)
	{
		printf("err:");
	}
	m_pFileSort = pNewFolder->m_pNext->m_pFirstFile;; //TEST
	while (m_pFileSort)
	{
		printf("%s", m_pFileSort->m_sName);
		m_pFileSort = m_pFileSort->m_pNextTime;
	}
	*/

}

void SetAvailableInFreeTrial(int foldertype, cFolderItem::sFolderFiles* pNewItem, cstr file)
{
	if (foldertype == 0)
	{
		// exclude all objects except those in files_availableinfreetrial list
		std::vector<std::string>::iterator itf = files_availableinfreetrial.begin();
		for (; itf != files_availableinfreetrial.end(); ++itf)
		{
			if (itf->size() > 0)
			{
				if (strnicmp(itf->c_str(), file.Get(), file.Len()) == 0)
				{
					pNewItem->bAvailableInFreeTrial = true;
					break;
				}
			}
		}
	}
	else
	{
		if (foldertype == 4)
		{
			// behavior exclusions
			LPSTR pExclude = "";
			bool bBlockThisOne = false;
			LPSTR pThisFile = file.Get();
			pExclude = "jumpscare.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "bounce.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "freezeplayer.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "heal.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "hurt.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "slip.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "teleport.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "collection count.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "dynamite.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "seecam.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "spiketrap.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			pExclude = "trapdoor.lua"; if (strnicmp(pThisFile + strlen(pThisFile) - strlen(pExclude), pExclude, strlen(pExclude)) == NULL) bBlockThisOne = true;
			if (bBlockThisOne == true)
			{
				pNewItem->bAvailableInFreeTrial = false;
			}
			else
			{
				pNewItem->bAvailableInFreeTrial = true;
			}
		}
		else
		{
			// allow all
			pNewItem->bAvailableInFreeTrial = true;
		}
	}
}

void GetMainEntityList(char* folder_s, char* rel_s, void *pFolder, char* folder_name_start, bool bForceToTop, int foldertype)
{
	//foldertype: 0 fpe , 1 waw mp3 ogg. , 2 BMP, JPG, PNG, TIF, and GIF
	// get legacy white list once
	if (g_bCreateLegacyWhiteList == true)
	{
		#ifdef WICKEDENGINE
		getVectorFileContent("favoritelist.ini", files_favorite);
		extern bool g_bFreeTrialVersion;
		if (g_bFreeTrialVersion == true)
		{
			getVectorFileContent("freetriallist.ini", files_availableinfreetrial);
		}
		#endif

		g_pLegacyWhiteList.clear();
		FILE* fp = GG_fopen(".\\..\\legacyblacklist.ini", "rt");
		if (fp)
		{
			char c;
			fread(&c, sizeof(char), 1, fp);
			while (!feof(fp))
			{
				// get string from file
				char szEntNameFromFile[MAX_PATH] = "";
				int iOffset = 0;
				while (!feof(fp) && c != 13 && c != 10)
				{
					szEntNameFromFile[iOffset++] = c;
					fread(&c, sizeof(char), 1, fp);
				}
				szEntNameFromFile[iOffset] = 0;

				// skip beyond CR
				while (!feof(fp) && (c == 13 || c == 10))
					fread(&c, sizeof(char), 1, fp);

				// write into array
				LPSTR pWhiteListItem = new char[512];
				strlwr(szEntNameFromFile);
				strcpy(pWhiteListItem, szEntNameFromFile);
				g_pLegacyWhiteList.push_back(pWhiteListItem);
			}
			fclose(fp);
		}
		g_bCreateLegacyWhiteList = false;
	}

	// get full entity list
	int tt = 0;
	cstr file_s = "";
	int fin = 0;
	cstr tempcstr;
	cFolderItem *pNewFolder = (cFolderItem *) pFolder;
	if (pNewFolder == NULL)
	{
		pNewFolder = &MainEntityList;
		iTotalFolders = 0;
		iTotalFiles = 0;
	}
	while (pNewFolder->m_pNext) 
	{
		pNewFolder = pNewFolder->m_pNext;
	}

	if (PathExist(folder_s) == 1)
	{
		SetDir(folder_s);

		//Create FolderItem.
		cFolderItem *pNewItem;
		pNewItem = new cFolderItem();
		pNewItem->m_sFolder = folder_s;

		LPSTR pOld = GetDir();
		pNewItem->m_sFolderFullPath = pOld;
		pNewItem->iType = foldertype;

		//PE: Added here for speed.
		pNewItem->m_iEntityOffset = pNewItem->m_sFolderFullPath.Len();
		cStr path = pNewItem->m_sFolderFullPath.Get();
		LPSTR pPathSearch = path.Get();
		LPSTR pFind = "\\entitybank";
		if(foldertype == 1)
			pFind = "\\audiobank";
		if (foldertype == 2)
			pFind = "\\imagebank";
		if (foldertype == 3)
			pFind = "\\videobank";
		if (foldertype == 4)
			pFind = "\\scriptbank";
		if (foldertype == 5)
			pFind = "\\particlesbank";
		if (foldertype == 6)
			pFind = "\\charactercreatorplus\\animations";
		
		for (int n = 0; n < strlen(pPathSearch); n++)
		{
			if (strnicmp(pPathSearch + n, pFind, strlen(pFind)) == NULL)
			{
				pNewItem->m_iEntityOffset = n + strlen(pFind);
				break;
			}
		}


		strcpy(pNewItem->cfolder, pNewItem->m_sFolderFullPath.Get() );

		#ifdef WICKEDENGINE
		// ensures writable folder sorts all its folders to top
		if ( bForceToTop == true )
		{
			// ensure when sorted, these folders stay at the very top for easy discovery
			pNewItem->cfolder[0] = 'A';
		}
		#endif

		pNewItem->m_pSubFolder = NULL;
		pNewItem->visible = true;
		pNewItem->deletethisentry = false;
		pNewItem->count = ++iTotalFolders;
		pNewItem->m_pFirstFile = NULL;
		pNewItem->bIsCustomFolder = false;
		pNewItem->m_pNext = NULL;

		//Update last folder modify date time.
		struct stat sb;
		if (stat(pNewItem->m_sFolderFullPath.Get(), &sb) == 0) 
		{
			if (sb.st_mtime != pNewItem->m_tFolderModify) 
			{
				pNewItem->m_tFolderModify = sb.st_mtime;
			}
		}	
		#ifdef WICKEDENGINE
		// taken care of above with bForceToTop
		#else
		if (pestrcasestr(pNewItem->m_sFolderFullPath.Get(), "entitybank\\user")) 
		{
			//Make sure they come ontop in list.
			strcpy(pNewItem->cfolder, pNewItem->m_sFolderFullPath.Get());
			char *find = (char *) pestrcasestr(pNewItem->cfolder, "entitybank\\user");
			if (find)
				find[11] = '1';
		}
		#endif
		pNewFolder->m_pNext = pNewItem;

		FindFirst(); fin = 0;

		std::vector<std::string> sorted_files;
		while (GetFileType()>-1)
		{
			file_s = GetFileName();
			if (file_s == "." || file_s == "..")
			{
				//  ignore . and ..
			}
			else
			{
				if (GetFileType() == 1)
				{
					//  folder
					GetMainEntityList(file_s.Get(), cstr(cstr(rel_s) + file_s + "\\").Get(), (void *)pNewFolder->m_pNext, "", bForceToTop, foldertype);
					FindFirst();
					if (fin > 0)
					{
						for (tt = 1; tt <= fin; tt++)
						{
							if (GetFileType()>-1)
							{
								FindNext();
							}
						}
					}
				}
				else
				{
					bool bValid = false;
					if (foldertype == 0 && pestrcasestr(file_s.Get(), ".fpe"))
						bValid = true;
					#ifdef WICKEDENGINE
					if (foldertype == 1 && pestrcasestr(file_s.Get(), ".wav"))
						bValid = true;
					if (foldertype == 1 && pestrcasestr(file_s.Get(), ".mp3"))
						bValid = true;
					if (foldertype == 1 && pestrcasestr(file_s.Get(), ".ogg"))
						bValid = true;

					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".png"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".dds"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".bmp"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".tif"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".jpg"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".gif"))
						bValid = true;

					if (foldertype == 3 && pestrcasestr(file_s.Get(), ".wmv"))
						bValid = true;
					//if (foldertype == 3 && pestrcasestr(file_s.Get(), ".ogv"))
					//	bValid = true;
					if (foldertype == 3 && pestrcasestr(file_s.Get(), ".mp4"))
						bValid = true;

					if (foldertype == 4 && pestrcasestr(file_s.Get(), ".lua"))
						bValid = true;

					if (foldertype == 5 && pestrcasestr(file_s.Get(), ".arx"))
						bValid = true;

					if (foldertype == 6 && pestrcasestr(file_s.Get(), ".fpe"))
						bValid = true;

					#endif
					// file
					if(bValid)
					{
						// compare entity with legacy whitelist, and skip if on list (slowly hiding older models)
						cstr pComboRelandFile = cstr(rel_s) + file_s;
						LPSTR pFilePtr = pComboRelandFile.Get();
						int iLegacyWhiteListCount = g_pLegacyWhiteList.size();
						int n = 0;
						for (; n < iLegacyWhiteListCount; n++)
						{
							if (stricmp(pFilePtr, g_pLegacyWhiteList[n]) == NULL)
								break;
						}
						if (n == iLegacyWhiteListCount)
						{
							// not found in legacy whitelist, allow
							sorted_files.push_back(file_s.Get());
							
							#ifdef WICKEDENGINE
							//SAVE: time_create
							time_t ts = GetFileDateLong();
							cstr file = pNewFolder->m_pNext->m_sFolderFullPath;
							file = file + "\\" + file_s;
							files_time_stamp.push_back( std::make_pair(file.Get(), ts ) );
							#endif
						}
					}
				}
			}
			FindNext();
			fin = fin + 1;
		}

		//sorted_files
		cFolderItem::sFolderFiles * m_pFiles = NULL;
		if (!sorted_files.empty()) 
		{
			std::sort(sorted_files.begin(), sorted_files.end(), NoCaseLess);
			std::vector<std::string>::iterator it = sorted_files.begin();
			if (it->size() > 0) 
			{
				cFolderItem::sFolderFiles *pNewItem = new cFolderItem::sFolderFiles;
				pNewItem->m_sName = "...";
				pNewItem->m_sNameFinal = pNewItem->m_sName;
				pNewItem->m_tFileModify = 0;
				pNewItem->m_Backdrop = "";
				pNewItem->bFavorite = false;
				pNewItem->bAvailableInFreeTrial = false;
				pNewItem->m_sPath = "";
				pNewItem->m_sFolder = "[na]";
				pNewItem->iFlags = 0;
				pNewItem->iPreview = 0;
				pNewItem->iBigPreview = 0;
				pNewItem->id = iTotalFiles++;
				pNewItem->bPreviewProcessed = false;
				pNewItem->m_pNext = NULL;
				pNewItem->m_pNextTime = NULL;
				pNewItem->m_pCustomSort = NULL;

				#ifdef WICKEDENGINE
				pNewItem->m_bFPELoaded = false;
				pNewItem->m_sFPEModel="";
				pNewItem->m_sFPETextured = "";
				pNewItem->m_sFPEKeywords = "";
				pNewItem->m_sFPEDBOFile = "";
				pNewItem->m_sDLuaDescription = "##na##";
				pNewItem->m_fDLuaHeight = 0.0f;
				pNewItem->m_iFPEDBOFileSize = 0;
				pNewItem->m_bIsCharacterCreator = 0;
				pNewItem->iType = foldertype;
				#endif

				pNewFolder->m_pNext->m_pFirstFile = pNewItem;
				m_pFiles = pNewItem;
			}
			for (; it != sorted_files.end(); ++it) 
			{
				bool bAddToList = true;
				#ifdef WICKEDENGINE
				//PE: Eliminate any duplicates here, DocWrite and Normal Path, Prefer DocWrite.
				if (it->size() > 0) {
					cStr sName = (char *) it->c_str();
					cStr m_sPath = pNewFolder->m_pNext->m_sFolderFullPath;
					cStr sCheck = m_sPath + cstr("\\") + sName;
					char *find = NULL;
					if(foldertype == 0)
						find = (char *) pestrcasestr(sCheck.Get(),"entitybank\\");
					if (foldertype == 1)
						find = (char *)pestrcasestr(sCheck.Get(), "audiobank\\");
					if (foldertype == 2)
						find = (char *)pestrcasestr(sCheck.Get(), "imagebank\\");
					if (foldertype == 3)
						find = (char *)pestrcasestr(sCheck.Get(), "videobank\\");
					if (foldertype == 4)
						find = (char *)pestrcasestr(sCheck.Get(), "scriptbank\\");
					if (foldertype == 5)
						find = (char *)pestrcasestr(sCheck.Get(), "particlesbank\\");
					if (foldertype == 6)
						find = (char *)pestrcasestr(sCheck.Get(), "charactercreatorplus\\animations\\");
					if (find)
					{
						// ZJ: Got heap corruption error here.
						//sCheck = find;
						cStr sFind(find);
						strcpy(sCheck.Get(), sFind.Get());
						//sCheck = sCheck + cstr("\\") + sName;
					}
					sCheck = sCheck.Lower();
					auto itr = std::find(duplicate_files_check.begin(), duplicate_files_check.end(), sCheck.Get());
					if (itr != duplicate_files_check.end() && duplicate_files_check.size() > 0 )
						bAddToList = false;
					else
						duplicate_files_check.push_back(sCheck.Get());
				}
				#endif
				if (it->size() > 0 && bAddToList) 
				{
					cFolderItem::sFolderFiles *pNewItem;
					pNewItem = new cFolderItem::sFolderFiles;

					pNewItem->m_sName = it->c_str();
					if ((foldertype == 0 || foldertype == 6) && pNewItem->m_sName.Len() > 4)
						pNewItem->m_sNameFinal = pNewItem->m_sName.Left(pNewItem->m_sName.Len() - 5);
					else
						pNewItem->m_sNameFinal = pNewItem->m_sName;

					//Generate a better search string. include category at end.
					#ifdef WICKEDENGINE
					std::string sBetterSearch = pNewItem->m_sNameFinal.Get();
					sBetterSearch = sBetterSearch + " ( " + pNewFolder->m_pNext->m_sFolder.Get() + " )";
					//Remove main folder for better search.
					if (foldertype == 1)
						replaceAll(sBetterSearch, "audiobank", "");
					if (foldertype == 2)
						replaceAll(sBetterSearch, "imagebank", "");
					if (foldertype == 3)
						replaceAll(sBetterSearch, "videobank", "");
					if (foldertype == 4)
						replaceAll(sBetterSearch, "scriptbank", "");
					if (foldertype == 5)
						replaceAll(sBetterSearch, "particlesbank", "");
					if (foldertype == 6)
						replaceAll(sBetterSearch, "charactercreatorplus\\animations", "");
					replaceAll(sBetterSearch, "_", " ");
					replaceAll(sBetterSearch, "-", " ");
					pNewItem->m_sBetterSearch = sBetterSearch.c_str();
					std::string phonetic_search = soundexall(sBetterSearch);
					pNewItem->m_sPhoneticSearch = phonetic_search.c_str();
					#endif

					pNewItem->m_tFileModify = 0; //PE: Need timestamp here.
					pNewItem->bFavorite = false;
					pNewItem->bAvailableInFreeTrial = false;

					#ifdef WICKEDENGINE
					//PE: This was to slow.
					struct stat sb;
					cstr file = pNewFolder->m_pNext->m_sFolderFullPath;
					file = file + "\\" + pNewItem->m_sName;
					std::vector< std::pair<std::string, time_t> >::iterator its = files_time_stamp.begin();
					for (; its != files_time_stamp.end(); ++its)
					{
						if (its->first.size() > 0)
						{
							if( strcmp(its->first.c_str(),file.Get()) == 0 )
							{
								pNewItem->m_tFileModify = its->second;
								break;
							}
						}
					}
					std::vector<std::string>::iterator itf = files_favorite.begin();
					for (; itf != files_favorite.end(); ++itf)
					{
						if (itf->size() > 0)
						{
							if (strnicmp(itf->c_str(), file.Get(), file.Len() ) == 0)
							{
								pNewItem->bFavorite = true;
								break;
							}
						}
					}
					extern bool g_bFreeTrialVersion;
					if (g_bFreeTrialVersion == true)
					{
						SetAvailableInFreeTrial(foldertype, pNewItem, file);
					}
					#endif

					pNewItem->m_sPath = pNewFolder->m_pNext->m_sFolderFullPath;
					pNewItem->m_sFolder = pNewFolder->m_pNext->m_sFolder;
					pNewItem->m_Backdrop = "";
					pNewItem->iFlags = 0;
					pNewItem->iPreview = 0;
					pNewItem->iBigPreview = 0;
					pNewItem->id = iTotalFiles++;
					pNewItem->bPreviewProcessed = false;
					pNewItem->m_pNext = NULL;
					pNewItem->m_pNextTime = NULL;
					pNewItem->m_pCustomSort = NULL;

					#ifdef WICKEDENGINE
					pNewItem->m_bFPELoaded = false;
					pNewItem->m_sFPEModel = "";
					pNewItem->m_sFPETextured = "";
					pNewItem->m_sFPEKeywords = "";
					pNewItem->m_sFPEDBOFile = "";
					pNewItem->m_sDLuaDescription = "##na##";
					pNewItem->m_fDLuaHeight = 0.0f;
					pNewItem->m_iFPEDBOFileSize = 0;
					pNewItem->m_bIsCharacterCreator = 0;
					pNewItem->iType = foldertype;
					#endif

					m_pFiles->m_pNext = pNewItem;
					m_pFiles->m_pNextTime = pNewItem;
					m_pFiles->m_pCustomSort = pNewItem;
					m_pFiles = pNewItem;
				}
			}
			sorted_files.clear();

			#ifdef WICKEDENGINE
			CustomSortFiles(0, pNewFolder->m_pNext->m_pFirstFile);
			#endif
		}
		SetDir("..");
	}
}

void RefreshEntityFolder(char* folder_s, void *pFolder)
{
	int tt = 0;
	cstr file_s = "";
	int fin = 0;
	cstr tempcstr;

	//pNewFolder->m_pFirstFile
	cFolderItem *pNewFolder = (cFolderItem *)pFolder;

	cstr OldDir = GetDir();
	if(pNewFolder && PathExist(folder_s) == 1)
	{
		int foldertype = pNewFolder->iType;

		//Delete everything inside pNewFolder->m_pFirstFile
		cFolderItem::sFolderFiles * m_pnextFiles, *m_pFiles = pNewFolder->m_pFirstFile;

		while (m_pFiles) 
		{
			m_pnextFiles = m_pFiles->m_pNext;
			if (m_pFiles->iPreview > 0) 
			{
				//Delete any old thumb images.
				//PE: Bug - tool icons can be used , so never delete m_pFiles->iPreview < UIV3IMAGES
				if ( m_pFiles->iPreview >= 4000 && GetImageExistEx(m_pFiles->iPreview) == 1 && m_pFiles->iPreview < UIV3IMAGES )
				{
					DeleteImage(m_pFiles->iPreview);
					m_pFiles->iPreview = 0;
				}
			}
			delete m_pFiles;
			m_pFiles = m_pnextFiles;
		}

		SetDir(folder_s);

		FindFirst(); fin = 0;

		std::vector<std::string> sorted_files;
		while (GetFileType() > -1)
		{
			file_s = GetFileName();
			if (file_s == "." || file_s == "..")
			{
				//  ignore . and ..
			}
			else
			{
				if (GetFileType() == 1)
				{
					//We only update the folder requested.
				}
				else
				{
					bool bValid = false;
					if (foldertype == 0 && pestrcasestr(file_s.Get(), ".fpe"))
						bValid = true;
					#ifdef WICKEDENGINE
					if (foldertype == 1 && pestrcasestr(file_s.Get(), ".wav"))
						bValid = true;
					if (foldertype == 1 && pestrcasestr(file_s.Get(), ".mp3"))
						bValid = true;
					if (foldertype == 1 && pestrcasestr(file_s.Get(), ".ogg"))
						bValid = true;

					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".png"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".dds"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".bmp"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".tif"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".jpg"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".gif"))
						bValid = true;

					if (foldertype == 3 && pestrcasestr(file_s.Get(), ".wmv"))
						bValid = true;
					//if (foldertype == 3 && pestrcasestr(file_s.Get(), ".ogv"))
					//	bValid = true;
					if (foldertype == 3 && pestrcasestr(file_s.Get(), ".mp4"))
						bValid = true;

					if (foldertype == 4 && pestrcasestr(file_s.Get(), ".lua"))
						bValid = true;

					if (foldertype == 5 && pestrcasestr(file_s.Get(), ".arx"))
						bValid = true;

					#endif

					if (bValid) {
						sorted_files.push_back(file_s.Get());

						#ifdef WICKEDENGINE
						//SAVE: time_create
						time_t ts = GetFileDateLong();
						cstr file = pNewFolder->m_sFolderFullPath;
						file = file + "\\" + file_s;

						std::vector< std::pair<std::string, time_t> >::iterator its = files_time_stamp.begin();
						for (; its != files_time_stamp.end(); ++its)
						{
							if (its->first.size() > 0) {
								if (strcmp(its->first.c_str(), file.Get()) == 0)
								{
									files_time_stamp.erase(its);
									break;
								}
							}
						}

						files_time_stamp.push_back(std::make_pair(file.Get(), ts));
						#endif

					}
				}
			}
			FindNext();
			fin = fin + 1;
		}

		//sorted_files
		if (!sorted_files.empty()) 
		{
			std::sort(sorted_files.begin(), sorted_files.end(), NoCaseLess);

			std::vector<std::string>::iterator it = sorted_files.begin();

			if (it->size() > 0) 
			{
				cFolderItem::sFolderFiles *pNewItem = new cFolderItem::sFolderFiles;
				pNewItem->m_sName = "...";
				pNewItem->m_sNameFinal = pNewItem->m_sName;
				pNewItem->m_tFileModify = 0;
				pNewItem->m_Backdrop = "";
				pNewItem->bFavorite = false;
				pNewItem->bAvailableInFreeTrial = false;
				pNewItem->m_sPath = "";
				pNewItem->m_sFolder = "[na]";
				pNewItem->iFlags = 0;
				pNewItem->iPreview = 0;
				pNewItem->iBigPreview = 0;
				pNewItem->id = iTotalFiles++;
				pNewItem->bPreviewProcessed = false;
				pNewItem->m_pNext = NULL;
				pNewItem->m_pNextTime = NULL;
				pNewItem->m_pCustomSort = NULL;

				#ifdef WICKEDENGINE
				pNewItem->m_bFPELoaded = false;
				pNewItem->m_sFPEModel = "";
				pNewItem->m_sFPETextured = "";
				pNewItem->m_sFPEKeywords = "";
				pNewItem->m_sFPEDBOFile = "";
				pNewItem->m_sDLuaDescription = "##na##";
				pNewItem->m_fDLuaHeight = 0.0f;
				pNewItem->m_iFPEDBOFileSize = 0;
				pNewItem->m_bIsCharacterCreator = 0;
				pNewItem->iType = foldertype;
				#endif

				pNewFolder->m_pFirstFile = pNewItem;
				m_pFiles = pNewItem;
			}

			for (; it != sorted_files.end(); ++it) 
			{
				#ifdef WICKEDENGINE
				//PE: Here we update , so we only have new files. but need to update duplicate_files_check
				if (it->size() > 0) {
					cStr sName = (char *)it->c_str();
					cStr m_sPath = folder_s;
					cStr sCheck = m_sPath + cstr("\\") + sName;
					char *find = NULL;

					if(foldertype == 0)
						find = (char *)pestrcasestr(sCheck.Get(), "entitybank\\");
					if (foldertype == 1)
						find = (char *)pestrcasestr(sCheck.Get(), "audiobank\\");
					if (foldertype == 2)
						find = (char *)pestrcasestr(sCheck.Get(), "imagebank\\");
					if (foldertype == 3)
						find = (char *)pestrcasestr(sCheck.Get(), "videobank\\");
					if (foldertype == 4)
						find = (char *)pestrcasestr(sCheck.Get(), "scriptbank\\");
					if (foldertype == 5)
						find = (char *)pestrcasestr(sCheck.Get(), "particlesbank\\");
					if (foldertype == 6)
						find = (char *)pestrcasestr(sCheck.Get(), "charactercreatorplus\\animations\\");

					if (find)
					{
						// ZJ: Got a heap corruption error here.
						//sCheck = find;
						cStr sFind(find);
						strcpy(sCheck.Get(), sFind.Get());
						//sCheck = sCheck + cstr("\\") + sName;
					}
					sCheck = sCheck.Lower();
					auto itr = std::find(duplicate_files_check.begin(), duplicate_files_check.end(), sCheck.Get());
					if(!(itr != duplicate_files_check.end() && duplicate_files_check.size() > 0))
						duplicate_files_check.push_back(sCheck.Get());
				}
				#endif
				if (it->size() > 0)
				{
					cFolderItem::sFolderFiles *pNewItem = new cFolderItem::sFolderFiles;
					pNewItem->m_sName = it->c_str();
					if ((foldertype == 0 || foldertype == 6) && pNewItem->m_sName.Len() > 4)
						pNewItem->m_sNameFinal = pNewItem->m_sName.Left(pNewItem->m_sName.Len() - 5);
					else
						pNewItem->m_sNameFinal = pNewItem->m_sName;
					
					//Generate a better search string. include category at end.#
					#ifdef WICKEDENGINE
					std::string sBetterSearch = pNewItem->m_sNameFinal.Get();
					sBetterSearch = sBetterSearch + " ( " + pNewFolder->m_sFolder.Get() + " )";
					//Remove main folder for better search.
					if (foldertype == 1)
						replaceAll(sBetterSearch, "audiobank", "");
					if (foldertype == 2)
						replaceAll(sBetterSearch, "imagebank", "");
					if (foldertype == 3)
						replaceAll(sBetterSearch, "videobank", "");
					if (foldertype == 4)
						replaceAll(sBetterSearch, "scriptbank", "");
					if (foldertype == 5)
						replaceAll(sBetterSearch, "particlesbank", "");
					if (foldertype == 6)
						replaceAll(sBetterSearch, "charactercreatorplus\\animations", "");

					replaceAll(sBetterSearch, "_", " ");
					replaceAll(sBetterSearch, "-", " ");
					pNewItem->m_sBetterSearch = sBetterSearch.c_str();
					std::string phonetic_search = soundexall(sBetterSearch);
					pNewItem->m_sPhoneticSearch = phonetic_search.c_str();
					#endif

					pNewItem->m_tFileModify = 0; //PE: Need timestamp here.
					pNewItem->bFavorite = false;
					pNewItem->bAvailableInFreeTrial = false;

					#ifdef WICKEDENGINE
					//PE: This was to slow.
					struct stat sb;
					cstr file = pNewFolder->m_sFolderFullPath;
					file = file + "\\" + pNewItem->m_sName;
					std::vector< std::pair<std::string, time_t> >::iterator its = files_time_stamp.begin();
					for (; its != files_time_stamp.end(); ++its)
					{
						if (its->first.size() > 0) 
						{
							if (strcmp(its->first.c_str(), file.Get()) == 0)
							{
								pNewItem->m_tFileModify = its->second;
								break;
							}
						}
					}
					std::vector<std::string>::iterator itf = files_favorite.begin();
					for (; itf != files_favorite.end(); ++itf)
					{
						if (itf->size() > 0) 
						{
							if (strnicmp(itf->c_str(), file.Get(), file.Len()) == 0)
							{
								pNewItem->bFavorite = true;
								break;
							}
						}
					}
					extern bool g_bFreeTrialVersion;
					if (g_bFreeTrialVersion == true)
					{
						SetAvailableInFreeTrial(foldertype, pNewItem, file);
						/*
						std::vector<std::string>::iterator itf = files_availableinfreetrial.begin();
						for (; itf != files_availableinfreetrial.end(); ++itf)
						{
							if (itf->size() > 0)
							{
								if (strnicmp(itf->c_str(), file.Get(), file.Len()) == 0)
								{
									pNewItem->bAvailableInFreeTrial = true;
									break;
								}
							}
						}
						*/
					}
					#endif

					pNewItem->m_sPath = folder_s;
					pNewItem->m_sFolder = folder_s;
					pNewItem->m_Backdrop = "";
					pNewItem->iFlags = 0;
					pNewItem->iPreview = 0;
					pNewItem->iBigPreview = 0;
					pNewItem->id = iTotalFiles++;
					pNewItem->bPreviewProcessed = false;
					pNewItem->m_pNext = NULL;
					pNewItem->m_pNextTime = NULL;
					pNewItem->m_pCustomSort = NULL;

					#ifdef WICKEDENGINE
					pNewItem->m_bFPELoaded = false;
					pNewItem->m_sFPEModel = "";
					pNewItem->m_sFPETextured = "";
					pNewItem->m_sFPEKeywords = "";
					pNewItem->m_sFPEDBOFile = "";
					pNewItem->m_sDLuaDescription = "##na##";
					pNewItem->m_fDLuaHeight = 0.0f;
					pNewItem->m_iFPEDBOFileSize = 0;
					pNewItem->m_bIsCharacterCreator = 0;
					pNewItem->iType = foldertype;
					#endif
					m_pFiles->m_pNext = pNewItem;
					m_pFiles->m_pNextTime = pNewItem;
					m_pFiles->m_pCustomSort = pNewItem;
					m_pFiles = pNewItem;
				}
			}
			sorted_files.clear();

			#ifdef WICKEDENGINE
			CustomSortFiles(0, pNewFolder->m_pFirstFile);
			#endif

		}

		//Update last folder modify date time.
		struct stat sb;
		if (stat(pNewFolder->m_sFolderFullPath.Get(), &sb) == 0) 
		{
			if (sb.st_mtime != pNewFolder->m_tFolderModify) 
			{
				pNewFolder->m_tFolderModify = sb.st_mtime;
			}
		}
	}

	SetDir(OldDir.Get());
}

void RefreshPurchasedFolder ( void )
{
	// First update for any folders
	// for this we need to delete the curretn one
	bool bCannotDeletFirstItIsGlobal = false;
	cFolderItem* pDeleteFolder = &MainEntityList;
	while (pDeleteFolder->m_pNext)
	{
		cFolderItem* pNextOneToDelete = pDeleteFolder->m_pNext;
		pDeleteFolder->m_pNext = NULL;
		if (bCannotDeletFirstItIsGlobal == false)
		{
			// just clear the first instance as it is global
			memset(&pDeleteFolder, 0, sizeof(pDeleteFolder));
			bCannotDeletFirstItIsGlobal = true;
	}
		else
		{
			// rest are created in runtime, so can delete
			delete pDeleteFolder;
		}
		pDeleteFolder = pNextOneToDelete;
	}
	extern bool bExternal_Entities_Init;
	bExternal_Entities_Init = false;

	// collect an entire list of all relevant filders (entitybank, scriptbank, images, particles, etc)
	extern void mapeditorexecutable_full_folder_refresh (void);
	mapeditorexecutable_full_folder_refresh();

	// Second scan purchased folder
	cFolderItem *pNewFolder = &MainEntityList;
	while (pNewFolder->m_pNext)
	{
		if (strnicmp (pNewFolder->m_sFolder.Get(), "purchased", strlen("purchased")) == NULL)
		{
			// force this folder to refresh
			pNewFolder->m_tFolderModify = 0;
			LPSTR pFolderFullPath = pNewFolder->m_sFolderFullPath.Get();
			RefreshEntityFolder(pFolderFullPath, pNewFolder);
			break;
		}
		pNewFolder = pNewFolder->m_pNext;
	}

	// Third scan ALL folders (store can place files and folders most places!)
	pNewFolder = &MainEntityList;
	while (pNewFolder->m_pNext)
	{
		// force this folder to refresh
		pNewFolder->m_tFolderModify = 0;
		LPSTR pFolderFullPath = pNewFolder->m_sFolderFullPath.Get();
		RefreshEntityFolder(pFolderFullPath, pNewFolder);
		pNewFolder = pNewFolder->m_pNext;
	}
}

char defaultWriteFolder[260];

void FindAWriteablefolder( void )
{
	if ( (SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, &defaultWriteFolder[0])) >= 0 ) {

		cstr tmp = defaultWriteFolder;
		tmp += "\\AppData";
		if (PathExist(tmp.Get()) == 1) {
			tmp += "\\Local\\TGC";
			if (PathExist(tmp.Get()) != 1) {
				_mkdir(tmp.Get());
			}
		}
		tmp += "\\";
		strcpy(defaultWriteFolder, tmp.Get());
	}
	else
	{
		//
		cstr tmp = GetDir();
		tmp += "\\";
		strcpy(defaultWriteFolder, tmp.Get());
	}
}

#include "Types.h"

void InitParseLuaScript(entityeleproftype *tmpeleprof)
{
	for (int i = 0; i < MAXPROPERTIESVARIABLES; i++) {
		tmpeleprof->PropertiesVariable.VariableType[i] = 0;
		strcpy(tmpeleprof->PropertiesVariable.Variable[i], "");
		strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], "");
		tmpeleprof->PropertiesVariable.VariableValueFrom[i] = 0.0f;
		tmpeleprof->PropertiesVariable.VariableValueTo[i] = 0.0f;
		strcpy(tmpeleprof->PropertiesVariable.VariableSectionDescription[i], "");
		strcpy(tmpeleprof->PropertiesVariable.VariableSectionEndDescription[i], "");
	}

	#ifdef WICKEDENGINE
	luadropdownlabels.clear();
	g_DLuaVariableNames.clear();
	#endif
}

void ParseLuaScriptWithElementID(entityeleproftype *tmpeleprof, char * script, int iObjID)
{
	char cScriptName[MAX_PATH];
	strcpy(cScriptName, script);
	int pos;
	for (pos = strlen(cScriptName); pos > 0; pos-- ) {
		if (cScriptName[pos] == '\\' || cScriptName[pos] == '/')
			break;
	}
	if (pos > 0) {
		strcpy(cScriptName, &cScriptName[pos+1]);
		if (strlen(cScriptName) > 4)
			if (cScriptName[strlen(cScriptName) - 4] == '.')
				cScriptName[strlen(cScriptName) - 4] = 0;
	}

	strcpy(tmpeleprof->PropertiesVariable.VariableScript, cScriptName);

	InitParseLuaScript(tmpeleprof);

	tmpeleprof->PropertiesVariableActive = 0;
	tmpeleprof->PropertiesVariable.iVariables = 0;
	tmpeleprof->PropertiesVariable.VariableDescription = "";
	//scriptbank\markers\storyinzone.lua fails.
	FILE* fScript = GG_fopen(script, "r");
	if (fScript)
	{
		char ctmp[4096];
		bool bFirstLine = true;
		while (!feof(fScript))
		{
			fgets(ctmp, 4095 , fScript);
			ctmp[4095] = 0;
			if (strlen(ctmp) > 0 && ctmp[strlen(ctmp) - 1] == '\n')
				ctmp[strlen(ctmp) - 1] = 0;
			int cadd = 0;
			char *find = (char *) pestrcasestr(ctmp, "-- DESCRIPTION:");
			if (!find) {
				find = (char *)pestrcasestr(ctmp, "--DESCRIPTION:");
				cadd = -1;
			}

			//PE: Make a raw description text first, then parse after all descriptions are merged.

			if (find) {
				if(find[15] == ' ')
					strcpy(ctmp, find + 16 + cadd);
				else
					strcpy(ctmp, find + 15 + cadd);

				if(bFirstLine)
					tmpeleprof->PropertiesVariable.VariableDescription = ctmp;
				else {
					tmpeleprof->PropertiesVariable.VariableDescription += " "; 
					tmpeleprof->PropertiesVariable.VariableDescription += ctmp;
				}
				//Activate Propertie Variables.
				bFirstLine = false;
			}

		}
		fclose(fScript);

		//Activate Propertie Variables.
		bFirstLine = false;
		//while [ , collect all variables.
		char *find = tmpeleprof->PropertiesVariable.VariableDescription.Get();
		char *SectionDescription = find;
		while ((find = (char *)pestrcasestr(find, "[")) && tmpeleprof->PropertiesVariable.iVariables < MAXPROPERTIESVARIABLES)
		{
			char *find2 = (char *)pestrcasestr(find, "]");
			if (find2) 
			{
				find2[0] = 0;
				find[0] = 0;
				if (strlen(SectionDescription) >= MAXVARIABLETEXTSIZELARGE)
				{
					//To Large.
					SectionDescription = find2 + 1;
				}
				else
				{
					if (tmpeleprof->PropertiesVariable.iVariables >= 1) 
					{
						//Add space
						strcpy(tmpeleprof->PropertiesVariable.VariableSectionDescription[tmpeleprof->PropertiesVariable.iVariables], " ");
						strcat(tmpeleprof->PropertiesVariable.VariableSectionDescription[tmpeleprof->PropertiesVariable.iVariables], SectionDescription);
					}
					else 
					{
						strcpy(tmpeleprof->PropertiesVariable.VariableSectionDescription[tmpeleprof->PropertiesVariable.iVariables], SectionDescription);
					}
					strcpy(tmpeleprof->PropertiesVariable.VariableSectionEndDescription[tmpeleprof->PropertiesVariable.iVariables], "");
					SectionDescription = find2 + 1;
					if (!pestrcasestr(SectionDescription, "[")) 
					{
						if (SectionDescription[0] != 0) 
						{
							strcpy(tmpeleprof->PropertiesVariable.VariableSectionEndDescription[tmpeleprof->PropertiesVariable.iVariables], SectionDescription);
						}
					}
					tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] = 0;
					strcpy(tmpeleprof->PropertiesVariable.Variable[tmpeleprof->PropertiesVariable.iVariables], find + 1);
					if (pestrcasestr(tmpeleprof->PropertiesVariable.Variable[tmpeleprof->PropertiesVariable.iVariables], "#"))
						tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] = 1; //float
					if (pestrcasestr(tmpeleprof->PropertiesVariable.Variable[tmpeleprof->PropertiesVariable.iVariables], "$"))
						tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] = 2; //string
					if (pestrcasestr(tmpeleprof->PropertiesVariable.Variable[tmpeleprof->PropertiesVariable.iVariables], "!"))
						tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] = 3; //bool
					#ifdef WICKEDENGINE
					if (pestrcasestr(tmpeleprof->PropertiesVariable.Variable[tmpeleprof->PropertiesVariable.iVariables], "@"))
						tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] = 4; // labelled int[]
					if (pestrcasestr(tmpeleprof->PropertiesVariable.Variable[tmpeleprof->PropertiesVariable.iVariables], "*"))
						tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] = 5; // user specified in seconds
					if (pestrcasestr(tmpeleprof->PropertiesVariable.Variable[tmpeleprof->PropertiesVariable.iVariables], "&"))
						tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] = 6; // should alter eleprof variable
					#endif			

					//Set default values search for =
					char *find3 = (char *)pestrcasestr(tmpeleprof->PropertiesVariable.Variable[tmpeleprof->PropertiesVariable.iVariables], "=");
					if (find3) 
					{
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[tmpeleprof->PropertiesVariable.iVariables], find3 + 1);
						
						find3[0] = 0; // remove = from Variable.
						//check if we got a range.
						char *find4 = (char *)pestrcasestr(tmpeleprof->PropertiesVariable.VariableValue[tmpeleprof->PropertiesVariable.iVariables], "(");
						if (find4) 
						{
							find4[0] = 0;
							char from[256];
							strcpy(from, find4 + 1);

							#ifdef WICKEDENGINE
							// Need to calculate the range differently for the dropdown type.
							if (tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] == 4)
							{
								// Count number of = symbols
								int iFirstEqualsIndex = -1;
								int iLastEqualsIndex = -1;
								for (int i = 0; i < strlen(from); i++)
								{
									if (from[i] == '=')
									{
										iLastEqualsIndex = i;
										if (iFirstEqualsIndex < 0)
											iFirstEqualsIndex = i;
									}
								}

								if (iFirstEqualsIndex > -1)
								{
									/* e.g. (1=Slow, 2=Fast, 3=Very Fast)*/
									/* or.  (0=AnimSetList)*/

									// Determine range.
									tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables]
										= atof(from + iFirstEqualsIndex - 1);

									tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables]
										= atof(from + iLastEqualsIndex - 1);

									// Store labels.
									char buffer[128];
									int count = tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables]
										- tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables];

									std::vector<std::string> labels;
									// First element is the variable number - needed for when there are more than one dropdown variables in a single script.
									char cVariable[8];
									sprintf(cVariable, "%d", tmpeleprof->PropertiesVariable.iVariables);
									labels.push_back(std::string(cVariable));

									for (int i = iFirstEqualsIndex; i < strlen(from); i++)
									{
										if (from[i] == '=')
										{
											int end = i;
											// Now find the comma or close bracket, to determine where the label starts and ends.
											for (int j = i+1; j < strlen(from); j++)
											{
												if (from[j] == ',' || from[j] == ')')
												{
													end = j;
													break;
												}
											}

											bool bExit = false;
											if (from[end] == ')')
												bExit = true;

											// copy the label into the buffer.
											strcpy(buffer, from + i+1);
											buffer[end - i - 1] = 0;

											i = end;

											labels.push_back(std::string(buffer));

											if (bExit) break;
										}
									}

									// can intercept label list here if special indicator that it is an animset list
									if (labels.size()==2 && iObjID > 0 )
									{
										if(stricmp(labels[1].c_str(),"animsetlist")==NULL)
										{
											labels.clear();
											labels.push_back(cVariable);
											extern sObject* GetObjectData (int);
											sObject* pObject = GetObjectData(iObjID);
											sAnimationSet* pAnimSet = pObject->pAnimationSet;
											int iAnimSetIndex = 0;
											if (pAnimSet)
											{
												if (stricmp(pAnimSet->szName, "all") == NULL)
												{
													pAnimSet = pAnimSet->pNext; // skip first one (base zero all character anims)
												}
											}
											while (pAnimSet)
											{
												if (stricmp(pAnimSet->szName, "mouthshapes") != NULL)
												{
													iAnimSetIndex++;
													labels.push_back(pAnimSet->szName);
												}
												pAnimSet = pAnimSet->pNext;
											}
											if (iAnimSetIndex > 0)
											{
												tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 1;
												tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = iAnimSetIndex;
											}
											else
											{
												tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 0;
												tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = 0;
											}
										}
									}

									// and add to official dropdown lists
									luadropdownlabels.push_back(labels);
								}
							}
							else
							{
								// Extract range normally.
								char *find5 = (char *)pestrcasestr(from, ",");
								if (find5)
								{
									find5[0] = 0;
									tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = atof(find4 + 1);
									char *find6 = (char *)pestrcasestr(find5 + 1, ")");
									if (find6)
									{
										find6[0] = 0;
										tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = atof(find5 + 1);
									}
								}
							}
							#else
							// Extract range normally.
							char *find5 = (char *)pestrcasestr(from, ",");
							if (find5)
							{
								find5[0] = 0;
								tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = atof(find4 + 1);
								char *find6 = (char *)pestrcasestr(find5 + 1, ")");
								if (find6)
								{
									find6[0] = 0;
									tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = atof(find5 + 1);
								}
							}
							#endif
						}
					}
					else
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[tmpeleprof->PropertiesVariable.iVariables], "");

					//Clean variable name.
					std::string clean_string = tmpeleprof->PropertiesVariable.Variable[tmpeleprof->PropertiesVariable.iVariables];
					replaceAll(clean_string, "#", "");
					replaceAll(clean_string, "$", "");
					replaceAll(clean_string, "!", "");
					#ifdef WICKEDENGINE
					replaceAll(clean_string, "@", "");
					replaceAll(clean_string, "*", "");
					replaceAll(clean_string, "&", "");
					if (tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] == 6)
					{
						g_DLuaVariableNames.push_back(clean_string);
					}
					#endif			
					strcpy(tmpeleprof->PropertiesVariable.Variable[tmpeleprof->PropertiesVariable.iVariables], clean_string.c_str());

					//Clean variablevalue.
					clean_string = tmpeleprof->PropertiesVariable.VariableValue[tmpeleprof->PropertiesVariable.iVariables];
					replaceAll(clean_string, "\"", ""); //cant use "
					strcpy(tmpeleprof->PropertiesVariable.VariableValue[tmpeleprof->PropertiesVariable.iVariables], clean_string.c_str());

					tmpeleprof->PropertiesVariable.iVariables++;
					tmpeleprof->PropertiesVariableActive = 1;
				}

				find = find2 + 1;
			}
			else find++;
		}


		//[RANGE] integer [SPEED#] float [TEXT$] string
		cstr sLuaScriptName = tmpeleprof->PropertiesVariable.VariableScript;
		sLuaScriptName += "_properties(";
		//Check if we need to update with new default values.
		if (!pestrcasestr(tmpeleprof->soundset4_s.Get(),sLuaScriptName.Get())) {
			if (tmpeleprof->PropertiesVariable.iVariables > 0) {
				tmpeleprof->soundset4_s = sLuaScriptName;
				//Add varables.
				for (int i = 0; i < tmpeleprof->PropertiesVariable.iVariables; i++) {

					char val[3];
					val[0] = tmpeleprof->PropertiesVariable.VariableType[i] + '0';
					val[1] = 0;

					tmpeleprof->soundset4_s += val;
					tmpeleprof->soundset4_s += "\"";
					std::string clean_string = tmpeleprof->PropertiesVariable.VariableValue[i];
					replaceAll(clean_string, "\"", ""); //cant use "
					tmpeleprof->soundset4_s += (char *)clean_string.c_str();
					//tmpeleprof->soundset4_s += tmpeleprof->PropertiesVariable.VariableValue[i];
					tmpeleprof->soundset4_s += "\"";
					if (i < tmpeleprof->PropertiesVariable.iVariables - 1)
						tmpeleprof->soundset4_s += ",";
				}
				tmpeleprof->soundset4_s += ")";
				//fclose(fScript);
				tmpeleprof->PropertiesVariableActive = 1;
				//return;
			}
		}

		//Get old value into arrayes.
		char tmp[4096];
		ZeroMemory(tmp, 4095);
		strcpy(tmp, tmpeleprof->soundset4_s.Get()); //dont change original.
//		char *find;
		int iNextVariable = 0;
		find = (char *)pestrcasestr(tmp, "(");
		if (find) {
			//cmd = &tmp[0];
			find[0] = 0;
			find++;
			while ((find = (char *)pestrcasestr(find, "\"")) && iNextVariable < tmpeleprof->PropertiesVariable.iVariables) {
				char *find2 = (char *)pestrcasestr(find + 1, "\"");
				if (find2) {
					int type = find[-1] - '0';
					find2[0] = 0;

					if (type >= 0 && type <= 9) {
						if (type == 1) {
							strcpy(tmpeleprof->PropertiesVariable.VariableValue[iNextVariable],find + 1);
						}
						else if (type == 2) {
							strcpy(tmpeleprof->PropertiesVariable.VariableValue[iNextVariable], find + 1);
						}
						else {
							strcpy(tmpeleprof->PropertiesVariable.VariableValue[iNextVariable], find + 1);
						}
					}
					find = find2 + 1;
				}
				else {
					find++;
				}

				iNextVariable++;
			}
		}

	}
}

void ParseLuaScript(entityeleproftype *tmpeleprof, char * script)
{
	ParseLuaScriptWithElementID(tmpeleprof, script, -1);
}

cstr DLUAFormatLabel(LPSTR pIn)
{
	// introduce a display space if the first word has a capital in the original variable label
	bool bDetectAnyLowerCase = false;
	for (int i = 0; i < strlen(pIn); i++)
	{
		unsigned char letter = pIn[i];
		if (letter >= 'a' && letter <= 'z')
		{
			bDetectAnyLowerCase = true;
			break;
		}
	}
	int iWordPos = 0;
	char pFullLabel[MAX_PATH];
	memset(pFullLabel, 0, sizeof(pFullLabel));
	if (bDetectAnyLowerCase == true)
	{
		// handle lower case word separating
		for (int i = 0; i < strlen(pIn); i++)
		{
			bool bUpper = false;
			unsigned char letter = pIn[i];
			if (letter >= 'A' && letter <= 'Z') bUpper = true;
			if (i > 0 && bUpper == true) pFullLabel[iWordPos++] = ' ';
			pFullLabel[iWordPos++] = letter;
		}
		pFullLabel[iWordPos] = 0;
	}
	else
	{
		// all or mostly upper case, treat as such
		strcpy (pFullLabel, pIn);
	}
	// format variable labels to convert _ to spaces
	for (int i = 0; i < strlen(pFullLabel); i++)
	{
		unsigned char letter = pFullLabel[i];
		if (letter == '_') pFullLabel[i] = ' ';
	}
	// finally ensure all lower case except for first letter of each word (now separated by space)
	strlwr(pFullLabel);
	bool bMakeUpperCase = true;
	for (int i = 0; i < strlen(pFullLabel); i++)
	{
		unsigned char letter = pFullLabel[i];
		if (bMakeUpperCase == true)
		{
			int iLwrToUpr = 'a' - 'A';
			pFullLabel[i] = pFullLabel[i] - iLwrToUpr;
			bMakeUpperCase = false;
		}
		if (letter == ' ') bMakeUpperCase = true;
	}
	cstr out = pFullLabel;
	return out;
}
float fDescriptionBoxSize = 0;
cstr sDescriptionBoxSize = "";
int DisplayLuaDescription(entityeleproftype *tmpeleprof)
{
	char tmp[2048];
	bool bUpdateMainString = false;
	int speech_entries = 0;
	float fPropertiesColoumWidth = ImGui::GetCursorPosX() + 110.0f;
	
	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	if (1) 
	{
		ImGui::PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_FrameBg]);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, style.FrameBorderSize);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.FramePadding);

		bool ret;

		if (fDescriptionBoxSize > ImGui::GetFontSize() * 7) 
		{
			ret = ImGui::BeginChild("##DLUADescriptionbox", ImVec2(ImGui::GetContentRegionAvailWidth() - 10, (ImGui::GetFontSize()*7)), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding );
		}
		else 
		{
			if (fDescriptionBoxSize == 0)
				ret = ImGui::BeginChild("##DLUADescriptionbox", ImVec2(ImGui::GetContentRegionAvailWidth() - 10, (ImGui::GetFontSize()*5)), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar);
			else
				ret = ImGui::BeginChild("##DLUADescriptionbox", ImVec2(ImGui::GetContentRegionAvailWidth() - 10, fDescriptionBoxSize), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar);
		}
		void fDisplayDescriptionBox(entityeleproftype *tmpeleprof, bool textonly = false);
		fDisplayDescriptionBox(tmpeleprof);
		ImGui::EndChild();

		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor();
	}
	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

	extern int speech_ids[5];

	for (int i = 0; i < tmpeleprof->PropertiesVariable.iVariables; i++) {

		bool speech = false;

		cstr tmpvar = tmpeleprof->PropertiesVariable.Variable[i];
		tmpvar = tmpvar.Lower();

		if (speech_entries <= 3) {
			if (tmpvar == "speech1" || tmpvar == "speech 1") {
				speech_ids[speech_entries++] = i; speech = true;
			}
			else if (tmpvar == "speech2" || tmpvar == "speech 2") {
				speech_ids[speech_entries++] = i; speech = true;
			}
			else if (tmpvar == "speech3" || tmpvar == "speech 3") {
				speech_ids[speech_entries++] = i; speech = true;
			}
			else if (tmpvar == "speech4" || tmpvar == "speech 4") {
				speech_ids[speech_entries++] = i; speech = true;
			}
			else if (tmpvar == "speech0" || tmpvar == "speech 0") {
				speech_ids[speech_entries++] = i; speech = true;
			}
		}
		if (!speech)
		{
			if (tmpeleprof->PropertiesVariable.VariableType[i] == 3)
			{
				// BOOL needs to be on one line
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::Text(DLUAFormatLabel(tmpeleprof->PropertiesVariable.Variable[i]).Get());
				ImGui::SameLine();
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
				ImGui::SetCursorPos(ImVec2(fPropertiesColoumWidth, ImGui::GetCursorPosY()));
			}
			else
			{
				// REST can use new gadget style
				ImGui::TextCenter(DLUAFormatLabel(tmpeleprof->PropertiesVariable.Variable[i]).Get());
			}

			ImGui::PushID(54321 + i); //unique id.
			ImGui::PushItemWidth(-10);
			cstr val = tmpeleprof->PropertiesVariable.VariableValue[i];
			if (tmpeleprof->PropertiesVariable.VariableType[i] == 1)
			{
				//Float
				float tmpfloat = atof(val.Get());
				#ifdef WICKEDENGINE
				if (tmpeleprof->PropertiesVariable.VariableValueTo[i] > 0 && tmpeleprof->PropertiesVariable.VariableValueTo[i] > tmpeleprof->PropertiesVariable.VariableValueFrom[i])
				{
					ImGui::PushItemWidth(-50);
					if (ImGui::SliderFloat("##floatslider", &tmpfloat, tmpeleprof->PropertiesVariable.VariableValueFrom[i], tmpeleprof->PropertiesVariable.VariableValueTo[i], " "))
					{
						sprintf(tmp, "%f", tmpfloat);
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmp);
						bUpdateMainString = true;
					}
					ImGui::SameLine();
					ImGui::PopItemWidth();
					ImGui::PushItemWidth(30);
					if (ImGui::InputFloat("##floatinput", &tmpfloat, 0, 0, "%.2f"))
					{
						// optionally cap within any specified range
						if (tmpfloat < tmpeleprof->PropertiesVariable.VariableValueFrom[i]) tmpfloat = tmpeleprof->PropertiesVariable.VariableValueFrom[i];
						if (tmpfloat > tmpeleprof->PropertiesVariable.VariableValueTo[i]) tmpfloat = tmpeleprof->PropertiesVariable.VariableValueTo[i];
					
						sprintf(tmp, "%f", tmpfloat);
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmp);
						bUpdateMainString = true;
					}
					ImGui::PopItemWidth();
				}
				else
				{
					cstr title = cstr("##float") + cstr(tmpeleprof->PropertiesVariable.Variable[i]);
					if (ImGui::InputFloat(title.Get(), &tmpfloat))
					{
						// ZJ: Range based floats are above.
						//// optionally cap within any specified range
						//if (tmpfloat < tmpeleprof->PropertiesVariable.VariableValueFrom[i]) tmpfloat = tmpeleprof->PropertiesVariable.VariableValueFrom[i];
						//if (tmpfloat > tmpeleprof->PropertiesVariable.VariableValueTo[i]) tmpfloat = tmpeleprof->PropertiesVariable.VariableValueTo[i];
					}
					sprintf(tmp, "%f", tmpfloat);
					strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmp);
					bUpdateMainString = true;
				}

				#else
				if (ImGui::InputFloat("##float", &tmpfloat)) 
				{
					if (tmpeleprof->PropertiesVariable.VariableValueFrom[i] > 0 && tmpeleprof->PropertiesVariable.VariableValueTo[i] > 0 && tmpeleprof->PropertiesVariable.VariableValueTo[i] > tmpeleprof->PropertiesVariable.VariableValueFrom[i])
					{
						// optionally cap within any specified range
						if (tmpfloat < tmpeleprof->PropertiesVariable.VariableValueFrom[i]) tmpfloat = tmpeleprof->PropertiesVariable.VariableValueFrom[i];
						if (tmpfloat > tmpeleprof->PropertiesVariable.VariableValueTo[i]) tmpfloat = tmpeleprof->PropertiesVariable.VariableValueTo[i];
					}
					sprintf(tmp, "%f", tmpfloat);
					strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmp);
					bUpdateMainString = true;
				}
				#endif
			}
			else if (tmpeleprof->PropertiesVariable.VariableType[i] == 2) 
			{
				//String
				char * imgui_setpropertyfile2(int group, char* data_s, char* field_s, char* desc_s, char* within_s);
				char * imgui_setpropertyfile2_v2(int group, char* data_s, char* field_s, char* desc_s, char* within_s, bool readonly, char *startsearch = NULL);;
				//Special setups.
				//VIDEO1, FILE-IMAGE for file selector.
				if (pestrcasestr(tmpvar.Get(), "file")) 
				{
					if (pestrcasestr(tmpvar.Get(), "image")) 
					{
						cstr tmpvalue = tmpeleprof->PropertiesVariable.VariableValue[i];
						#ifdef WICKEDENGINE
						bool readonly = false;

						#define IMGFILEID (PROPERTIES_CACHE_ICONS+998)
						static cstr imgfile = "";
						static int imgfile_preview_id = 0;
						if (tmpvalue != imgfile)
						{
							//Load new image preview.
							imgfile_preview_id = 0;
							if (tmpvalue != "")
							{
								image_setlegacyimageloading(true);
								LoadImage((char *)tmpvalue.Get(), IMGFILEID);
								image_setlegacyimageloading(false);
								imgfile_preview_id = IMGFILEID;
								if (!GetImageExistEx(IMGFILEID))
								{
									imgfile_preview_id = 0;
								}
							}
							imgfile = tmpvalue;
						}


						tmpvalue = imgui_setpropertyfile2_v2(1, tmpvalue.Get(), "", "Select image to appear in-level", "imagebank\\", readonly);

						if (imgfile_preview_id > 0 && GetImageExistEx(imgfile_preview_id))
						{
							extern ImVec4 drawCol_back;
							extern ImVec4 drawCol_normal;
							extern ImVec4 drawCol_hover;
							extern ImVec4 drawCol_Down;

							float w = ImGui::GetContentRegionAvailWidth();
							float iwidth = w;
							float ImgW = ImageWidth(imgfile_preview_id);
							float ImgH = ImageHeight(imgfile_preview_id);
							float fHighRatio = ImgH / ImgW;
							ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (iwidth*0.5), 0.0f));
							ImGui::ImgBtn(imgfile_preview_id, ImVec2( iwidth-18.0f, (iwidth - 18.0f) * fHighRatio), drawCol_back, drawCol_normal, drawCol_normal, drawCol_normal, -1, 0, 0, 0, true);
						}
						#else
						tmpvalue = imgui_setpropertyfile2(1, tmpvalue.Get(), "", "Select image to appear in-level", "scriptbank\\images\\imagesinzone\\");
						#endif
						if (tmpvalue != tmpeleprof->PropertiesVariable.VariableValue[i]) {
							strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmpvalue.Get());
							bUpdateMainString = true;
						}
					}
					else 
					{
						cstr tmpvalue = tmpeleprof->PropertiesVariable.VariableValue[i];
						tmpvalue = imgui_setpropertyfile2(1, tmpvalue.Get(), "", "Select File", "..\\files\\");
						if (tmpvalue != tmpeleprof->PropertiesVariable.VariableValue[i]) {
							strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmpvalue.Get());
							bUpdateMainString = true;
						}
					}
				}
				else if (tmpvar == "video1" || tmpvar == "video 1") 
				{
					cstr tmpvalue = tmpeleprof->soundset1_s;

					#ifdef WICKEDENGINE
					#define VIDEOFILEID (PROPERTIES_CACHE_ICONS+997)
					static cstr videofile = "";
					static int videofile_preview_id = 0;
					if (tmpvalue != videofile)
					{
						//Load new image preview.
						videofile_preview_id = 0;
						if (tmpvalue != "")
						{
							videofile_preview_id = VIDEOFILEID;

							std::string stmp = tmpvalue.Get();
							replaceAll(stmp, "videobank", ""); //Video thumbs stored without videobank.
							replaceAll(stmp, "\\\\", "\\"); //Remove double backslash.

							bool CreateBackBufferCacheName(char *file, int width, int height);
							extern cstr BackBufferCacheName;
							CreateBackBufferCacheName( (char *) stmp.c_str(), 512, 288);
							SetMipmapNum(1); //PE: mipmaps not needed.
							image_setlegacyimageloading(true);
							if (FileExist(BackBufferCacheName.Get()))
							{
								LoadImage((char *)BackBufferCacheName.Get(), videofile_preview_id);
							}
							image_setlegacyimageloading(false);
							SetMipmapNum(-1); //PE: mipmaps not needed.
							if (!GetImageExistEx(VIDEOFILEID))
							{
								videofile_preview_id = 0;
							}
						}
						videofile = tmpvalue;
					}

					#endif

					tmpvalue = imgui_setpropertyfile2(1, tmpeleprof->soundset1_s.Get(), "", "Specify movie file you would like to play when the player enters the zone", "videobank\\");
					if (tmpvalue != tmpeleprof->soundset1_s) 
					{
						tmpeleprof->soundset1_s = tmpvalue;
						bUpdateMainString = true;
					}

					#ifdef WICKEDENGINE
					if (videofile_preview_id > 0 && GetImageExistEx(videofile_preview_id))
					{
						extern ImVec4 drawCol_back;
						extern ImVec4 drawCol_normal;
						extern ImVec4 drawCol_hover;
						extern ImVec4 drawCol_Down;

						float w = ImGui::GetContentRegionAvailWidth();
						float iwidth = w;
						float ImgW = ImageWidth(videofile_preview_id);
						float ImgH = ImageHeight(videofile_preview_id);
						float fHighRatio = ImgH / ImgW;
						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (iwidth*0.5), 0.0f));
						ImGui::ImgBtn(videofile_preview_id, ImVec2(iwidth - 18.0f, (iwidth - 18.0f) * fHighRatio), drawCol_back, drawCol_normal, drawCol_normal, drawCol_normal, -1, 0, 0, 0, true);
					}
					#endif

				}
				else 
				{
					if (ImGui::InputText("##string", &tmpeleprof->PropertiesVariable.VariableValue[i][0], MAXVARIABLESIZE)) 
					{
						bUpdateMainString = true;
					}
					#ifdef WICKEDENGINE
					if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;
					#endif

				}
			}
			else if (tmpeleprof->PropertiesVariable.VariableType[i] == 3) 
			{
				//Bool
				bool tmpbool = atoi(val.Get());
				if( ImGui::Checkbox("##bool",&tmpbool) ) 
				{
					if(tmpbool)
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], "1");
					else
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], "0");
					bUpdateMainString = true;
				}
				// hack in tooltip for now, better idea is adding verbose to LUA itself!
				LPSTR pTooltipForTickbox = NULL;
				if (stricmp (tmpeleprof->PropertiesVariable.Variable[i], "FollowAPath") == NULL) pTooltipForTickbox = "Tick to all character to find and follow a nearby flag path";
				if (stricmp (tmpeleprof->PropertiesVariable.Variable[i], "CanRetreat") == NULL) pTooltipForTickbox = "Tick to allow the character to retreat when hurt";
				if (stricmp (tmpeleprof->PropertiesVariable.Variable[i], "StandGround") == NULL) pTooltipForTickbox = "Tick to ensure the character stays near their starting position";
				if (stricmp (tmpeleprof->PropertiesVariable.Variable[i], "FlankTarget") == NULL) pTooltipForTickbox = "Tick to allow the character to wide flank their target";
				if (stricmp (tmpeleprof->PropertiesVariable.Variable[i], "Alerted") == NULL) pTooltipForTickbox = "Tick to skip the initial suspicion and go direct to full alert";
				if (stricmp (tmpeleprof->PropertiesVariable.Variable[i], "Feeding") == NULL) pTooltipForTickbox = "Tick to start the zombie off in a ground feeding state";
				if (pTooltipForTickbox && ImGui::IsItemHovered()) ImGui::SetTooltip(pTooltipForTickbox);
			}
			#ifdef WICKEDENGINE
			else if (tmpeleprof->PropertiesVariable.VariableType[i] == 4)
			{
				// Dropdown of labelled integers.
				std::vector<std::string> labels;
				bool bGotLabels = false;
				// Retrieve the labels for this dropdown.
				for (int l = 0; l < luadropdownlabels.size(); l++)
				{
					labels = luadropdownlabels[l];

					// The first element is the variable number.
					if (atol(labels[0].c_str()) == i)
					{
						bGotLabels = true;
						break;
					}
				}
				
				if (bGotLabels)
				{
					// Determine the label for the currently selected value.
					int iSelectedIndex = atol(tmpeleprof->PropertiesVariable.VariableValue[i]) - tmpeleprof->PropertiesVariable.VariableValueFrom[i];

					// Since the first element of the labels is the variable number, add 1.
					iSelectedIndex++;

					// Ensure preview for combo is always valid
					const char* preview = "";
					if (iSelectedIndex < labels.size() ) preview = labels[iSelectedIndex].c_str();

					// No combi if no choices
					if (tmpeleprof->PropertiesVariable.VariableValueTo[i] == 0 && tmpeleprof->PropertiesVariable.VariableValueFrom[i] == 0)
					{
						// Indicate no choices (no drop down list or animations)
						ImGui::TextCenter("No choices available");
					}
					else
					{
						// Create combo now
						if (ImGui::BeginCombo("##DLUACOMBO", preview))
						{
							int iCount = tmpeleprof->PropertiesVariable.VariableValueTo[i] - tmpeleprof->PropertiesVariable.VariableValueFrom[i] + 1;
							for (int j = 0; j < iCount; j++)
							{
								bool bSelected = false;
								char labelID[128];
								int iValue = tmpeleprof->PropertiesVariable.VariableValueFrom[i] + j;
								sprintf(labelID, "%d", iValue);

								if (strcmp(labelID, tmpeleprof->PropertiesVariable.VariableValue[i]) == NULL)
									bSelected = true;

								char label[128];
								strcpy(label, labels[j + 1].c_str());

								if (ImGui::Selectable(label, bSelected))
								{
									strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], labelID);
									bUpdateMainString = true;
								}
								if (bSelected) ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}
					}

					// User needs to select a non-default next level.
					if (preview && strcmp(preview, "Go to Specific Level") == 0)
					{
						extern StoryboardStruct Storyboard;
						char title[MAX_PATH];
						if (strlen(tmpeleprof->ifused_s.Get()) > 0)
						{
							// Remove mapbank\ and .fpm from the level name.
							int offset = 0;
							if (strstr(tmpeleprof->ifused_s.Get(), "mapbank"))
								offset = 8;
							strcpy(title, tmpeleprof->ifused_s.Get() + offset);
							title[strlen(title) - 4] = 0;
						}
						else
							strcpy(title, "Select a level from your game project...");

						if (ImGui::BeginCombo("##LuaLevelCombo", title))
						{
							for (int i = 0; i < STORYBOARD_MAXNODES; i++)
							{
								if (Storyboard.Nodes[i].used && strlen(Storyboard.Nodes[i].level_name) > 0)
								{
									title[0] = 0;
									int offset = 0;
									if (strstr(Storyboard.Nodes[i].level_name, "mapbank"))
										offset = 8;
									strcpy(title, Storyboard.Nodes[i].level_name + offset);
									title[strlen(title) - 4] = 0;
									if (ImGui::Selectable(title))
									{
										strcpy(tmpeleprof->ifused_s.Get(), Storyboard.Nodes[i].level_name);
									}
								}
							}
							ImGui::EndCombo();
						}
					}
					else if (preview)
					{
						strcpy(tmpeleprof->ifused_s.Get(), "");
					}
				}
			}
			else if (tmpeleprof->PropertiesVariable.VariableType[i] == 5)
			{
				// integer - but converts seconds (from user) to milliseconds in actual stored value
				int tmpint = atoi(val.Get()) / 1000;
				LPSTR pTooltipForIntegerSlider = "Use slider to set the desired value.";
				if (stricmp (tmpeleprof->PropertiesVariable.Variable[i], "CombatTime") == NULL) pTooltipForIntegerSlider = "Set the time in seconds for how long the character stays in combat mode before returning to base";
				cstr id = cstr("##") + tmpeleprof->PropertiesVariable.VariableScript/*tmpeleprof->name_s*/ + cstr(tmpeleprof->PropertiesVariable.Variable[i]);
				if (tmpeleprof->PropertiesVariable.VariableValueFrom[i] > 0 && tmpeleprof->PropertiesVariable.VariableValueTo[i] > 0 && tmpeleprof->PropertiesVariable.VariableValueTo[i] > tmpeleprof->PropertiesVariable.VariableValueFrom[i])
				{
					if (ImGui::MaxSliderInputInt(id.Get(), &tmpint, (int)tmpeleprof->PropertiesVariable.VariableValueFrom[i], (int)tmpeleprof->PropertiesVariable.VariableValueTo[i], pTooltipForIntegerSlider))
					{
						sprintf(tmp, "%d", tmpint * 1000);
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmp);
						bUpdateMainString = true;
					}
				}
				else
				{
					if (ImGui::MaxSliderInputInt(id.Get(), &tmpint, 0.0F, 100.0f, pTooltipForIntegerSlider))
					{
						sprintf(tmp, "%d", tmpint * 1000);
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmp);
						bUpdateMainString = true;
					}
				}
			}
			else if (tmpeleprof->PropertiesVariable.VariableType[i] == 6)
			{
				// Should alter the matching eleprof variable value e.g. in script "&QUANTITY" is eleprof->quantity
				
				// Determine the index into g_DLuaVariableNames for this properties variable.
				int variableNameIndex = -1;
				for (int j = 0; j < MAXPROPERTIESVARIABLES; j++)
				{
					if (tmpeleprof->PropertiesVariable.VariableType[j] == 6)
					{
						variableNameIndex++;
						if (j == i)
							break;
					}
				}

				if (variableNameIndex >= 0 && variableNameIndex < g_DLuaVariableNames.size())
				{
					// Find the matching eleprof variable that we want to edit.
					std::string variableName = g_DLuaVariableNames[variableNameIndex];
					void* pVariable = nullptr;

					enum VariableType
					{
						NONE,
						INT,
						FLOAT,
						STRING,
						BOOL
					};
					VariableType type = NONE;
					
					if (stricmp(variableName.c_str(), "quantity") == 0)
					{
						type = INT;
						pVariable = &tmpeleprof->quantity;
					}
					else if (stricmp(variableName.c_str(), "damage") == 0)
					{
						type = INT;
						pVariable = &tmpeleprof->damage;
					}
					else if (stricmp(variableName.c_str(), "accuracy") == 0)
					{
						type = INT;
						pVariable = &tmpeleprof->accuracy;
					}
					else if (stricmp(variableName.c_str(), "reloadqty") == 0)
					{
						type = INT;
						pVariable = &tmpeleprof->reloadqty;
					}
					else if (stricmp(variableName.c_str(), "fireiterations") == 0)
					{
						type = INT;
						pVariable = &tmpeleprof->fireiterations;
					}
					else if (stricmp(variableName.c_str(), "range") == 0)
					{
						type = INT;
						pVariable = &tmpeleprof->range;
					}
					else if (stricmp(variableName.c_str(), "dropoff") == 0)
					{
						type = INT;
						pVariable = &tmpeleprof->dropoff;
					}

					// Display the correct ImGui gadget based on the variable type.
					switch (type)
					{
					case INT:
					{
						int* pEditVariable = (int*)pVariable;
						if (pEditVariable)
						{
							// Choose a range for the slider (1,100 if not specified in script).
							int from = 1;
							int to = 100;
							int scriptFrom = tmpeleprof->PropertiesVariable.VariableValueFrom[i];
							int scriptTo = tmpeleprof->PropertiesVariable.VariableValueTo[i];
							if (scriptFrom != scriptTo)
							{
								from = scriptFrom;
								to = scriptTo;
							}

							// Edit the variable via ImGui
							cstr sliderLabel = cstr("##") + cstr((char*)variableName.c_str());
							ImGui::MaxSliderInputInt(sliderLabel.Get(), pEditVariable, from, to, 0);
						}
						break;
					}
					case FLOAT:
						break;
					case STRING:
						break;
					case BOOL:
						break;
					}
				}
			}
			#endif
			else 
			{
				//Integer
				int tmpint = atoi(val.Get());
				#ifdef WICKEDENGINE
				// hack in tooltip for now, better idea is adding verbose to LUA itself!
				LPSTR pTooltipForIntegerSlider = "Use slider to set the desired value.";
				if (stricmp (tmpeleprof->PropertiesVariable.Variable[i], "RetreatRange") == NULL) pTooltipForIntegerSlider = "Set the distance the character will retreat to before stopping";
				if (stricmp (tmpeleprof->PropertiesVariable.Variable[i], "ChaseModes") == NULL) pTooltipForIntegerSlider = "The first three modes are slow walkers and the last two are fast walkers";
				cstr id = cstr("##") + tmpeleprof->PropertiesVariable.VariableScript/*tmpeleprof->name_s*/ + cstr(tmpeleprof->PropertiesVariable.Variable[i]);
				if (tmpeleprof->PropertiesVariable.VariableValueFrom[i] > 0 && tmpeleprof->PropertiesVariable.VariableValueTo[i] > 0 && tmpeleprof->PropertiesVariable.VariableValueTo[i] > tmpeleprof->PropertiesVariable.VariableValueFrom[i])
				{
					if (ImGui::MaxSliderInputInt(id.Get(), &tmpint, (int)tmpeleprof->PropertiesVariable.VariableValueFrom[i], (int)tmpeleprof->PropertiesVariable.VariableValueTo[i], pTooltipForIntegerSlider))
					{
						sprintf(tmp, "%d", tmpint);
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmp);
						bUpdateMainString = true;
					}
				}
				else
				{
					if (ImGui::MaxSliderInputInt(id.Get(), &tmpint, 0.0F, 100.0f, pTooltipForIntegerSlider))
					{
						sprintf(tmp, "%d", tmpint);
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmp);
						bUpdateMainString = true;
					}
				}
				#else	
				if (tmpeleprof->PropertiesVariable.VariableValueFrom[i] > 0 && tmpeleprof->PropertiesVariable.VariableValueTo[i] > 0 && tmpeleprof->PropertiesVariable.VariableValueTo[i] > tmpeleprof->PropertiesVariable.VariableValueFrom[i])
				{
					if (ImGui::MaxSliderInputInt("##integer", &tmpint, (int)tmpeleprof->PropertiesVariable.VariableValueFrom[i], (int)tmpeleprof->PropertiesVariable.VariableValueTo[i], "Use slider to set the desired value."))
					{
						sprintf(tmp, "%d", tmpint);
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmp);
						bUpdateMainString = true;
					}
				}
				else
				{
					if (ImGui::MaxSliderInputInt("##integer", &tmpint, 0.0F, 100.0f, "Use slider to set the desired value."))
					{
						sprintf(tmp, "%d", tmpint);
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmp);
						bUpdateMainString = true;
					}
				}
				#endif
			}
			ImGui::PopItemWidth();
			ImGui::PopID();
		}
	}

	//Update soundset4_s when we have changes.
	if (bUpdateMainString) 
	{
		cstr sLuaScriptName = tmpeleprof->PropertiesVariable.VariableScript;
		sLuaScriptName += "_properties(";
		//Check if we need to update with new default values.
		if (tmpeleprof->PropertiesVariable.iVariables > 0) 
		{
			tmpeleprof->soundset4_s = sLuaScriptName;
			//Add variables.
			for (int i = 0; i < tmpeleprof->PropertiesVariable.iVariables; i++) 
			{
				char val[3];
				val[0] = tmpeleprof->PropertiesVariable.VariableType[i] + '0';
				val[1] = 0;

				tmpeleprof->soundset4_s += val;
				tmpeleprof->soundset4_s += "\"";
				std::string clean_string = tmpeleprof->PropertiesVariable.VariableValue[i];
				replaceAll(clean_string, "\"", ""); //cant use "
				tmpeleprof->soundset4_s += (char *)clean_string.c_str();
				//tmpeleprof->soundset4_s += tmpeleprof->PropertiesVariable.VariableValue[i];
				tmpeleprof->soundset4_s += "\"";
				if (i < tmpeleprof->PropertiesVariable.iVariables - 1)
					tmpeleprof->soundset4_s += ",";
			}
			tmpeleprof->soundset4_s += ")";
		}
	}

	return speech_entries;
}


int DisplayLuaDescriptionOnly(entityeleproftype *tmpeleprof)
{
	char tmp[2048];
	bool bUpdateMainString = false;
	int speech_entries = 0;
	float fPropertiesColoumWidth = ImGui::GetCursorPosX() + 110.0f;

	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	if (1) {
		ImGui::PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_FrameBg]);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, style.FrameBorderSize);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.FramePadding);

		bool ret;
		if (fDescriptionBoxSize > ImGui::GetFontSize() * 7) {
			ret = ImGui::BeginChild("##DLUADescriptionbox", ImVec2(ImGui::GetContentRegionAvailWidth() - 10, ImGui::GetFontSize() * 7), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding);
		}
		else {
			if (fDescriptionBoxSize == 0)
				ret = ImGui::BeginChild("##DLUADescriptionbox", ImVec2(ImGui::GetContentRegionAvailWidth() - 10, ImGui::GetFontSize() * 5), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar);
			else
				ret = ImGui::BeginChild("##DLUADescriptionbox", ImVec2(ImGui::GetContentRegionAvailWidth() - 10, fDescriptionBoxSize), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar);
		}
		void fDisplayDescriptionBox(entityeleproftype *tmpeleprof, bool textonly = false);
		fDisplayDescriptionBox(tmpeleprof,true);
		ImGui::EndChild();

		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor();
	}
	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

	return 0;
}


void fDisplayDescriptionBox(entityeleproftype *tmpeleprof, bool textonly = false)
{
	struct Segment
	{
		Segment(const char* text, ImU32 col = 0, bool underline = false)
			: textStart(text)
			, textEnd(text + strlen(text))
			, colour(col)
			, underline(underline)
		{}

		const char* textStart;
		const char* textEnd;
		ImU32		colour;
		bool		underline;

		Segment() { colour = 0; underline = false; }

	};
	
	Segment segs[MAXPROPERTIESVARIABLES*3] = {};
	int curseg = 0;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	ImVec4 col = style.Colors[ImGuiCol_Text] * ImVec4(255.0f, 255.0f, 255.0f, 255.0f);

	if (textonly) 
	{
		segs[curseg++] = Segment(tmpeleprof->PropertiesVariable.VariableDescription.Get());
	}
	else 
	{
		for (int i = 0; i < tmpeleprof->PropertiesVariable.iVariables; i++)
		{
			segs[curseg++] = Segment(tmpeleprof->PropertiesVariable.VariableSectionDescription[i]);
			segs[curseg++] = Segment(tmpeleprof->PropertiesVariable.Variable[i], IM_COL32(col.x, col.y, col.z, col.w), true);
			if (strlen(tmpeleprof->PropertiesVariable.VariableSectionEndDescription[i]) > 0) 
			{
				segs[curseg++] = Segment(tmpeleprof->PropertiesVariable.VariableSectionEndDescription[i]);
			}
			if (curseg >= (MAXPROPERTIESVARIABLES * 3) - 1)
				break;
		}
	}
	float startpos = ImGui::GetCursorPosY();

	const float wrapWidth = ImGui::GetWindowContentRegionWidth();
	for (int i = 0; i < curseg; ++i)
	{
		const char* textStart = segs[i].textStart;
		const char* textEnd = segs[i].textEnd ? segs[i].textEnd : textStart + strlen(textStart);

		ImFont* Font = ImGui::GetFont();

		do
		{
			float widthRemaining = ImGui::CalcWrapWidthForPos(ImGui::GetCursorScreenPos(), 0.0f);
			widthRemaining *= 1.65;
			const char* drawEnd = ImGui::CalcWordWrapPositionB(1.0f, textStart, textEnd, wrapWidth, wrapWidth - widthRemaining); //, wrapWidth - widthRemaining);
			if (textStart == drawEnd)
			{
				ImGui::NewLine();
				drawEnd = ImGui::CalcWordWrapPositionB(1.0f, textStart, textEnd, wrapWidth, wrapWidth - widthRemaining); //, wrapWidth - widthRemaining);
			}

			if (segs[i].colour)
				ImGui::PushStyleColor(ImGuiCol_Text, segs[i].colour);
			ImGui::TextUnformatted(textStart, textStart == drawEnd ? nullptr : drawEnd);
			if (segs[i].colour)
				ImGui::PopStyleColor();
			if (segs[i].underline)
			{
				ImVec2 lineEnd = ImGui::GetItemRectMax();
				ImVec2 lineStart = lineEnd;
				lineStart.x = ImGui::GetItemRectMin().x;
				ImGui::GetWindowDrawList()->AddLine(lineStart, lineEnd, segs[i].colour);

				//if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly))
				//	ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
			}

			if (textStart == drawEnd || drawEnd == textEnd)
			{
				ImGui::SameLine(0.0f, 0.0f);
				break;
			}

			textStart = drawEnd;

			while (textStart < textEnd)
			{
				const char c = *textStart;
				if (ImCharIsBlankA(c)) { textStart++; }
				else if (c == '\n') { textStart++; break; }
				else { break; }
			}
		} while (true);
	}

	fDescriptionBoxSize = (ImGui::GetCursorPosY() - startpos) + (ImGui::GetFontSize()*2.0);

}

float fDisplaySegmentText(char *text)
{
	if (!text) return(0);
	struct Segment
	{
		Segment(const char* text, ImU32 col = 0, bool underline = false)
			: textStart(text)
			, textEnd(text + strlen(text))
			, colour(col)
			, underline(underline)
		{}

		const char* textStart;
		const char* textEnd;
		ImU32		colour;
		bool		underline;

		Segment() { colour = 0; underline = false; }

	};

	Segment segs[MAXPROPERTIESVARIABLES * 3] = {};
	int curseg = 0;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	ImVec4 col = style.Colors[ImGuiCol_Text] * ImVec4(255.0f, 255.0f, 255.0f, 255.0f);

	segs[curseg++] = Segment(text);
	
	float startpos = ImGui::GetCursorPosY();

	const float wrapWidth = ImGui::GetWindowContentRegionWidth();
	for (int i = 0; i < curseg; ++i)
	{
		const char* textStart = segs[i].textStart;
		const char* textEnd = segs[i].textEnd ? segs[i].textEnd : textStart + strlen(textStart);

		ImFont* Font = ImGui::GetFont();

		do
		{
			float widthRemaining = ImGui::CalcWrapWidthForPos(ImGui::GetCursorScreenPos(), 0.0f);
			widthRemaining *= 1.65;
			const char* drawEnd = ImGui::CalcWordWrapPositionB(1.0f, textStart, textEnd, wrapWidth, wrapWidth - widthRemaining); //, wrapWidth - widthRemaining);
			if (textStart == drawEnd)
			{
				ImGui::NewLine();
				drawEnd = ImGui::CalcWordWrapPositionB(1.0f, textStart, textEnd, wrapWidth, wrapWidth - widthRemaining); //, wrapWidth - widthRemaining);
			}

			if (segs[i].colour)
				ImGui::PushStyleColor(ImGuiCol_Text, segs[i].colour);
			ImGui::TextUnformatted(textStart, textStart == drawEnd ? nullptr : drawEnd);
			if (segs[i].colour)
				ImGui::PopStyleColor();
			if (segs[i].underline)
			{
				ImVec2 lineEnd = ImGui::GetItemRectMax();
				ImVec2 lineStart = lineEnd;
				lineStart.x = ImGui::GetItemRectMin().x;
				ImGui::GetWindowDrawList()->AddLine(lineStart, lineEnd, segs[i].colour);
			}

			if (textStart == drawEnd || drawEnd == textEnd)
			{
				ImGui::SameLine(0.0f, 0.0f);
				break;
			}

			textStart = drawEnd;

			while (textStart < textEnd)
			{
				const char c = *textStart;
				if (ImCharIsBlankA(c)) { textStart++; }
				else if (c == '\n') { textStart++; break; }
				else { break; }
			}
		} while (true);
	}
	return( (ImGui::GetCursorPosY() - startpos) + (ImGui::GetFontSize()*2.0) );
}

void UniversalKeyboardShortcutAddItem(int iIconID, int iIcon2ID, LPSTR pLabel)
{
	ImGui::Indent(10);
	// standard spacing settings
	int iRightColumn = 70;
	int iKeyIconSize = 26;
	float fShortcutTextSpacing = 5.0f;
	float fShortcutVerticalSpacing = -10.0f;

	if (iIconID > 0)
	{
		float fSize = 1.0f;
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x - 4.0f, ImGui::GetCursorPos().y));
	

		ImGui::ImgBtn(iIconID, ImVec2(iKeyIconSize*fSize, iKeyIconSize*fSize), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), 0, 0, 0, 0, false, false, false, false);
		LPSTR pTooltip = "";
		if (iIcon2ID != 0)
		{
			if (iIconID == KEY_TAB) pTooltip = "Hold down the TAB key";
			if (iIconID == KEY_SHIFT) pTooltip = "Hold down the SHIFT key";
			if (iIconID == KEY_CONTROL) pTooltip = "Hold down the CONTROL key";
			if (iIconID == KEY_CONTROL_SHIFT) pTooltip = "Hold down the CONTROL and SHIFT keys";
			if (iIconID == KEY_ALT) pTooltip = "Hold down the ALT key";
		}
		else
		{
			if (iIconID == KEY_TAB) pTooltip = "Press the TAB key";
			if (iIconID == KEY_SHIFT) pTooltip = "Press the SHIFT key";
			if (iIconID == KEY_CONTROL) pTooltip = "Press the CONTROL key";
			if (iIconID == KEY_CONTROL_SHIFT) pTooltip = "Press the CONTROL and SHIFT keys";
			if (iIconID == KEY_ALT) pTooltip = "Press the ALT key";

		}
		if (iIconID == KEY_KEYBOARD) pTooltip = "Use the WASD or the arrow keys";
		if (iIconID == MOUSE_LMB) pTooltip = "Use the left mouse button";
		if (iIconID == MOUSE_MMB) pTooltip = "Use the mouse wheel";
		if (iIconID == MOUSE_RMB) pTooltip = "Use the right mouse button";
		if (iIconID == KEY_BACKSPACE) pTooltip = "Press the BACKSPACE key";
		if (iIconID == KEY_R) pTooltip = "Press the R key";
		if (iIconID == KEY_Y) pTooltip = "Press the Y key";
		if (iIconID == KEY_F) pTooltip = "Press the F key";
		if (iIconID == KEY_G) pTooltip = "Press the G key";
		if (iIconID == KEY_I) pTooltip = "Press the I key";
		if (iIconID == KEY_N) pTooltip = "Press the N key";
		if (iIconID == KEY_L) pTooltip = "Press the L key";
		if (iIconID == KEY_E) pTooltip = "Press the E key";
		if (iIconID == KEY_Q) pTooltip = "Press the Q key";
		if (iIconID == KEY_SPACE) pTooltip = "Press the SPACE key";
		if (iIconID == KEY_DELETE) pTooltip = "Press the DELETE key";
		if (iIconID == KEY_RETURN) pTooltip = "Press the ENTER key";
		if (iIconID == KEY_PGUP) pTooltip = "Press the PAGEUP key";
		if (iIconID == KEY_PGDN) pTooltip = "Press the PAGEDOWN key";
		if (iIconID == KEY_O) pTooltip = "Press the O key";
		if (iIconID == KEY_T) pTooltip = "Press the T key";
		if (iIconID == KEY_Z) pTooltip = "Press the Z key";
		LPSTR pTooltip2 = "";
		ImGui::SameLine();
		if (iIcon2ID != 0)
		{
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x - 14.0f - ((fSize - 1.0f)*4.0f), ImGui::GetCursorPos().y));
			ImGui::ImgBtn(KEY_SEPARATOR_SMALL, ImVec2((float)iKeyIconSize*0.5*fSize, iKeyIconSize*fSize), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), 0, 0, 0, 0, false, false, false, false);
			ImGui::SameLine();
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x - 14.0f - ((fSize - 1.0f)*4.0f), ImGui::GetCursorPos().y));
			ImGui::ImgBtn(iIcon2ID, ImVec2(iKeyIconSize*fSize, iKeyIconSize*fSize), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), 0, 0, 0, 0, false, false, false, false);
			if (iIcon2ID == KEY_PLUS) pTooltip2 = "Press the PLUS key";
			if (iIcon2ID == KEY_MINUS) pTooltip2 = "Press the MINUS key";
			if (iIcon2ID == KEY_SHIFT) pTooltip2 = "Press the SHIFT key";
			if (iIcon2ID == MOUSE_LMB) pTooltip2 = "Use the left mouse button";
			if (iIcon2ID == MOUSE_RMB) pTooltip2 = "Use the right mouse button";
			if (iIcon2ID == KEY_BACKSPACE) pTooltip2 = "Press the BACKSPACE key";
			if (iIcon2ID == KEY_R) pTooltip2 = "Press the R key";
			if (iIcon2ID == KEY_G) pTooltip2 = "Press the G key";
			if (iIcon2ID == KEY_I) pTooltip2 = "Press the I key";
			if (iIcon2ID == KEY_SPACE) pTooltip2 = "Press the SPACE key";
			if (iIcon2ID == KEY_N) pTooltip2 = "Press the N key";
			if (iIcon2ID == KEY_L) pTooltip2 = "Press the L key";
			if (iIcon2ID == KEY_E) pTooltip2 = "Press the E key";
			if (iIcon2ID == KEY_DELETE) pTooltip2 = "Press the DELETE key";
			if (iIcon2ID == KEY_Y) pTooltip2 = "Press the Y key";
			if (iIcon2ID == KEY_RETURN) pTooltip2 = "Press the ENTER key";
			if (iIconID == KEY_O) pTooltip = "Press the O key";
			if (iIconID == KEY_T) pTooltip = "Press the T key";
			if (iIconID == KEY_Z) pTooltip = "Press the Z key";

			ImGui::SameLine();
		}
		int iNewRightColumn = iRightColumn;
		if (fSize > 1.0 && iIcon2ID != 0) iNewRightColumn = iRightColumn + 42.0f;
		ImGui::SetCursorPos(ImVec2(iNewRightColumn, ImGui::GetCursorPos().y + fShortcutTextSpacing));
		//if (bEnlargingKeyIcon == true)
		//{
			cstr pFullLabel = pTooltip;
			if (strlen(pTooltip2) > 0)
			{
				pFullLabel += " and ";
				pFullLabel += pTooltip2;
				//fShortcutVerticalSpacing += 12.0f;
			}
			else
			{
				//fShortcutVerticalSpacing += 8.0f;
			}
			pFullLabel += " to ";
			pFullLabel += pLabel;
			//pFullLabel += ".";
			//ImGui::TextWrapped(pFullLabel.Get());
		//}
		//else
		//{
			ImGui::TextWrapped(pLabel);

			if (ImGui::IsItemHovered()) ImGui::SetTooltip(pFullLabel.Get());
		//}
	}
	else
	{
		// separator
		//ImGui::Text("");
	}
	ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, fShortcutVerticalSpacing));
	ImGui::Indent(-10);
}

//void UniversalKeyboardShortcutAddItem(int iIconID, int iIcon2ID, LPSTR pLabel)
//{
//	// standard spacing settings
//	int iRightColumn = 60;
//	int iKeyIconSize = 26;
//	float fShortcutTextSpacing = 5.0f;
//	float fShortcutVerticalSpacing = -10.0f;
//
//	if (iIconID > 0)
//	{
//
//		float fSize = 1.0f;
//		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x - 4.0f, ImGui::GetCursorPos().y));
//		ImRect bbwin(ImGui::GetWindowPos()+ImGui::GetCursorPos()+ImVec2(0,-ImGui::GetScrollY()), ImGui::GetWindowPos() + ImGui::GetCursorPos() + ImVec2(0, -ImGui::GetScrollY()) + ImVec2(ImGui::GetWindowSize().x,ImGui::GetFontSize()*2.0));
//		// enlarge key icon if hover over it
//		bool bEnlargingKeyIcon = false;
//		if (ImGui::IsMouseHoveringRect(bbwin.Min, bbwin.Max))
//		{
//			//fSize = 1.75f;
//			ImGuiWindow* window = ImGui::GetCurrentWindow();
//			ImVec4 back_col = ImGui::GetStyle().Colors[ImGuiCol_ChildBg];
//			ImVec2 offset = { -10.0f,8.0f };
//			window->DrawList->AddRectFilled(bbwin.Min + offset, bbwin.Max + offset, ImGui::GetColorU32(back_col), 0.0f, 0.0f);
//			//bEnlargingKeyIcon = true;
//		}
//
//		ImGui::ImgBtn(iIconID, ImVec2(iKeyIconSize*fSize, iKeyIconSize*fSize), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), 0, 0, 0, 0, false, false, false, false);
//		LPSTR pTooltip = "";
//		if (iIcon2ID != 0)
//		{
//			if (iIconID == KEY_TAB) pTooltip = "Hold down the TAB key";
//			if (iIconID == KEY_SHIFT) pTooltip = "Hold down the SHIFT key";
//			if (iIconID == KEY_CONTROL) pTooltip = "Hold down the CONTROL key";
//			if (iIconID == KEY_CONTROL_SHIFT) pTooltip = "Hold down the CONTROL and SHIFT keys";
//			if (iIconID == KEY_ALT) pTooltip = "Hold down the ALT key";
//		}
//		else
//		{
//			if (iIconID == KEY_TAB) pTooltip = "Press the TAB key";
//			if (iIconID == KEY_SHIFT) pTooltip = "Press the SHIFT key";
//			if (iIconID == KEY_CONTROL) pTooltip = "Press the CONTROL key";
//			if (iIconID == KEY_CONTROL_SHIFT) pTooltip = "Press the CONTROL and SHIFT keys";
//			if (iIconID == KEY_ALT) pTooltip = "Press the ALT key";
//		}
//		if (iIconID == KEY_KEYBOARD) pTooltip = "Use the W, A, S and D keys";
//		if (iIconID == MOUSE_LMB) pTooltip = "Use the left mouse button";
//		if (iIconID == MOUSE_MMB) pTooltip = "Use the mouse wheel";
//		if (iIconID == MOUSE_RMB) pTooltip = "Use the right mouse button";
//		if (iIconID == KEY_BACKSPACE) pTooltip = "Press the BACKSPACE key";
//		if (iIconID == KEY_R) pTooltip = "Press the R key";
//		if (iIconID == KEY_Y) pTooltip = "Press the Y key";
//		if (iIconID == KEY_F) pTooltip = "Press the F key";
//		if (iIconID == KEY_G) pTooltip = "Press the G key";
//		if (iIconID == KEY_N) pTooltip = "Press the N key";
//		if (iIconID == KEY_L) pTooltip = "Press the L key";
//		if (iIconID == KEY_E) pTooltip = "Press the E key";
//		if (iIconID == KEY_SPACE) pTooltip = "Press the SPACE key";
//		if (iIconID == KEY_DELETE) pTooltip = "Press the DELETE key";
//		if (iIconID == KEY_RETURN) pTooltip = "Press the ENTER key";
//		if (iIconID == KEY_PGUP) pTooltip = "Press the PAGEUP key";
//		if (iIconID == KEY_PGDN) pTooltip = "Press the PAGEDOWN key";
//		LPSTR pTooltip2 = "";
//		ImGui::SameLine();
//		if (iIcon2ID != 0)
//		{
//			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x - 14.0f - ((fSize-1.0f)*4.0f), ImGui::GetCursorPos().y));
//			ImGui::ImgBtn(KEY_SEPARATOR_SMALL, ImVec2((float)iKeyIconSize*0.5*fSize, iKeyIconSize*fSize), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), 0, 0, 0, 0, false, false, false, false);
//			ImGui::SameLine();
//			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x - 14.0f - ((fSize - 1.0f)*4.0f), ImGui::GetCursorPos().y));
//			ImGui::ImgBtn(iIcon2ID, ImVec2(iKeyIconSize*fSize, iKeyIconSize*fSize), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), 0, 0, 0, 0, false, false, false, false);
//			if (iIcon2ID == KEY_PLUS) pTooltip2 = "Press the PLUS key";
//			if (iIcon2ID == KEY_MINUS) pTooltip2 = "Press the MINUS key";
//			if (iIcon2ID == KEY_SHIFT) pTooltip2 = "Press the SHIFT key";
//			if (iIcon2ID == MOUSE_LMB) pTooltip2 = "Use the left mouse button";
//			if (iIcon2ID == MOUSE_RMB) pTooltip2 = "Use the right mouse button";
//			if (iIcon2ID == KEY_BACKSPACE) pTooltip2 = "Press the BACKSPACE key";
//			if (iIcon2ID == KEY_R) pTooltip2 = "Press the R key";
//			if (iIcon2ID == KEY_G) pTooltip2 = "Press the G key";
//			if (iIcon2ID == KEY_SPACE) pTooltip2 = "Press the SPACE key";
//			if (iIcon2ID == KEY_N) pTooltip2 = "Press the N key";
//			if (iIcon2ID == KEY_L) pTooltip2 = "Press the L key";
//			if (iIcon2ID == KEY_E) pTooltip2 = "Press the E key";
//			if (iIcon2ID == KEY_DELETE) pTooltip2 = "Press the DELETE key";
//			if (iIcon2ID == KEY_Y) pTooltip2 = "Press the Y key";
//			if (iIcon2ID == KEY_RETURN) pTooltip2 = "Press the ENTER key";
//			ImGui::SameLine();
//		}
//		int iNewRightColumn = iRightColumn;
//		if (fSize > 1.0 && iIcon2ID != 0) iNewRightColumn = iRightColumn + 42.0f;
//		ImGui::SetCursorPos(ImVec2(iNewRightColumn, ImGui::GetCursorPos().y + fShortcutTextSpacing));
//		if (bEnlargingKeyIcon == true)
//		{
//			cstr pFullLabel = pTooltip;
//			if (strlen(pTooltip2) > 0)
//			{
//				pFullLabel += " and ";
//				pFullLabel += pTooltip2;
//				fShortcutVerticalSpacing += 12.0f;
//			}
//			else
//			{
//				fShortcutVerticalSpacing += 8.0f;
//			}
//			pFullLabel += " to ";
//			pFullLabel += pLabel;
//			pFullLabel += ".";
//			ImGui::TextWrapped(pFullLabel.Get());
//		}
//		else
//		{
//			ImGui::TextWrapped(pLabel);
//		}
//	}
//	else
//	{
//		// separator
//		ImGui::Text("");
//	}
//	ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, fShortcutVerticalSpacing));
//}

int iContentHeight[eKST_Last]; //PE: Support multiply different panels open at the same time.
float fLastContentHeight[eKST_Last];

void UniversalKeyboardShortcut(eKeyboardShortcutType KST)
{
	#ifdef WICKEDENGINE

	if (pref.iHideKeyboardShortcuts)
		return;

	extern int iLastOpenHeader;
	extern bool bStoryboardWindow;
	int iHeader = 30;
	if (KST == eKST_ObjectMode)
		iHeader = 20;

	if (!bStoryboardWindow && pref.bAutoClosePropertySections && iLastOpenHeader != iHeader)
		ImGui::SetNextItemOpen(false, ImGuiCond_Always);

	// called to create Keyboard Shortcut component for all software
	if (ImGui::StyleCollapsingHeader("Keyboard Shortcuts", ImGuiTreeNodeFlags_DefaultOpen))
	{
		iLastOpenHeader = iHeader;

		ImVec2 vChildSize = { 0,0 };
		if (fLastContentHeight[KST] > 20)
			vChildSize.y = fLastContentHeight[KST];

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
		ImGui::BeginChild("##KeyboardShortcutsChild", vChildSize, false, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_None | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
		ImGui::PopStyleColor();
		iContentHeight[KST] = 0;
		// indent to start list of shortcuts
		ImGui::Indent(10);
		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 4.0f));

		// record start of keyboard shortcut content
		float fStartOfKeyboardShortcutContent = ImGui::GetCursorPos().y;

		// additional common controls for specific modes
		bool bShowCameraViewShortcuts = false;
		if (KST == eKST_Sculpt || KST == eKST_Paint || KST == eKST_AddVeg || KST == eKST_ObjectMode) bShowCameraViewShortcuts = true;
		if (bShowCameraViewShortcuts == true)
		{
			ImGui::Text("General Shortcuts"); iContentHeight[KST]++;
			//UniversalKeyboardShortcutAddItem(0, 0, ""); iContentHeight[KST]++; //PE: Removed for consistency with other sub sections.
			UniversalKeyboardShortcutAddItem(KEY_KEYBOARD, 0, "Move camera around the map"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_E, 0, "Move camera up"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_Q, 0, "Move camera down"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_SHIFT, 0, "Move camera around quickly"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, 0, "Move camera around slowly"); iContentHeight[KST]++;
			#ifdef WICKEDENGINE
			extern bool bEditorInFreeFlightMode;
			if (bEditorInFreeFlightMode == true)
			{
				UniversalKeyboardShortcutAddItem(MOUSE_RMB, 0, "Rotate camera view"); iContentHeight[KST]++;
			}
			#endif
			UniversalKeyboardShortcutAddItem(MOUSE_MMB, 0, "Zoom in and out"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_O, 0, "Enable object tools"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_T, 0, "Enable terrain tools"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_F, 0, "Toggle camera view"); iContentHeight[KST]++;

		}

		float fSectionSpacer = 8.0f;

		// shortcuts for each type
		switch (KST)
		{
		case eKST_Sculpt:
			if( bShowCameraViewShortcuts ) ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, fSectionSpacer));
			ImGui::Text("Sculpt Shortcuts"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(MOUSE_LMB, 0, "Raise and Lower the terrain"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_TAB, 0, "Toggle between raising and lowering"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_PLUS, 0, "Increase the brush size"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_MINUS, 0, "Decrease the brush size"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, MOUSE_MMB, "Increase/Decrease the brush size"); iContentHeight[KST]++;
			break;
		case eKST_Paint:
			if (bShowCameraViewShortcuts) ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, fSectionSpacer));
			ImGui::Text("Paint Shortcuts"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(MOUSE_LMB, 0, "Paint and clear terrain texture"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_TAB, 0, "Toggle between painting modes"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_PLUS, 0, "Increase the brush size"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_MINUS, 0, "Decrease the brush size"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, MOUSE_MMB, "Increase/Decrease the brush size"); iContentHeight[KST]++;
			break;
		case eKST_AddVeg:
			if (bShowCameraViewShortcuts) ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, fSectionSpacer));
			ImGui::Text("Vegetation Shortcuts"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(MOUSE_LMB, 0, "Add and remove terrain vegetation"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_TAB, 0, "Toggle between adding and removing"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_PLUS, 0, "Increase the brush size"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_MINUS, 0, "Decrease the brush size"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, MOUSE_MMB, "Increase/Decrease the brush size"); iContentHeight[KST]++;
			break;
		case eKST_ObjectMode:
			if (bShowCameraViewShortcuts) ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, fSectionSpacer));
			ImGui::Text("Object Tools Shortcuts"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(MOUSE_LMB, 0, "Drag camera around"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_SHIFT, MOUSE_LMB, "Select multiple objects"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_R, 0, "Rotate selected object"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_SHIFT, KEY_R, "Rotate object quickly"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, KEY_R, "Rotate object slowly"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_TAB, 0, "Toggle between positioning modes"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_DELETE, 0, "Delete selected object(s)"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, KEY_Z, "Undo"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, KEY_Y, "Redo"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_Y, 0, "Toggle object static and dynamic"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_G, 0, "Toggle between grid modes"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_I, 0, "Toggle spray objects"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_RETURN, 0, "Place object on nearest surface"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_PGUP, 0, "Move object upward"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_PGDN, 0, "Move object downward"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, KEY_G, "Group selected objects"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL_SHIFT, KEY_G, "UnGroup selected objects"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, KEY_L, "Lock/unlock selected object"); iContentHeight[KST]++;

			iContentHeight[KST]++; //Extra for wrapping.
			iContentHeight[KST]++; //Extra for wrapping.
			iContentHeight[KST]++; //Extra for wrapping.
			break;
		case eKST_CharacterCreator:
			if (bShowCameraViewShortcuts) ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, fSectionSpacer));
			ImGui::Text("Character Creator Shortcuts"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_KEYBOARD, 0, "Move the camera around"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(MOUSE_RMB, 0, "Look around with the camera"); iContentHeight[KST]++;
			break;
		case eKST_ObjectLibrary:
		case eKST_TerrainGenerator:
			if (bShowCameraViewShortcuts) ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, fSectionSpacer));
			if (KST == eKST_TerrainGenerator)
			{
				ImGui::Text("Terrain Generator Shortcuts"); iContentHeight[KST]++;
				UniversalKeyboardShortcutAddItem(KEY_KEYBOARD, 0, "Move camera around the map"); iContentHeight[KST]++;

			}
			else
			{
				ImGui::Text("Object Library Shortcuts"); iContentHeight[KST]++;
			}
			UniversalKeyboardShortcutAddItem(MOUSE_LMB, 0, "Move the terrain preview"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(MOUSE_RMB, 0, "Rotate the terrain preview"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(MOUSE_MMB, 0, "Zoom in and out of the terrain preview"); iContentHeight[KST]++;
			break;
		case eKST_Storyboard:
			if (bShowCameraViewShortcuts) ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, fSectionSpacer));
			ImGui::Text("Storyboard Shortcuts"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(MOUSE_LMB, 0, "Drag screen around"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_SHIFT, MOUSE_LMB, "Select multiple nodes"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, KEY_N, "Add new level"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, KEY_L, "Add existing level"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, KEY_SPACE, "Play game"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, KEY_E, "Save standalone"); iContentHeight[KST]++;

			break;
		}

		// restore indent
		ImGui::Indent(-10);

		float fShortcutTextSpacing = 6.6f;
		//PE: TODO - We need to calcutate wrapping lines also for this to be precise.
		//PE: We need some additional height when we are using wrapping.
		if(iContentHeight[KST] <= 5)
			fLastContentHeight[KST] = (float) (iContentHeight[KST] + 2) * (ImGui::GetFontSize()+ fShortcutTextSpacing);
		else
			fLastContentHeight[KST] = (float)(iContentHeight[KST] + 5) * (ImGui::GetFontSize() + fShortcutTextSpacing);

		if (bShowCameraViewShortcuts) fLastContentHeight[KST] += fSectionSpacer;

		// finished panel
		ImGui::EndChild();
	}
	#endif
}

void coreResetIMGUIFunctionalityPrefs(void)
{
	// this is called when MAXVERSION incremented
	// and restores/resets all flags so that each NEW build
	// provides users with a view of the STANDARD USER MOD�
	// not advanced or developer tools modes
	#ifdef WICKEDENGINE
	pref.current_style = 25; // 0
	pref.bHideTutorials = false;
	pref.bMultiplyOpenHeaders = false;
	pref.bAutoClosePropertySections = true;
	pref.bDisableMultipleViewport = false;
	pref.bAutoOpenMenuItems = true;
	pref.iAllowUndocking = false;
	pref.iTurnOffUITransparent = false;
	pref.iEnableDragDropEntityMode = 1;
	pref.iEnableAdvancedSky = 0;
	pref.iEnableAdvancedWater = 0;
	pref.iEnableAdvancedPostProcessing = 0;
	pref.iEnableAdvancedShadows = 0;
	pref.iEnableArcRelationshipLines = 1;
	pref.iEnableRelationPopupWindow = 1;// 0; we want you back ;)
	pref.iEnableAxisRotationShortcuts = 0;
	pref.iObjectEnableAdvanced = 0;
	pref.iEnableDragDropWidgetSelect = 0;
	pref.iEnableEditorOutlineSelection = 1;
	pref.iEnableSingleRightPanelAdvanced = 0;
	pref.iDragCameraMovement = 1;
	pref.iGameCreaterStore = 0;
	pref.iFullscreenPreviewAdvanced = 0;
	pref.iDisplayWelcomeScreen = 1;
	pref.iDisplayIntroScreen = 1;
	pref.iEnableAdvancedEntityList = 0;
	pref.fHighLightThickness = 1.0;
	pref.iTurnOffEditboxTooltip = false;
	pref.iImporterDome = 1;
	pref.iEnableAutoExposureInEditor = 0;
	pref.iSetColumnsEntityLib = 3;
	pref.iTerrainAdvanced = 0;
	pref.iStoryboardAdvanced = 0;
	pref.iTerrainDebugMode = 0;
	pref.iEnableAdvancedCharacterCreator = 0;
	pref.iDisableProjectAutoSave = 0;
	pref.iDisableLevelAutoSave = 0;
	pref.iEnableFpsMemMonitor = 0;
	pref.iEnableAutoFlattenSystem = 1;

	for (int i = 0; i < 10; i++)
		pref.iCheckboxFilters[i] = 1;

	//pref.iDisableObjectLibraryViewport = false;
	pref.iLastInStoryboard = false;
	strcpy(pref.cLastUsedStoryboardProject, "");
	pref.changelog_ftime = 0;

	pref.iHideKeyboardShortcuts = 0;
	pref.iEnableDeveloperObjectTools = 0;
	pref.iEnableLevelEditorOpenAndNew = 0;
	pref.iDisplayTerrainGeneratorWelcome = 1;
	pref.iTestGameGraphicsQuality = 2;
	pref.iEnableAutoFlattenSystem = 1;
	pref.iEnableAdvancedGrass = 0;

	pref.iEnableDragDropStopSelectFromInside = 0;

	#endif
}

void DrawRubberBand(float fx, float fy, float fx2, float fy2 )
{
	ImGuiViewport* mainviewport = ImGui::GetMainViewport();
	if (mainviewport)
	{
		ImDrawList* dl = ImGui::GetForegroundDrawList(mainviewport);
		if (dl)
		{
			ImRect bb;
			bb.Min.x = fx;
			bb.Min.y = fy;
			bb.Max.x = fx2;
			bb.Max.y = fy2;
			dl->AddRect(bb.Min,bb.Max, ImGui::GetColorU32(ImVec4(1.0,1.0,1.0,0.8)), 0.0f, ImDrawCornerFlags_None, 3.0f);
		}
	}

}

bool ImGui_GetScissorArea(float* pX1, float* pY1, float* pX2, float* pY2)
{
	#ifdef WICKEDENGINE
	extern bool m_bForceRender;
	if (m_bForceRender)
		return false;
	#endif

	if (1)
	{
		if (bDigAHoleToHWND)
		{
			*pX1 = rDigAHole.left;
			*pY1 = rDigAHole.top;
			*pX2 = rDigAHole.right;
			*pY2 = rDigAHole.bottom;
		}
		else
		{
			*pX1 = fImGuiScissorTopLeft.x;
			*pY1 = fImGuiScissorTopLeft.y;
			*pX2 = fImGuiScissorBottomRight.x;
			*pY2 = fImGuiScissorBottomRight.y;
		}
		return true;
	}
	else
		return false;
}