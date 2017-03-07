#include "ForgeTool.hpp"
#include "../ElDorito.hpp"
#include "../Blam/BlamObjects.hpp"
#include "../Blam/BlamData.hpp"
#include "../Blam/BlamInput.hpp"
#include "../Blam/BlamObjects.hpp"
#include "../Blam/BlamTypes.hpp"
#include "../Blam/BlamPlayers.hpp"
#include "../Blam/Tags/TagInstance.hpp"
#include "../Blam/Tags/TagReference.hpp"
#include "../Blam/BlamInput.hpp"
#include "../Web/Ui/WebForge.hpp"
#include <d3dx9.h>

using namespace Blam;
using namespace Blam::Objects;
using namespace Blam::Players;
using namespace Blam::Tags;
using namespace Blam::Input;

namespace
{
	// TODO: move this into Blam::Math

#ifndef M_PI
#define M_PI 3.14159265359
#endif

#define RAD2DEG(x)((x) * (180.0f / M_PI))
#define DEG2RAD(x)((x) / 180.0f * M_PI)

	inline D3DXVECTOR3 QuaternionVec3Transform(const D3DXVECTOR3& vector, const D3DXQUATERNION& q)
	{
		float x = q.x + q.x;
		float y = q.y + q.y;
		float z = q.z + q.z;

		float wx = q.w * x, wy = q.w * y, wz = q.w * z;
		float xx = q.x * x, xy = q.x * y, xz = q.x * z;
		float yy = q.y * y, yz = q.y * z, zz = q.z * z;

		return D3DXVECTOR3(
			((vector.x * ((1.0f - yy) - zz)) + (vector.y * (xy - wz))) + (vector.z * (xz + wy)),
			((vector.x * (xy + wz)) + (vector.y * ((1.0f - xx) - zz))) + (vector.z * (yz - wx)),
			((vector.x * (xz - wy)) + (vector.y * (yz + wx))) + (vector.z * ((1.0f - xx) - yy)));
	}

	inline void MatrixToEulerAngles(D3DXMATRIX* m, D3DXVECTOR3* outEulerAngles)
	{
		static const float PI_OVER_2 = 1.57079632679f;

		float h, p, b;
		float sp = -m->_32;
		if (sp <= -1.0f)
		{
			p = -PI_OVER_2;
		}
		else if (sp >= 1.0f)
		{
			p = PI_OVER_2;
		}
		else
		{
			p = asin(sp);
		}
		if (fabs(sp) > 0.9999f)
		{
			b = 0.0f;
			h = atan2(-m->_13, m->_11);
		}
		else
		{
			h = atan2(m->_31, m->_33);
			b = atan2(m->_12, m->_22);
		}

		outEulerAngles->x = h;
		outEulerAngles->y = p;
		outEulerAngles->z = b;
	}

	struct AABB
	{
		float X1, X2, Y1, Y2, Z1, Z2;
	};
}

namespace
{
	using namespace Forge::Tool;

	volatile bool dirty = false;
	ObjectTransform objectTransform;

	Pointer GetObjectPtr(DatumIndex objectIndex)
	{
		auto objectHeadersPtr = ElDorito::GetMainTls(GameGlobals::ObjectHeader::TLSOffset);
		auto objectHeaders = objectHeadersPtr.Read<const DataArray<ObjectHeader>*>();
		if (!objectHeaders)
			return nullptr;

		auto objectHeader = objectHeaders->Get(objectIndex);
		if (!objectHeader)
			return nullptr;

		return Pointer(objectHeader->Data);
	}

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

	void UpdateObjectTransform()
	{
		typedef int(__cdecl *SetObjectTransformPtr)(int a1, uint32_t objectIndex, float* position, float* rightVector, float* upVector);
		auto SetObjectTransform = reinterpret_cast<SetObjectTransformPtr>(0x0059E340);

		auto objectInfo = &objectTransform;

		auto objectHeadersPtr = ElDorito::GetMainTls(GameGlobals::ObjectHeader::TLSOffset);
		auto objectHeaders = objectHeadersPtr.Read<const DataArray<ObjectHeader>*>();
		if (!objectHeaders)
			return;

		auto gameEngineGlobals = ElDorito::GetMainTls(0x48)[0];
		if (!gameEngineGlobals)
			return;

		auto objectHeader = objectHeaders->Get(objectInfo->ObjectIndex);
		if (!objectHeader)
			return;

		auto objectPtr = Pointer(objectHeader->Data);
		if (!objectPtr)
			return;

		D3DXQUATERNION q;
		D3DXQuaternionRotationYawPitchRoll(&q, DEG2RAD(objectInfo->RotationX), DEG2RAD(objectInfo->RotationY), DEG2RAD(objectInfo->RotationZ));

		D3DXVECTOR3 right(1, 0, 0), up(0, 0, 1), position(objectInfo->PositionX, objectInfo->PositionY, objectInfo->PositionZ);
		right = QuaternionVec3Transform(right, q);
		up = QuaternionVec3Transform(up, q);

		D3DXVec3Normalize(&right, &right);
		D3DXVec3Normalize(&up, &up);

		SetObjectTransform(1, (int)objectInfo->ObjectIndex, (float*)&position, (float*)&right, (float*)&up);
	}
};

