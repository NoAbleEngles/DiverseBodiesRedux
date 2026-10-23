#pragma once

#include <cstdint>

// FormID из co-save нельзя использовать как есть: он записан в порядке плагинов того момента, и после
// добавления/удаления плагина тот же номер указывает на другую форму. F4SE пересчитывает его
// (SerializationInterface::ResolveFormID, только внутри load-колбэка). Временные формы (0xFF......)
// от порядка плагинов не зависят и возвращаются как есть.
//
// 0 - плагина, под которым FormID был записан, в этой сборке больше нет (запись надо пропустить).
inline std::uint32_t ResolveSavedFormID(std::uint32_t a_saved)
{
	if (a_saved == 0 || (a_saved >> 24) == 0xFF) {
		return a_saved;
	}
	const auto* intfc = F4SE::GetSerializationInterface();
	if (!intfc) {
		return a_saved;
	}
	const auto resolved = intfc->ResolveFormID(a_saved);
	return resolved ? static_cast<std::uint32_t>(*resolved) : 0;
}
