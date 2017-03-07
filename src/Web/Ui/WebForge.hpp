#pragma once

#include "../../ThirdParty/rapidjson/document.h"
#include "../../Web/Bridge/WebRendererQuery.hpp"
#include "../../Forge/ForgeTool.hpp"


namespace Web
{
	namespace Ui
	{
		namespace Forge
		{
			using namespace ::Forge::Tool;

			void Init();
			void Show();
			void Hide();

			void SelectObject(ObjectTransform* transform);

			Anvil::Client::Rendering::Bridge::QueryError OnCommand(const rapidjson::Value &p_Args, std::string *p_Result);
		}
	}
}