namespace Forge
{
	namespace Tool
	{
		inline void SelectObject(DatumIndex objectIndex)
		{
			ObjectTransform transform;
			if (GetObjectTransform(objectIndex, &transform))
				Web::Ui::Forge::SelectObject(&transform);
		}


		void Update()
		{
			if (GetActionState(eGameActionUiB)->Ticks == 1 || GetKeyTicks(eKeyCodeQ, eInputTypeUi) == 1)
			{
				auto newObjectIndex = CloneObjectUnderCrosshair(1.0f);
				SelectObject(newObjectIndex);
			}

			if (dirty)
			{
				UpdateObjectTransform();
				dirty = false;
			}
		}

		void SetObjectTransform(const ObjectTransform* object)
		{
			objectTransform = *object;
			dirty = true;
		}

		ObjectTransform* GetObjectTransform(DatumIndex objectIndex, ObjectTransform* outTransform)
		{
			auto objectPtr = GetObjectPtr(objectIndex);
			if (!objectPtr)
				return nullptr;

			auto tagIndex = objectPtr(0).Read<DatumIndex>();

			D3DXVECTOR3 position, right, up, forward, zeroVector(0, 0, 0);

			position = objectPtr(0x54).Read<D3DXVECTOR3>();
			right = objectPtr(0x60).Read<D3DXVECTOR3>();
			up = objectPtr(0x6C).Read<D3DXVECTOR3>();

			D3DXVec3Normalize(&right, &right);
			D3DXVec3Normalize(&up, &up);
			D3DXVec3Cross(&forward, &up, &right);
			D3DXVec3Normalize(&forward, &forward);

			D3DXMATRIX m0
			{
				right.x, right.y, right.z, 1.0f,
				forward.x, forward.y, forward.z, 1.0f,
				up.x, up.y, up.z, 1.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			};

			D3DXVECTOR3 eulerAngles;
			MatrixToEulerAngles(&m0, &eulerAngles);

			outTransform->ObjectIndex = objectIndex;
			outTransform->PositionX = position.x;
			outTransform->PositionY = position.y;
			outTransform->PositionZ = position.z;
			outTransform->RotationX = RAD2DEG(eulerAngles.x);
			outTransform->RotationY = RAD2DEG(eulerAngles.y);
			outTransform->RotationZ = RAD2DEG(eulerAngles.z);

			return outTransform;
		}

		inline DatumIndex GetObjectIndexUnderCrosshair()
		{
			auto gameEngineGlobals = ElDorito::GetMainTls(0x48)[0];
			if (!gameEngineGlobals)
				return DatumIndex::Null;

			return gameEngineGlobals(0xe45c).Read<uint32_t>();
		}

		DatumIndex CloneObjectUnderCrosshair(float boundScale)
		{
			auto playerIndex = GetLocalPlayer(0);
			if (playerIndex == DatumIndex::Null)
				return DatumIndex::Null;

			auto& players = GetPlayers();

			auto player = players.Get(playerIndex);
			if (!player)
				return DatumIndex::Null;

			auto objectHeadersPtr = ElDorito::GetMainTls(GameGlobals::ObjectHeader::TLSOffset);
			auto objectHeaders = objectHeadersPtr.Read<const DataArray<ObjectHeader>*>();
			if (!objectHeaders)
				return DatumIndex::Null;

			auto objectIndexUnderCrosshair = GetObjectIndexUnderCrosshair();
			if (objectIndexUnderCrosshair == DatumIndex::Null)
				return DatumIndex::Null;

			auto objectUnderCrosshair = GetObjectPtr(objectIndexUnderCrosshair);
			if (!objectUnderCrosshair)
				return DatumIndex::Null;

			auto tagIndex = objectUnderCrosshair(0).Read<DatumIndex>();

			D3DXVECTOR3 position, right, up, forward, zeroVector(0, 0, 0), displacement;
			D3DXVECTOR4 worldIntersectNormal;
			D3DXMATRIX m0;

			auto gameEngineGlobals = ElDorito::GetMainTls(0x48)[0];
			if (!gameEngineGlobals)
				return DatumIndex::Null;

			auto modelIntersectNormal = gameEngineGlobals(0xE2DC).Read<D3DXVECTOR3>();

			position = objectUnderCrosshair(0x54).Read<D3DXVECTOR3>();
			right = objectUnderCrosshair(0x60).Read<D3DXVECTOR3>();
			up = objectUnderCrosshair(0x6C).Read<D3DXVECTOR3>();

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

			D3DXVECTOR3 newPosition = position + displacement * boundScale;

			return SpawnTagAt(tagIndex.Index(), playerIndex, &newPosition, &right, &up);
		}

		void OnMoveObject(DatumIndex objectIndex)
		{
			static long lastUpdateTime = GetTickCount();

			if (GetTickCount() - lastUpdateTime < 100)
				return;

			SelectObject(objectIndex);

			lastUpdateTime = GetTickCount();
		}
	}
}