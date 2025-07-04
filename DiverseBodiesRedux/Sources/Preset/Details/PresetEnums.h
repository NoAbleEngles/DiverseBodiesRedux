#pragma once
#include <string>
#include <cctype>

/**
* @brief Типы тел для пресетов.
*/
enum class BodyType : char {
	NONE,  ///< Не определено
	FAT,   ///< Полный
	SLIM,  ///< Худой
	SEXY,  ///< Стандартный/сексуальный
	ATHL,  ///< Атлетичный
	END    ///< Конец перечисления, используется для прокрутки в циклах
};

inline BodyType GetBodyType(int i) {
	if (i < 0 || i >= static_cast<int>(BodyType::END)) {
		return BodyType::NONE; // Возвращаем NONE для недопустимых значений
	}
	return static_cast<BodyType>(i);
}

inline BodyType GetBodyType(double d) {
	return GetBodyType(static_cast<int>(d));
}

inline bool CompareIgnoreCase(const std::string& a, const std::string& b) {
	if (a.size() != b.size()) return false;
	for (size_t i = 0; i < a.size(); ++i) {
		if (std::tolower(a[i]) != std::tolower(b[i]))
			return false;
	}
	return true;
};

inline BodyType GetBodyType(const std::string& str) {

	if (CompareIgnoreCase(str, "FAT")) {
		return BodyType::FAT;
	} else if (CompareIgnoreCase(str, "SLIM")) {
		return BodyType::SLIM;
	} else if (CompareIgnoreCase(str, "SEXY")) {
		return BodyType::SEXY;
	} else if (CompareIgnoreCase(str, "ATHL")) {
		return BodyType::ATHL;
	}
	return BodyType::NONE;
}

/**
 * @brief Типы пресетов.
 */
enum class PresetType : char
{
	NONE,			///< Не определено
	BODYMORPHS,		///< Пресет морфов тела
	BODYHAIRS,		///< Пресет наложений волосы на тело
	END				///< Конец перечисления, используется для прокрутки в циклах
};

inline constexpr std::string GetPresetTypeString(PresetType type) {
	switch (type) {
	case PresetType::BODYMORPHS:
		return "BodyMorphs";
	case PresetType::BODYHAIRS:
		return "BodyHairs";
	default:
		return "None";
	}
}