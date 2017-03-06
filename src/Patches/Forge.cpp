#include "Forge.hpp"

#include "../Patch.hpp"
#include "../Blam/BlamObjects.hpp"
#include "../Blam/BlamTypes.hpp"
#include "../Blam/BlamPlayers.hpp"
#include "../Blam/Tags/TagInstance.hpp"
#include "../Blam/Tags/TagReference.hpp"
#include "../Blam/BlamInput.hpp"
#include "../ElDorito.hpp"
#include <d3dx9.h>

using namespace Blam;
using namespace Blam::Objects;
using namespace Blam::Players;
using namespace Blam::Tags;
using namespace Blam::Input;

namespace
{
	bool shouldDelete = false;

	bool barriersEnabledValid = false;
	bool killBarriersEnabled = true;
	bool pushBarriersEnabled = true;

	void UpdateForgeInputHook();
	void UpdateBarriersEnabled();
	bool CheckKillTriggersHook(int a0, void *a1);
	bool ObjectSafeZoneHook(void *a0);
	void* PushBarriersGetStructureDesignHook(int index);

	inline const DataArray<ObjectHeader>* GetObjectHeaders()
	{
		auto objectHeadersPtr = ElDorito::GetMainTls(GameGlobals::ObjectHeader::TLSOffset);
		auto objectHeaders = objectHeadersPtr.Read<const DataArray<ObjectHeader>*>();
		return objectHeaders;
	}

	struct AABB
	{
		float X1, X2, Y1, Y2, Z1, Z2;
	};

	AABB* GetObjectBoundingBox(DatumIndex tagIndex)
	{
		auto objectMeta = Pointer(TagInstance(tagIndex.Index()).GetDefinition<void>());
		if (!objectMeta)
			return nullptr;

		const auto& hlmtRef = objectMeta(0x34).Read<TagReference>();

		auto hlmtMeta = Pointer(TagInstance(hlmtRef.TagIndex).GetDefinition<void>());
		if (!hlmtMeta)
			return nullptr;

		auto modeRef = hlmtMeta(0).Read<TagReference>();

		auto modeMeta = Pointer(TagInstance(modeRef.TagIndex).GetDefinition<void>());
		if (!modeMeta)
			return nullptr;

		auto compressionInfoBlock = modeMeta(0x74);
		if (compressionInfoBlock.Read<int32_t>() < 1)
			return nullptr;

		auto& boundingBox = compressionInfoBlock(4)[0](4).Read<AABB>();

		return &boundingBox;
	}

	inline DatumIndex SpawnTagAt(DatumIndex tagIndex, uint32_t playerIndex, D3DXVECTOR3* position, D3DXVECTOR3* right, D3DXVECTOR3* up)
	{
		typedef int(__cdecl *fn_sub_599F00)(int a1, void *a2, int a);
		auto sub_599F00 = reinterpret_cast<fn_sub_599F00>(0x00599F00);

		typedef void*(*fn_sub_583230)();
		auto sub_583230 = reinterpret_cast<fn_sub_583230>(0x00583230);

		typedef uint32_t(__thiscall *fn_sub_582110)(void* thisptr, int a2, int a3, int a4, void *a5, void *a6, void *a7, int a8, int a9, int a10, __int16 a11);
		auto sub_582110 = reinterpret_cast<fn_sub_582110>(0x00582110);

		void* factory = sub_583230();

		return sub_582110(factory, tagIndex.Index(), 0, -1, position, right, up, -1, -1, 0, 0);
	}

	void UpdateInput()
	{
		if (GetActionState(Blam::Input::eGameActionUiB)->Ticks == 1 || GetKeyTicks(eKeyCodeQ, eInputTypeUi) == 1)
		{
			Patches::Forge::CloneObjectUnderCrosshair(1.0f);
		}
	}
}

