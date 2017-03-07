#include "WebForge.hpp"
#include "ScreenLayer.hpp"
#include "../../Patches/Input.hpp"
#include "../../ThirdParty/rapidjson/writer.h"
#include "../../ThirdParty/rapidjson/stringbuffer.h"
#include "../../ThirdParty/rapidjson/document.h"
#include "../../Forge/ForgeTool.hpp"

using namespace Blam::Input;
using namespace Web::Ui;

namespace
{
	using namespace ::Forge::Tool;

	void OnGameInputUpdated()
	{
		if (GetKeyTicks(eKeyCodeInsert, eInputTypeUi) == 1)
		{
			ScreenLayer::Show("forge", "{}");

			auto objectUnderCrosshair = GetObjectIndexUnderCrosshair();
			if (objectUnderCrosshair == DatumIndex::Null)
				return;

			ObjectTransform transform;
			if (GetObjectTransform(objectUnderCrosshair, &transform))
			{		
				Web::Ui::Forge::SelectObject(&transform);
			}
		}
	}
}

namespace Web
{
	namespace Ui
	{
		namespace Forge
		{
			using namespace Anvil::Client::Rendering::Bridge;

			void Init()
			{
				Patches::Input::RegisterDefaultInputHandler(OnGameInputUpdated);
			}

			void Show()
			{
				ScreenLayer::Show("forge", "{}");
			}

			void Hide()
			{
				ScreenLayer::Hide("forge");
			}

			QueryError OnCommand(const rapidjson::Value &p_Args, std::string *p_Result)
			{
				auto typeMember = p_Args.FindMember("type");
				if (typeMember == p_Args.MemberEnd() || !typeMember->value.IsString())
				{
					*p_Result = "Bad query: A \"type\" argument is required and must be a string";
					return QueryError::QueryError_BadQuery;
				}

				auto commandType = typeMember->value.GetString();

				auto payloadMember = p_Args.FindMember("payload");
				if (payloadMember == p_Args.MemberEnd() || !payloadMember->value.IsObject())
				{
					*p_Result = "Bad query: A \"payload\" argument is required and must be an object";
					return QueryError::QueryError_BadQuery;
				}

#define ReadFloat(key) payloadMember->value.FindMember(key)->value.GetDouble()
#define ReadUint(key) payloadMember->value.FindMember(key)->value.GetUint()

				if (!_stricmp(commandType, "updateTransform"))
				{
					ObjectTransform object;
					object.ObjectIndex = ReadUint("ObjectIndex");
					object.PositionX = ReadFloat("PositionX");
					object.PositionY = ReadFloat("PositionY");
					object.PositionZ = ReadFloat("PositionZ");
					object.RotationX = ReadFloat("RotationX");
					object.RotationY = ReadFloat("RotationY");
					object.RotationZ = ReadFloat("RotationZ");

					SetObjectTransform(&object);
				}

				return QueryError::QueryError_Ok;
			}

			void SelectObject(ObjectTransform* transform)
			{
				rapidjson::StringBuffer jsonBuffer;
				rapidjson::Writer<rapidjson::StringBuffer> jsonWriter(jsonBuffer);

#define WriteFloat(key, value) jsonWriter.Key(key), jsonWriter.Double(value);
#define WriteUInt(key, value) jsonWriter.Key(key), jsonWriter.Uint(value);

				jsonWriter.StartObject();
				WriteUInt("ObjectIndex",transform->ObjectIndex);
				WriteFloat("PositionX", transform->PositionX);
				WriteFloat("PositionY", transform->PositionY);
				WriteFloat("PositionZ", transform->PositionZ);
				WriteFloat("RotationX", transform->RotationX);
				WriteFloat("RotationY", transform->RotationY);
				WriteFloat("RotationZ", transform->RotationZ);
				jsonWriter.EndObject();

				ScreenLayer::Notify("WebForge::SelectObject", jsonBuffer.GetString(), true);
			}
		}
	}
}

