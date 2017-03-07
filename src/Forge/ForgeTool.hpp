#pragma once

#include "../Blam/BlamTypes.hpp"
#include "../Blam/BlamData.hpp"
#include "../Blam/BlamObjects.hpp"
#include "../Blam/Tags/Tag.hpp"

namespace Forge
{
	namespace Tool
	{
		using namespace Blam;
		using namespace Blam::Objects;
		using namespace Blam::Tags;

		struct ObjectTransform {
			uint32_t ObjectIndex;
			float PositionX;
			float PositionY;
			float PositionZ;
			float RotationX;
			float RotationY;
			float RotationZ;
		};

		void Update();

		DatumIndex CloneObjectUnderCrosshair(float boundScale);
		DatumIndex GetObjectIndexUnderCrosshair();

		ObjectTransform* GetObjectTransform(DatumIndex objectIndex, ObjectTransform* outTransform);
		void SetObjectTransform(const ObjectTransform* object);

		void OnMoveObject(DatumIndex objectIndex);
	}
}