namespace Patches
{
	namespace Forge
	{
		void ApplyAll()
		{
			Hook(0x19D482, UpdateForgeInputHook, HookFlags::IsCall).Apply();
			Hook(0x771C7D, CheckKillTriggersHook, HookFlags::IsCall).Apply();
			Hook(0x7B4C32, CheckKillTriggersHook, HookFlags::IsCall).Apply();
			Hook(0x19EBA1, ObjectSafeZoneHook, HookFlags::IsCall).Apply();
			Hook(0x19FDBE, ObjectSafeZoneHook, HookFlags::IsCall).Apply();
			Hook(0x19FEEC, ObjectSafeZoneHook, HookFlags::IsCall).Apply();
			Hook(0x32663D, ObjectSafeZoneHook, HookFlags::IsCall).Apply();
			Hook(0x2749D1, PushBarriersGetStructureDesignHook, HookFlags::IsCall).Apply();
			Hook(0x274DBA, PushBarriersGetStructureDesignHook, HookFlags::IsCall).Apply();
			Hook(0x2750F8, PushBarriersGetStructureDesignHook, HookFlags::IsCall).Apply();
			Hook(0x275655, PushBarriersGetStructureDesignHook, HookFlags::IsCall).Apply();

			// enable teleporter volume editing compliments of zedd
			Patch::NopFill(Pointer::Base(0x6E4796), 0x66);
		}

		void Tick()
		{
			// Require a rescan for barrier disabler objects each tick
			barriersEnabledValid = false;
		}

		void SignalDelete()
		{
			shouldDelete = true;
		}

		uint32_t CloneObjectUnderCrosshair(float boundsScale)
		{
			auto playerIndex = GetLocalPlayer(0);
			if (playerIndex == DatumIndex::Null)
				return DatumIndex::Null;

			auto& players = GetPlayers();

			auto player = players.Get(playerIndex);
			if (!player)
				return DatumIndex::Null;

			auto objectHeaders = GetObjectHeaders();
			if (!objectHeaders)
				return DatumIndex::Null;

			auto gameEngineGlobals = ElDorito::GetMainTls(0x48)[0];
			if (!gameEngineGlobals)
				return DatumIndex::Null;

			auto objectIndexUnderCrosshair = gameEngineGlobals(0xe45c).Read<DatumIndex>();
			if (objectIndexUnderCrosshair == DatumIndex::Null)
				return DatumIndex::Null;

			auto objectHeaderUnderCrosshair = objectHeaders->Get(objectIndexUnderCrosshair);
			if (!objectHeaderUnderCrosshair)
				return DatumIndex::Null;

			auto objectUnderCrossHair = Pointer(objectHeaderUnderCrosshair->Data);
			if (!objectUnderCrossHair)
				return DatumIndex::Null;

			auto tagIndex = objectUnderCrossHair(0).Read<DatumIndex>();

			D3DXVECTOR3 position, right, up, forward, zeroVector(0, 0, 0), displacement;
			D3DXVECTOR4 worldIntersectNormal;
			D3DXMATRIX m0;

			auto modelIntersectNormal = gameEngineGlobals(0xE2DC).Read<D3DXVECTOR3>();
			position = objectUnderCrossHair(0x54).Read<D3DXVECTOR3>();
			right = objectUnderCrossHair(0x60).Read<D3DXVECTOR3>();
			up = objectUnderCrossHair(0x6C).Read<D3DXVECTOR3>();

			D3DXVec3Normalize(&right, &right);
			D3DXVec3Normalize(&up, &up);
			D3DXVec3Cross(&forward, &up, &right);
			D3DXVec3Normalize(&forward, &forward);

			D3DXMatrixLookAtLH(&m0, &zeroVector, &up, &forward);
			D3DXVec3Transform(&worldIntersectNormal, &modelIntersectNormal, &m0);

			auto bounds = GetObjectBoundingBox(tagIndex);
			if (!bounds)
				return false;

			if (abs(worldIntersectNormal.z) > abs(worldIntersectNormal.y) && abs(worldIntersectNormal.z) > abs(worldIntersectNormal.x))
			{
				if (worldIntersectNormal.z > 0)
					displacement = up * (bounds->Z2 - bounds->Z1);
				else
					displacement = up  * -(bounds->Z2 - bounds->Z1);
			}
			else if (abs(worldIntersectNormal.y) > abs(worldIntersectNormal.z) && abs(worldIntersectNormal.y) > abs(worldIntersectNormal.x))
			{
				if (worldIntersectNormal.y > 0)
					displacement = forward * (bounds->Y2 - bounds->Y1);
				else
					displacement = forward  * -(bounds->Y2 - bounds->Y1);
			}
			else if (abs(worldIntersectNormal.x) > abs(worldIntersectNormal.y) && abs(worldIntersectNormal.x) > abs(worldIntersectNormal.z))
			{
				if (worldIntersectNormal.x > 0)
					displacement = right * (bounds->X2 - bounds->X1);
				else
					displacement = right * -(bounds->X2 - bounds->X1);
			}

			D3DXVECTOR3 newPosition = position + displacement * boundsScale;

			return SpawnTagAt(tagIndex.Index(), playerIndex, &newPosition, &right, &up);
		}
	}
}

