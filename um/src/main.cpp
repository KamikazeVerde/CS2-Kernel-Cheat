#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
//#include "modules/modules.h"
#include "main.h"
#include "offsets.h"
#include "aimbot/aimbot.h"
#include <termcolor.h>
#include "esp/esp.h"
#include <thread>

void bunnyhop() {
	while (true) {
		if (GetAsyncKeyState(VK_END))
			break;

		const auto local_player_pawn = driver::read_memory<std::uintptr_t>(driverfile, client + offsets::client_dll::dwLocalPlayerPawn);

		if (local_player_pawn == 0)
			continue;
		const auto flags = driver::read_memory<std::uint32_t>(driverfile, local_player_pawn + schemas::client_dll::C_BaseEntity::m_fFlags);
		const bool in_air = flags & (1 << 0);
		const bool space_pressed = GetAsyncKeyState(VK_SPACE);
		const auto force_jump = driver::read_memory<DWORD>(driverfile, local_player_pawn + schemas::client_dll::C_BaseEntity::m_fFlags);



		if (space_pressed && in_air) {
			driver::write_memory(driverfile, client + buttons::jump, 65537);
		}
		else if (space_pressed && !in_air) {
			driver::write_memory(driverfile, client + buttons::jump, 256);
		}
		else if (!space_pressed && force_jump == 65537) {
			driver::write_memory(driverfile, client + buttons::jump, 256);
		}
		/*
		else if (GetAsyncKeyState(VK_LMENU)) {
			break;
		}
		*/
		break;


	}
}
void triggerbot() {

	static uintptr_t local_player_pawn = driver::read_memory<std::uintptr_t>(driverfile, client + offsets::client_dll::dwLocalPlayerPawn);
	BYTE team = driver::read_memory<DWORD>(driverfile, local_player_pawn + schemas::client_dll::C_BaseEntity::m_iTeamNum);
	int entityIndex = driver::read_memory<DWORD>(driverfile, local_player_pawn + schemas::client_dll::C_CSPlayerPawnBase::m_iIDEntIndex);
	int attack = driver::read_memory<DWORD>(driverfile, client + buttons::attack);
	const bool trigger_key = GetAsyncKeyState(VK_LMENU);
	static uintptr_t entity_list = driver::read_memory<std::uintptr_t>(driverfile, client + offsets::client_dll::dwEntityList);
	
	uintptr_t listEntry = driver::read_memory<std::uintptr_t>(driverfile, entity_list + 0x8 * (entityIndex >> 9) + 0x10);
	uintptr_t entity = driver::read_memory<std::uintptr_t>(driverfile, listEntry + 120 * (entityIndex & 0x1ff));
	int crouch = driver::read_memory<DWORD>(driverfile, client + buttons::duck);


	if (trigger_key && entityIndex > 0) {
		Sleep(3);
		driver::write_memory(driverfile, client + buttons::attack, 65537);
		Sleep(3);
		driver::write_memory(driverfile, client + buttons::duck, 65537);
		Sleep(3);
		//std::cout << "[+] TARGET\n";
		//driver::write_memory(driver, client + buttons::forward, 65537);
	}
	else if (!trigger_key && entityIndex > 0) {
		driver::write_memory(driverfile, client + buttons::attack, 256);
		driver::write_memory(driverfile, client + buttons::duck, 256);
		
	}
	else if (trigger_key && entityIndex == -1) {
		driver::write_memory(driverfile, client + buttons::attack, 256);
		driver::write_memory(driverfile, client + buttons::duck, 256);
		
	}
	else if (trigger_key && !entityIndex > 0) {
		driver::write_memory(driverfile, client + buttons::attack, 256);
		driver::write_memory(driverfile, client + buttons::duck, 256);
		
	}
	/*
	else if (!trigger_key && attack == 65537) {
		driver::write_memory(driverfile, client + buttons::attack, 256);
	}
	*/
	else if (GetAsyncKeyState(VK_SPACE)) {
		
		bunnyhop();
	}
	
}

void esp::loop() {

	uintptr_t entity_list = driver::read_memory<std::uintptr_t>(driverfile, client + offsets::client_dll::dwEntityList);
	uintptr_t local_player_pawn = driver::read_memory<std::uintptr_t>(driverfile, client + offsets::client_dll::dwLocalPlayerPawn);
	BYTE team = driver::read_memory<DWORD>(driverfile, local_player_pawn + schemas::client_dll::C_BaseEntity::m_iTeamNum);

	while (true) {
		std::vector<uintptr_t> buffer = {};

		for (int i = 1; i < 32; i++)
		{
			uintptr_t listEntry = driver::read_memory<std::uintptr_t>(driverfile, entity_list + ((8 * (i & 0x7ff) >> 9) + 16));
			if (!listEntry) continue;

			uintptr_t entityController = driver::read_memory<std::uintptr_t>(driverfile, listEntry + 120 * (i & 0x1ff));
			if (!entityController) continue;

			uintptr_t entityControllerPawn = driver::read_memory<std::uintptr_t>(driverfile, entityController + schemas::client_dll::CCSPlayerController::m_hPlayerPawn);
			if (!entityControllerPawn) continue;
			
			uintptr_t entity = driver::read_memory<std::uintptr_t>(driverfile, listEntry + 120 * (entityControllerPawn & 0x1ff));
			if (!entity) continue;

			if (entity) buffer.emplace_back(entity);
		}

		entities = buffer;
		Sleep(10);
	}

}
void esp::frame() {
	renderer::pDevice->Clear(0, 0, D3DCLEAR_TARGET, NULL, 1.f, 0);
	renderer::pDevice->BeginScene();

	render();

	renderer::pDevice->EndScene();
	renderer::pDevice->Present(0, 0, 0, 0);
}

