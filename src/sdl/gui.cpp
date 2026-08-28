/*
	SDL OSD - Dear ImGui front-end implementation
*/

#include "gui.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
#include "../emu.h"
#include "../config.h"
#include "osd.h"
#include <string>
#include <cstdio>

static SDL_Window*   g_window   = NULL;
static SDL_Renderer* g_renderer = NULL;
static EMU*          g_emu      = NULL;

static const float STATUS_BAR_HEIGHT = 26.0f;

// ----------------------------------------------------------------------------
// helpers
// ----------------------------------------------------------------------------
static std::string run_osascript(const char* script)
{
	std::string cmd = "osascript -e '";
	cmd += script;
	cmd += "' 2>/dev/null";
	std::string out;
	FILE* p = popen(cmd.c_str(), "r");
	if(p != NULL) {
		char buf[1024];
		while(fgets(buf, sizeof(buf), p) != NULL) {
			out += buf;
		}
		pclose(p);
	}
	// strip trailing newline
	while(!out.empty() && (out.back() == '\n' || out.back() == '\r')) {
		out.pop_back();
	}
	return out;
}

static std::string choose_open_file(const char* prompt)
{
	std::string s = "POSIX path of (choose file with prompt \"";
	s += prompt;
	s += "\")";
	return run_osascript(s.c_str());
}

static std::string choose_save_file(const char* prompt)
{
	std::string s = "POSIX path of (choose file name with prompt \"";
	s += prompt;
	s += "\")";
	return run_osascript(s.c_str());
}

static void update_history(_TCHAR recent[][_MAX_PATH], const _TCHAR* path)
{
	int index = MAX_HISTORY - 1;
	for(int i = 0; i < MAX_HISTORY; i++) {
		if(_tcsicmp(recent[i], path) == 0) {
			index = i;
			break;
		}
	}
	for(int i = index; i > 0; i--) {
		my_tcscpy_s(recent[i], _MAX_PATH, recent[i - 1]);
	}
	my_tcscpy_s(recent[0], _MAX_PATH, path);
}

// last path component (file name) of a media path
static const char* file_base_name(const _TCHAR* p)
{
	if(p == NULL) {
		return NULL;
	}
	const _TCHAR* slash = strrchr(p, '/');
	return slash ? slash + 1 : p;
}

// disabled, informational menu row showing the loaded file name (full path on
// hover), or empty_label when nothing is loaded. Followed by a separator.
static void media_info_item(const _TCHAR* path, bool inserted, const char* empty_label)
{
	bool has = inserted && path != NULL && path[0] != _T('\0');
	ImGui::BeginDisabled();
	ImGui::MenuItem(has ? file_base_name(path) : empty_label);
	ImGui::EndDisabled();
	if(has && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip("%s", path);
	}
	ImGui::Separator();
}

// ----------------------------------------------------------------------------
// init / shutdown
// ----------------------------------------------------------------------------
bool gui_init(SDL_Window* window, SDL_Renderer* renderer, EMU* emu)
{
	g_window = window;
	g_renderer = renderer;
	g_emu = emu;
	if(window == NULL || renderer == NULL) {
		return false;
	}
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = NULL;	// don't write imgui.ini
	ImGui::StyleColorsDark();
	if(!ImGui_ImplSDL2_InitForSDLRenderer(window, renderer)) {
		return false;
	}
	if(!ImGui_ImplSDLRenderer2_Init(renderer)) {
		return false;
	}
	return true;
}

