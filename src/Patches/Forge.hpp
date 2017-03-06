#pragma once

#include "../Blam/BlamData.hpp"

namespace Patches
{
	namespace Forge
	{
		void ApplyAll();
		void Tick();

		// Signal to delete the current item, if any
		void SignalDelete();

		uint32_t CloneObjectUnderCrosshair(float boundsScale);
	}
}