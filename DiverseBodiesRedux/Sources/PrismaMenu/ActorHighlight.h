#pragma once

#include <cstdint>

// Подсветка NPC шейдером (TESEffectShader) в мире, пока тот наведён в списке меню, чтобы было
// видно, какого именно персонажа выбираешь (после выбора подсветка снимается). Одновременно подсвечен один. Игровой поток.
//
// Шейдер: [MENU] sHighlightShader в ini, формат "Plugin.esp|0xLOCALID" (по умолчанию
// Fallout4.esm|0x22517C, DetectLifeFXS - тот же, что в Underhelmet Hairfix Redux).
namespace ActorHighlight
{
	// Подсветить актёра (formID), снять подсветку с прежнего. 0 - просто снять.
	void Set(std::uint32_t a_actorFormID);

	// Текущий подсвеченный актёр или 0.
	std::uint32_t Current();
}