void gui_shutdown()
{
	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

void gui_process_event(const SDL_Event* ev)
{
	ImGui_ImplSDL2_ProcessEvent(ev);
}

bool gui_want_keyboard()
{
	return ImGui::GetIO().WantCaptureKeyboard;
}

// ----------------------------------------------------------------------------
// menu builders
// ----------------------------------------------------------------------------
static void menu_control()
{
	if(!ImGui::BeginMenu("Control")) {
		return;
	}
	EMU* emu = g_emu;
	if(ImGui::MenuItem("Reset")) emu->reset();
#ifdef USE_SPECIAL_RESET
	if(ImGui::MenuItem("NMI")) emu->special_reset();
#endif
	ImGui::Separator();
	const char* cpu_labels[5] = { "CPU x1", "CPU x2", "CPU x4", "CPU x8", "CPU x16" };
	for(int i = 0; i < 5; i++) {
		if(ImGui::MenuItem(cpu_labels[i], NULL, config.cpu_power == i)) {
			config.cpu_power = i;
			emu->update_config();
		}
	}
	if(ImGui::MenuItem("Full Speed", NULL, config.full_speed)) {
		config.full_speed = !config.full_speed;
	}
	if(ImGui::MenuItem("Drive VM in M1/R/W Cycle", NULL, config.drive_vm_in_opecode)) {
		config.drive_vm_in_opecode = !config.drive_vm_in_opecode;
	}
	ImGui::Separator();
#ifdef USE_AUTO_KEY
	if(ImGui::MenuItem("Paste", NULL, false, !emu->is_auto_key_running())) {
		char* clip = SDL_GetClipboardText();
		if(clip != NULL) {
			emu->stop_auto_key();
			emu->set_auto_key_list(clip, (int)strlen(clip));
			emu->start_auto_key();
			SDL_free(clip);
		}
	}
	if(ImGui::MenuItem("Stop", NULL, false, emu->is_auto_key_running())) {
		emu->stop_auto_key();
	}
	if(ImGui::MenuItem("Romaji to Kana", NULL, config.romaji_to_kana)) {
		if(!config.romaji_to_kana) {
			emu->set_auto_key_char(1);
			config.romaji_to_kana = true;
		} else {
			emu->set_auto_key_char(0);
			config.romaji_to_kana = false;
		}
	}
	ImGui::Separator();
#endif
#ifdef USE_STATE
	if(ImGui::BeginMenu("Save State")) {
		for(int i = 0; i < 10; i++) {
			char l[16]; snprintf(l, sizeof(l), "State %d", i);
			if(ImGui::MenuItem(l)) emu->save_state(emu->state_file_path(i));
		}
		ImGui::EndMenu();
	}
	if(ImGui::BeginMenu("Load State")) {
		for(int i = 0; i < 10; i++) {
			char l[16]; snprintf(l, sizeof(l), "State %d", i);
			bool exists = FILEIO::IsFileExisting(emu->state_file_path(i));
			if(ImGui::MenuItem(l, NULL, false, exists)) emu->load_state(emu->state_file_path(i));
		}
		ImGui::EndMenu();
	}
	ImGui::Separator();
#endif
#ifdef USE_DEBUGGER
	const char* dbg[3] = { "Debug Main CPU", "Debug Sub CPU", "Debug Keyboard CPU" };
	for(int i = 0; i < 3; i++) {
		bool en = (!emu->now_debugging) && emu->is_debugger_enabled(i);
		if(ImGui::MenuItem(dbg[i], NULL, false, en)) emu->open_debugger(i);
	}
	if(ImGui::MenuItem("Close Debugger", NULL, false, emu->now_debugging)) emu->close_debugger();
	ImGui::Separator();
#endif
	if(ImGui::MenuItem("Exit")) emu->get_osd()->power_off();
	ImGui::EndMenu();
}

#ifdef USE_FLOPPY_DISK
static void menu_fd(int drv)
{
	char title[8]; snprintf(title, sizeof(title), "FD%d", drv);
	if(!ImGui::BeginMenu(title)) {
		return;
	}
	EMU* emu = g_emu;
	media_info_item(emu->get_floppy_disk_path(drv), emu->is_floppy_disk_inserted(drv), "(no disk)");
	if(ImGui::MenuItem("Insert")) {
		std::string path = choose_open_file("Floppy Disk");
		if(!path.empty()) {
			update_history(config.recent_floppy_disk_path[drv], path.c_str());
			emu->open_floppy_disk(drv, path.c_str(), 0);
		}
	}
	bool inserted = emu->is_floppy_disk_inserted(drv);
	if(ImGui::MenuItem("Eject", NULL, false, inserted)) emu->close_floppy_disk(drv);
	if(ImGui::MenuItem("Insert Blank 2D Disk")) {
		std::string path = choose_save_file("Blank 2D Disk");
		if(!path.empty() && emu->create_blank_floppy_disk(path.c_str(), 0x00)) emu->open_floppy_disk(drv, path.c_str(), 0);
	}
	if(ImGui::MenuItem("Insert Blank 2DD Disk")) {
		std::string path = choose_save_file("Blank 2DD Disk");
		if(!path.empty() && emu->create_blank_floppy_disk(path.c_str(), 0x10)) emu->open_floppy_disk(drv, path.c_str(), 0);
	}
	if(ImGui::MenuItem("Insert Blank 2HD Disk")) {
		std::string path = choose_save_file("Blank 2HD Disk");
		if(!path.empty() && emu->create_blank_floppy_disk(path.c_str(), 0x20)) emu->open_floppy_disk(drv, path.c_str(), 0);
	}
	ImGui::Separator();
	if(ImGui::MenuItem("Write Protected", NULL, emu->is_floppy_disk_protected(drv), inserted)) {
		emu->is_floppy_disk_protected(drv, !emu->is_floppy_disk_protected(drv));
	}
	if(ImGui::MenuItem("Correct Timing", NULL, config.correct_disk_timing[drv])) {
		config.correct_disk_timing[drv] = !config.correct_disk_timing[drv];
	}
	if(ImGui::MenuItem("Ignore CRC Errors", NULL, config.ignore_disk_crc[drv])) {
		config.ignore_disk_crc[drv] = !config.ignore_disk_crc[drv];
	}
	// d88 multi-bank
	if(emu->d88_file[drv].bank_num > 1) {
		ImGui::Separator();
		if(ImGui::BeginMenu("Banks")) {
			for(int i = 0; i < emu->d88_file[drv].bank_num; i++) {
				if(ImGui::MenuItem(emu->d88_file[drv].disk_name[i], NULL, emu->d88_file[drv].cur_bank == i)) {
					if(emu->d88_file[drv].cur_bank != i) emu->open_floppy_disk(drv, emu->d88_file[drv].path, i);
				}
			}
			ImGui::EndMenu();
		}
	}
	ImGui::Separator();
	if(ImGui::BeginMenu("Recent")) {
		bool any = false;
		for(int i = 0; i < MAX_HISTORY; i++) {
			if(config.recent_floppy_disk_path[drv][i][0] == _T('\0')) continue;
			any = true;
			if(ImGui::MenuItem(config.recent_floppy_disk_path[drv][i])) {
				_TCHAR path[_MAX_PATH];
				my_tcscpy_s(path, _MAX_PATH, config.recent_floppy_disk_path[drv][i]);
				update_history(config.recent_floppy_disk_path[drv], path);
				emu->open_floppy_disk(drv, path, 0);
			}
		}
		if(!any) ImGui::MenuItem("None", NULL, false, false);
		ImGui::EndMenu();
	}
	ImGui::EndMenu();
}
#endif

#ifdef USE_TAPE
static void menu_cmt()
{
	if(!ImGui::BeginMenu("CMT")) {
		return;
	}
	EMU* emu = g_emu;
	media_info_item(emu->get_tape_path(0), emu->is_tape_inserted(0), "(no tape)");
	if(ImGui::MenuItem("Play")) {
		std::string path = choose_open_file("Tape (Play)");
		if(!path.empty()) {
			update_history(config.recent_tape_path[0], path.c_str());
			emu->play_tape(0, path.c_str());
		}
	}
	if(ImGui::MenuItem("Rec")) {
		std::string path = choose_save_file("Tape (Rec)");
		if(!path.empty()) {
			update_history(config.recent_tape_path[0], path.c_str());
			emu->rec_tape(0, path.c_str());
		}
	}
	bool inserted = emu->is_tape_inserted(0);
	if(ImGui::MenuItem("Eject", NULL, false, inserted)) emu->close_tape(0);
	ImGui::Separator();
	if(ImGui::MenuItem("Play Button", NULL, false, inserted)) emu->push_play(0);
	if(ImGui::MenuItem("Stop Button", NULL, false, inserted)) emu->push_stop(0);
	if(ImGui::MenuItem("Fast Forward", NULL, false, inserted)) emu->push_fast_forward(0);
	if(ImGui::MenuItem("Fast Rewind", NULL, false, inserted)) emu->push_fast_rewind(0);
	if(ImGui::MenuItem("APSS Forward", NULL, false, inserted)) emu->push_apss_forward(0);
	if(ImGui::MenuItem("APSS Rewind", NULL, false, inserted)) emu->push_apss_rewind(0);
	ImGui::Separator();
	if(ImGui::MenuItem("Waveform Shaper", NULL, config.wave_shaper[0])) {
		config.wave_shaper[0] = !config.wave_shaper[0];
	}
	ImGui::Separator();
	if(ImGui::BeginMenu("Recent")) {
		bool any = false;
		for(int i = 0; i < MAX_HISTORY; i++) {
			if(config.recent_tape_path[0][i][0] == _T('\0')) continue;
			any = true;
			if(ImGui::MenuItem(config.recent_tape_path[0][i])) {
				_TCHAR path[_MAX_PATH];
				my_tcscpy_s(path, _MAX_PATH, config.recent_tape_path[0][i]);
				update_history(config.recent_tape_path[0], path);
				emu->play_tape(0, path);
			}
		}
		if(!any) ImGui::MenuItem("None", NULL, false, false);
		ImGui::EndMenu();
	}
	ImGui::EndMenu();
}
#endif

#ifdef USE_HARD_DISK
static void menu_hd(int drv)
{
	char title[8]; snprintf(title, sizeof(title), "HD%d", drv);
	if(!ImGui::BeginMenu(title)) {
		return;
	}
	EMU* emu = g_emu;
	media_info_item(emu->get_hard_disk_path(drv), emu->is_hard_disk_inserted(drv), "(not mounted)");
	if(ImGui::MenuItem("Mount")) {
		std::string path = choose_open_file("Hard Disk");
		if(!path.empty()) {
			update_history(config.recent_hard_disk_path[drv], path.c_str());
			emu->open_hard_disk(drv, path.c_str());
		}
	}
	if(ImGui::MenuItem("Unmount", NULL, false, emu->is_hard_disk_inserted(drv))) emu->close_hard_disk(drv);
	if(ImGui::MenuItem("Mount Blank 20MB Disk")) {
		std::string path = choose_save_file("Blank 20MB HDD");
		if(!path.empty() && emu->create_blank_hard_disk(path.c_str(), 256, 33, 4, 615)) emu->open_hard_disk(drv, path.c_str());
	}
	ImGui::Separator();
	if(ImGui::BeginMenu("Recent")) {
		bool any = false;
		for(int i = 0; i < MAX_HISTORY; i++) {
			if(config.recent_hard_disk_path[drv][i][0] == _T('\0')) continue;
			any = true;
			if(ImGui::MenuItem(config.recent_hard_disk_path[drv][i])) {
				_TCHAR path[_MAX_PATH];
				my_tcscpy_s(path, _MAX_PATH, config.recent_hard_disk_path[drv][i]);
				update_history(config.recent_hard_disk_path[drv], path);
				emu->open_hard_disk(drv, path);
			}
		}
		if(!any) ImGui::MenuItem("None", NULL, false, false);
		ImGui::EndMenu();
	}
	ImGui::EndMenu();
}
#endif

static void radio_item(const char* label, int* field, int value, bool update)
{
	if(ImGui::MenuItem(label, NULL, *field == value)) {
		*field = value;
		if(update) g_emu->update_config();
	}
}

static void menu_device()
{
	if(!ImGui::BeginMenu("Device")) {
		return;
	}
	EMU* emu = g_emu; (void)emu;
#ifdef USE_DRIVE_TYPE
	if(ImGui::BeginMenu("Boot Device")) {
		radio_item("5/3-inch 2D",  &config.drive_type, 0, true);
		radio_item("5/3-inch 2DD", &config.drive_type, 1, true);
		radio_item("5/3-inch 2HD", &config.drive_type, 2, true);
		radio_item("8-inch 1S",    &config.drive_type, 6, true);
		radio_item("HARD DISK",    &config.drive_type, 7, true);
		ImGui::EndMenu();
	}
#endif
#ifdef USE_KEYBOARD_TYPE
	if(ImGui::BeginMenu("Keyboard")) {
		radio_item("Keyboard Mode A", &config.keyboard_type, 0, true);
		radio_item("Keyboard Mode B", &config.keyboard_type, 1, true);
		ImGui::EndMenu();
	}
#endif
	if(ImGui::BeginMenu("Sound")) {
		radio_item("PSG",        &config.sound_type, 0, false);
		radio_item("CZ-8BS1 x1", &config.sound_type, 1, false);
		radio_item("CZ-8BS1 x2", &config.sound_type, 2, false);
		ImGui::Separator();
		if(ImGui::MenuItem("Play FDD Noise", NULL, config.sound_noise_fdd)) { config.sound_noise_fdd = !config.sound_noise_fdd; g_emu->update_config(); }
		if(ImGui::MenuItem("Play CMT Noise", NULL, config.sound_noise_cmt)) { config.sound_noise_cmt = !config.sound_noise_cmt; g_emu->update_config(); }
		if(ImGui::MenuItem("Play CMT Signal", NULL, config.sound_tape_signal)) { config.sound_tape_signal = !config.sound_tape_signal; }
		if(ImGui::MenuItem("Play CMT Voice", NULL, config.sound_tape_voice)) { config.sound_tape_voice = !config.sound_tape_voice; }
		ImGui::EndMenu();
	}
	if(ImGui::BeginMenu("Display")) {
#ifdef _X1TURBO_FEATURE
		// monitor_type feeds the turbo dipswitch (port 0x1ff0); the original X1 has no 400-line mode
		radio_item("High Resolution", &config.monitor_type, 0, true);
		radio_item("Standard",        &config.monitor_type, 1, true);
#ifdef USE_SCANLINE
		ImGui::Separator();
#endif
#endif
#ifdef USE_SCANLINE
		if(ImGui::MenuItem("Scanline", NULL, config.scan_line)) { config.scan_line = !config.scan_line; g_emu->update_config(); }
#endif
		ImGui::EndMenu();
	}
	if(ImGui::BeginMenu("Printer")) {
		radio_item("Write Printer to File", &config.printer_type, 0, false);
		radio_item("MZ-1P17",               &config.printer_type, 1, false);
		ImGui::BeginDisabled(); radio_item("PC-PR201", &config.printer_type, 2, false); ImGui::EndDisabled();
		radio_item("JAST SOUND",            &config.printer_type, 3, false);
		radio_item("None",                  &config.printer_type, 4, false);
		ImGui::EndMenu();
	}
	if(ImGui::BeginMenu("Serial")) {
		ImGui::BeginDisabled(); radio_item("Physical Comm Port", &config.serial_type, 0, false); ImGui::EndDisabled();
		ImGui::BeginDisabled(); radio_item("Named Pipe", &config.serial_type, 1, false); ImGui::EndDisabled();
		radio_item("MIDI Device", &config.serial_type, 2, false);
		radio_item("None",        &config.serial_type, 3, false);
		ImGui::EndMenu();
	}
	ImGui::EndMenu();
}

static void menu_sound()
{
	if(!ImGui::BeginMenu("Sound")) {
		return;
	}
	const char* freqs[8] = { "2000Hz","4000Hz","8000Hz","11025Hz","22050Hz","44100Hz","62500Hz","96000Hz" };
	for(int i = 0; i < 8; i++) radio_item(freqs[i], &config.sound_frequency, i, true);
	ImGui::Separator();
	const char* lates[5] = { "50msec","100msec","200msec","300msec","400msec" };
	for(int i = 0; i < 5; i++) radio_item(lates[i], &config.sound_latency, i, true);
	ImGui::EndMenu();
}

static void menu_screen()
{
	if(!ImGui::BeginMenu("Screen")) {
		return;
	}
	if(ImGui::MenuItem("Capture Screen")) g_emu->capture_screen();
	ImGui::EndMenu();
}

// ----------------------------------------------------------------------------
// status bar (FDD / HDD activity LEDs + CMT message)
// ----------------------------------------------------------------------------
static void draw_led(ImDrawList* dl, ImVec2 p, float r, ImU32 col)
{
	dl->AddCircleFilled(ImVec2(p.x + r, p.y + r), r, col);
	dl->AddCircle(ImVec2(p.x + r, p.y + r), r, IM_COL32(0, 0, 0, 160));
}

static void status_bar()
{
	ImGuiViewport* vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(vp->Pos.x, vp->Pos.y + vp->Size.y - STATUS_BAR_HEIGHT));
	ImGui::SetNextWindowSize(ImVec2(vp->Size.x, STATUS_BAR_HEIGHT));
	ImGuiWindowFlags f = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollbar;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 4));
	if(ImGui::Begin("##statusbar", NULL, f)) {
		EMU* emu = g_emu;
		ImDrawList* dl = ImGui::GetWindowDrawList();
		const float led_r = 5.0f;
		const ImU32 col_off   = IM_COL32(70, 70, 70, 255);
		const ImU32 col_red   = IM_COL32(230, 40, 40, 255);
		const ImU32 col_green = IM_COL32(40, 220, 60, 255);

#ifdef USE_FLOPPY_DISK
		ImGui::TextUnformatted("FD:"); ImGui::SameLine();
		uint32_t fd_acc = emu->is_floppy_disk_accessed();
		uint32_t fd_col = emu->floppy_disk_indicator_color();
		for(int i = 0; i < USE_FLOPPY_DISK; i++) {
			ImU32 c = !((fd_acc >> i) & 1) ? col_off : !((fd_col >> i) & 1) ? col_red : col_green;
			ImVec2 p = ImGui::GetCursorScreenPos();
			draw_led(dl, ImVec2(p.x, p.y + 2), led_r, c);
			ImGui::Dummy(ImVec2(led_r * 2 + 3, led_r * 2));
			ImGui::SameLine();
		}
		ImGui::Dummy(ImVec2(10, 0)); ImGui::SameLine();
#endif
#ifdef USE_HARD_DISK
		ImGui::TextUnformatted("HD:"); ImGui::SameLine();
		uint32_t hd_acc = emu->is_hard_disk_accessed();
		for(int i = 0; i < USE_HARD_DISK; i++) {
			ImU32 c = ((hd_acc >> i) & 1) ? col_red : col_off;
			ImVec2 p = ImGui::GetCursorScreenPos();
			draw_led(dl, ImVec2(p.x, p.y + 2), led_r, c);
			ImGui::Dummy(ImVec2(led_r * 2 + 3, led_r * 2));
			ImGui::SameLine();
		}
		ImGui::Dummy(ImVec2(10, 0)); ImGui::SameLine();
#endif
#ifdef USE_TAPE
		{
			const _TCHAR* msg = emu->get_tape_message(0);
			ImGui::Text("CMT: %s", (msg != NULL) ? msg : "");
		}
#endif
	}
	ImGui::End();
	ImGui::PopStyleVar();
}

