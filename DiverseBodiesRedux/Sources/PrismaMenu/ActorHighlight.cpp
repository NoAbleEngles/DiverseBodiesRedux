#include "PCH.h"
#include "PrismaMenu/ActorHighlight.h"

#include "globals.h"

namespace
{
	constexpr auto kDefaultShader = "Fallout4.esm|0x22517C";  // DetectLifeFXS

	std::uint32_t g_current{ 0 };

	RE::TESEffectShader* ShaderForm()
	{
		const auto identifier = globals::g_ini ? globals::g_ini->at("MENU/sHighlightShader", std::string(kDefaultShader)) : std::string(kDefaultShader);
		const auto bar = identifier.find('|');
		if (bar == std::string::npos) {
			logger::warn("ActorHighlight: bad sHighlightShader '{}' (expected Plugin.esp|0xLOCALID)", identifier);
			return nullptr;
		}
		auto* form = functions::getFormFromString(identifier.substr(bar + 1), identifier.substr(0, bar));
		auto* shader = form ? RE::fallout_cast<RE::TESEffectShader*>(form) : nullptr;
		if (!shader) {
			logger::warn("ActorHighlight: shader '{}' not found or is not an effect shader", identifier);
		}
		return shader;
	}

	// TESObjectREFR::ApplyEffectShader (1.11.240.0 RVA 0x5E21A0 / NG_ID 2205201; OG ID 652173 / 1.10.163 RVA 0x422180). Возвращает
	// ShaderReferenceEffect*, завершаем установкой finished (offset 0x40) = true.
	void* g_activeEffect{ nullptr };

	void ApplyShader(RE::TESObjectREFR* a_ref, RE::TESEffectShader* a_shader)
	{
		using func_t = void* (*)(RE::TESObjectREFR*, RE::TESEffectShader*, float, RE::TESObjectREFR*, bool, bool, RE::NiAVObject*, bool);
		static REL::Relocation<func_t> func{ REL::Module::get().base() + 0x5E21A0 };
		g_activeEffect = func(a_ref, a_shader, -1.0f, nullptr, false, false, nullptr, false);
	}

	void Stop(std::uint32_t a_actorFormID, RE::TESEffectShader* a_shader)
	{
		if (g_activeEffect) {
			*reinterpret_cast<bool*>(reinterpret_cast<char*>(g_activeEffect) + 0x40) = true;
			g_activeEffect = nullptr;
		}
	}
}

namespace ActorHighlight
{
	void Set(std::uint32_t a_actorFormID)
	{
		if (a_actorFormID == g_current) {
			return;
		}
		auto* shader = ShaderForm();
		if (g_current) {
			Stop(g_current, shader);
			g_current = 0;
		}
		if (!a_actorFormID || !shader) {
			return;
		}
		auto* actor = functions::getFormByFormID<RE::Actor>(a_actorFormID);
		if (!actor || !actor->GetFullyLoaded3D()) {
			return;
		}
		ApplyShader(actor, shader);
		g_current = a_actorFormID;
		logger::info("ActorHighlight: {:08X} highlighted", a_actorFormID);
	}

	std::uint32_t Current()
	{
		return g_current;
	}
}
