#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <math.h>
#define M_PI 3.14159265358979323846
using namespace std;

struct QAngles{
	float x;
	float y;
	float z;
};

struct OAngles {
	float yaw;
	float pitch;

	void operator=(const OAngles& other) {
		yaw = other.yaw;
		pitch = other.pitch;
	}
	bool operator<(const OAngles& other) {
		return (yaw < other.yaw && pitch < other.pitch);
	}
	bool operator>(const OAngles& other) {
		return (yaw > other.yaw && pitch > other.pitch);
	}
};

struct Entity
{
	char unk1[4];
	QAngles qangles;
	char unk2[0x30];
	OAngles oangles;
	char unk3[0x1DD];
	char nickname[15];
	char unk4[0x104];
	bool dead;

	OAngles Predict(Entity* localPlayer)
	{
		float abspos_x = qangles.x - localPlayer->qangles.x;
		float abspos_y = qangles.y - localPlayer->qangles.y;
		float abspos_z = qangles.z - localPlayer->qangles.z;

		float azimuth_xy = atan2f(abspos_y, abspos_x);
		float yaw = (float)(azimuth_xy * (180.0 / M_PI) + 90);

		if (abspos_y < 0) {
			abspos_y *= -1;
		}
		if (abspos_y < 5) {
			if (abspos_x < 0) {
				abspos_x *= -1;
			}
			abspos_y = abspos_x;
		}

		float azimuth_z = atan2f(abspos_z, abspos_y);
		float pitch = (float)(azimuth_z * (180.0 / M_PI));

		return OAngles{ yaw, pitch };
	}

	OAngles CalculateDistance(Entity* localPlayer)
	{
		OAngles angles = Predict(localPlayer);

		float yaw_dif = localPlayer->oangles.yaw - angles.yaw;
		float pitch_dif = localPlayer->oangles.pitch - angles.pitch;

		yaw_dif > 180 ? yaw_dif -= 360 : yaw_dif;
		yaw_dif < -180 ? yaw_dif += 360 : yaw_dif;

		return OAngles{ std::abs(yaw_dif), std::abs(pitch_dif) };
	}

	void Aim(Entity* localPlayer)
	{
		if (!dead)
			localPlayer->oangles = Predict(localPlayer);
	}
};

DWORD* entityList = (DWORD*)(0x50F4F8);
DWORD* entityNumb = (DWORD*)0x50F500;
DWORD* pLocalPlayer = (DWORD*)0x509B74;
Entity* localPlayer = NULL;

void Aimbot();
Entity* GetBestEnemy(float FOV);

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		AllocConsole();
		freopen("CONOUT$", "w", stdout);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Aimbot, NULL, 0, NULL);
	}

	return true;
}

void Aimbot()
{
	while (true)
	{
		localPlayer = (Entity*)(*pLocalPlayer);

		if (localPlayer != NULL && GetAsyncKeyState(VK_CONTROL))
		{
			Entity* enemy = GetBestEnemy(10.0f);
			if (enemy != NULL) {
				enemy->Aim(localPlayer);
			}
		}

		Sleep(1);
	}
}

Entity* GetBestEnemy(float FOV)
{
	if (localPlayer == NULL)
		return NULL;

	Entity* bestEnemy = NULL;

	for (int i = 0; i < *entityNumb; i++)
	{
		Entity* enemy = (Entity*)(*(DWORD*)(*entityList + (i * 4)));
		
		if (enemy != NULL && localPlayer != NULL)
		{
			if (bestEnemy == NULL)
				bestEnemy = enemy;
			else
			{
				OAngles enemyDistance = enemy->CalculateDistance(localPlayer);
				OAngles bestEnemyDistance = bestEnemy->CalculateDistance(localPlayer);

				if (enemyDistance.yaw < bestEnemyDistance.yaw)
					bestEnemy = enemy;
			}
		}
	}

	return bestEnemy;
}