// ----------------------------------------------------------------------------
// frame
// ----------------------------------------------------------------------------
gui_viewport_t gui_build()
{
	ImGui_ImplSDLRenderer2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	float menu_h = 0.0f;
	if(ImGui::BeginMainMenuBar()) {
		menu_control();
#ifdef USE_FLOPPY_DISK
		for(int d = 0; d < USE_FLOPPY_DISK; d++) menu_fd(d);
#endif
#ifdef USE_TAPE
		menu_cmt();
#endif
#ifdef USE_HARD_DISK
		for(int d = 0; d < USE_HARD_DISK; d++) menu_hd(d);
#endif
		menu_device();
		menu_sound();
		menu_screen();
		menu_h = ImGui::GetWindowSize().y;
		ImGui::EndMainMenuBar();
	}

	status_bar();

	// emulator viewport = area between menu bar and status bar
	ImGuiViewport* vp = ImGui::GetMainViewport();
	gui_viewport_t r;
	r.x = (int)vp->Pos.x;
	r.y = (int)(vp->Pos.y + menu_h);
	r.w = (int)vp->Size.x;
	r.h = (int)(vp->Size.y - menu_h - STATUS_BAR_HEIGHT);
	if(r.h < 1) r.h = 1;
	return r;
}

void gui_render()
{
	ImGui::Render();
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), g_renderer);
}