namespace
{
	__declspec(naked) void UpdateForgeInputHook()
	{
		__asm
		{
			call UpdateInput
			mov al, shouldDelete
			test al, al
			jnz del

			// Not deleting - just call the original function
			push esi
			mov eax, 0x59F0E0
			call eax
			retn 4

		del:
		mov shouldDelete, 0

			// Simulate a Y button press
			mov eax, 0x244D1F0              // Controller data
			mov byte ptr[eax + 0x9E], 1    // Ticks = 1
			and byte ptr[eax + 0x9F], 0xFE // Clear the "handled" flag

			// Call the original function
			push esi
			mov eax, 0x59F0E0
			call eax

			// Make sure nothing else gets the fake press
			mov eax, 0x244D1F0          // Controller data
			or byte ptr[eax + 0x9F], 1 // Set the "handled" flag
			retn 4
		}
	}

	void UpdateBarriersEnabled()
	{
		if (barriersEnabledValid)
			return; // Don't scan multiple times per tick

		// Scan the object table to check if the barrier disablers are spawned
		auto objectHeaders = GetObjectHeaders();
		if (!objectHeaders)
			return;
		killBarriersEnabled = true;
		pushBarriersEnabled = true;
		for (auto &&header : *objectHeaders)
		{
			// The objects are identified by tag index.
			// scen 0x5728 disables kill barriers
			// scen 0x5729 disables push barriers
			if (header.Type != eObjectTypeScenery)
				continue;
			auto tagIndex = header.GetTagIndex().Index();
			if (tagIndex == 0x5728)
				killBarriersEnabled = false;
			else if (tagIndex == 0x5729)
				pushBarriersEnabled = false;
			if (!killBarriersEnabled && !pushBarriersEnabled)
				break;
		}
		barriersEnabledValid = true;
	}

	bool CheckKillTriggersHook(int a0, void *a1)
	{
		UpdateBarriersEnabled();
		if (!killBarriersEnabled)
			return false;

		typedef bool(*CheckKillTriggersPtr)(int a0, void *a1);
		auto CheckKillTriggers = reinterpret_cast<CheckKillTriggersPtr>(0x68C410);
		return CheckKillTriggers(a0, a1);
	}

	bool ObjectSafeZoneHook(void *a0)
	{
		UpdateBarriersEnabled();
		if (!killBarriersEnabled)
			return true;

		typedef bool(*CheckSafeZonesPtr)(void *a0);
		auto CheckSafeZones = reinterpret_cast<CheckSafeZonesPtr>(0x4EB130);
		return CheckSafeZones(a0);
	}

	void* PushBarriersGetStructureDesignHook(int index)
	{
		UpdateBarriersEnabled();
		if (!pushBarriersEnabled)
			return nullptr; // Return a null sddt if push barriers are disabled

		typedef void*(*GetStructureDesignPtr)(int index);
		auto GetStructureDesign = reinterpret_cast<GetStructureDesignPtr>(0x4E97D0);
		return GetStructureDesign(index);
	}
}