void esp::render() {


	vm = driver::read_memory<viewMatrix>(driverfile, client + offsets::client_dll::dwViewMatrix);
	

	for (uintptr_t entity : entities)
	{
		vec3 absOrigin = driver::read_memory<vec3>(driverfile, entity + schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin);
		vec3 eyePos = driver::read_memory<vec3>(driverfile, entity + schemas::client_dll::C_BaseModelEntity::m_vecViewOffset);

		vec2 head, feet;

		if (w2s(absOrigin, head, vm.m))
		{
			if (w2s(eyePos, feet, vm.m))
			{
				float width = (head.y - feet.y);
				feet.x += width;
				feet.y -= width;

				renderer::draw::box(D3DXVECTOR2{ head.x, head.y }, D3DXVECTOR2{ feet.x, feet.y }, D3DCOLOR_XRGB(255, 255, 255));
			}
		}
	}      
}

bool esp::w2s(const vec3& world, vec2& screen, float m[16]) {
	vec4 clipCoords;
	clipCoords.x = world.x * m[0] + world.y * m[1] + world.z * m[2] + m[3];
	clipCoords.y = world.x * m[4] + world.y * m[5] + world.z * m[6] + m[7];
	clipCoords.z = world.x * m[8] + world.y * m[9] + world.z * m[10] + m[11];
	clipCoords.w = world.x * m[12] + world.y * m[13] + world.z * m[14] + m[15];

	if (clipCoords.w < 0.1f) return false;

	vec3 ndc;

	ndc.x = clipCoords.x / clipCoords.w;
	ndc.y = clipCoords.y / clipCoords.w;

	screen.x = (WINDOW_H / 2 * ndc.x) + (ndc.x + WINDOW_H / 2);
	screen.y = -(WINDOW_H / 2 * ndc.y) + (ndc.y + WINDOW_H / 2);

	return true;
}

/*
void esp::render() {


	vm = driver::read_memory<viewMatrix>(driverfile, client + offsets::client_dll::dwViewMatrix);

	for (uintptr_t entity : entities)
	{
		vec3 absOrigin = driver::read_memory<vec3>(driverfile, entity + schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin);
		vec3 eyePos = driver::read_memory<vec3>(driverfile, entity + schemas::client_dll::C_BaseModelEntity::m_vecViewOffset);

		vec2 head, feet;

		if (w2s(absOrigin, head, vm.m))
		{
			if (w2s(eyePos, feet, vm.m))
			{
				float width = (head.y - feet.y);
				feet.x += width;
				feet.y -= width;

				renderer::draw::box(D3DXVECTOR2{ head.x, head.y }, D3DXVECTOR2{ feet.x, feet.y }, D3DCOLOR_XRGB(255, 255, 255));
			}
		}
	}
}
*/
float aimbot::distance(vec3 p1, vec3 p2) 
{
	return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2) + pow(p1.z - p2.z, 2));
}
void aimbot::frame() {
	uintptr_t entity_list = driver::read_memory<std::uintptr_t>(driverfile, client + offsets::client_dll::dwEntityList);
	if (!entity_list)
		return;
	uintptr_t local_player_pawn = driver::read_memory<std::uintptr_t>(driverfile, client + offsets::client_dll::dwLocalPlayerPawn);
	if (!local_player_pawn)
		return;
	BYTE team = driver::read_memory<DWORD>(driverfile, local_player_pawn + schemas::client_dll::C_BaseEntity::m_iTeamNum);

	vec3 localEyePos = driver::read_memory<vec3>(driverfile, local_player_pawn + schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin) +
		driver::read_memory<vec3>(driverfile, local_player_pawn + schemas::client_dll::C_BaseModelEntity::m_vecViewOffset);

	float closest_distance = -1;
	vec3 enemyPos;

	for (int i = 0; i < 32; i++)
	{

		uintptr_t listEntry = driver::read_memory<std::uintptr_t>(driverfile, entity_list + ((8 * (i & 0x7ff) >> 9) + 16));
		if (!listEntry) continue;

		uintptr_t entityController = driver::read_memory<std::uintptr_t>(driverfile, listEntry + 120 * (i & 0x1ff));
		if (!entityController) continue;

		uintptr_t entityControllerPawn = driver::read_memory<std::uintptr_t>(driverfile, entityController + schemas::client_dll::CCSPlayerController::m_hPlayerPawn);
		if (!entityControllerPawn) continue;

		uintptr_t entityPawn = driver::read_memory<std::uintptr_t>(driverfile, listEntry + 120 * (entityControllerPawn & 0x1ff));
		if (!entityPawn) continue;

		if (team == driver::read_memory<DWORD>(driverfile, entityPawn + schemas::client_dll::C_BaseEntity::m_iTeamNum))
			continue;

		if (driver::read_memory<std::uint32_t>(driverfile, entityPawn + schemas::client_dll::C_BaseEntity::m_iHealth) <= 0)
			continue;
		
		vec3 entityEyePos = driver::read_memory<vec3>(driverfile, entityPawn + schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin) +
			driver::read_memory<vec3>(driverfile, entityPawn + schemas::client_dll::C_BaseModelEntity::m_vecViewOffset);

		float current_distance = distance(localEyePos, entityEyePos);

		if (closest_distance < 0 || current_distance < closest_distance)
		{
			closest_distance = current_distance;
			enemyPos = entityEyePos;
		}
	}

	vec3 relativeAngle = (enemyPos - localEyePos).RelativeAngle();
	//mouse_event(MOUSEEVENTF_MOVE, (vec3)(enemyPos), (uint32_t), NULL, NULL);
	driver::write_memory<vec3>(driverfile, client + offsets::client_dll::dwViewAngles, relativeAngle);
	
}
void fovv() {
	
	

	uintptr_t local_player_pawn = driver::read_memory<std::uintptr_t>(driverfile, client + offsets::client_dll::dwLocalPlayerPawn);
	intptr_t cameraServices = driver::read_memory<std::uintptr_t>(driverfile, local_player_pawn + schemas::client_dll::C_BasePlayerPawn::m_pCameraServices);

	uintptr_t currentFov = driver::read_memory<std::uintptr_t>(driverfile, cameraServices + schemas::client_dll::CCSPlayerBase_CameraServices::m_iFOV);
	
	if (currentFov != fov::desideredFov) {
		driver::write_memory<std::uintptr_t>(driverfile, cameraServices + schemas::client_dll::CCSPlayerBase_CameraServices::m_iFOV, fov::desideredFov);
	}
	

}
int main(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow) {


	std::cout << termcolor::yellow << "[!] CS2 Cheat by Creeper215\n";

	if (pid == 0) {
		std::cout << termcolor::red << "\n[-] Process not found!\n";
		std::cin.get();
		return 1;
	}


	if (driverfile == INVALID_HANDLE_VALUE) {
		std::cout << termcolor::red << "\n[-] Handle failed to create!\n";

		return 1;
	}
	if (driver::attach_to_process(driverfile, pid) == true) {
		std::cout << termcolor::green << "\n[+] Attached successuflly to the process!" << termcolor::cyan << " PID " << pid << "\n";
	}
	
	if (const std::uintptr_t client = get_module_base(pid, L"client.dll"); client != 0) {
		std::cout << termcolor::green <<"[+] client.dll found!\n";
	}

	/*
	HWND hwnd = window::InitWindow(hInstance);
	if (!hwnd) return -1;


	if (!renderer::init(hwnd))
	{
		renderer::destroy();
		return -1;
	}
	std::thread read(esp::loop);

	while (!GetAsyncKeyState(VK_F9))
	{
		esp::frame();
	}

	renderer::destroy();
	UnregisterClass(L"overlay", hInstance);
	*/
	

	
	
	//triggerbot();
	//bunnyhop();
	
	while (true) {
		
		
	
		
		uintptr_t local_player_pawn = driver::read_memory<std::uintptr_t>(driverfile, client + offsets::client_dll::dwLocalPlayerPawn);
		intptr_t cameraServices = driver::read_memory<std::uintptr_t>(driverfile, local_player_pawn + schemas::client_dll::C_BasePlayerPawn::m_pCameraServices);
		uintptr_t currentFov = driver::read_memory<std::uintptr_t>(driverfile, cameraServices + schemas::client_dll::CCSPlayerBase_CameraServices::m_iFOV);
		bool isScoped = driver::read_memory<bool>(driverfile, local_player_pawn + schemas::client_dll::C_CSPlayerPawn::m_bIsScoped);

		
		if (!isScoped && currentFov != 110) {
			driver::write_memory<std::uintptr_t>(driverfile, cameraServices + schemas::client_dll::CCSPlayerBase_CameraServices::m_iFOV, 120);
		}
		


		triggerbot();
		if (GetAsyncKeyState(VK_LSHIFT))
			aimbot::frame();

	}

	
	
	/*
	std::thread red(esp::loop);
	while (true) {
		esp::frame();
	}
	*/
	CloseHandle(driverfile);

	

	std::cin.get();


	return 0;
	

}

