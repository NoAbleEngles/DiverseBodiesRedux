#pragma once
#include "F4SE/F4SE.h"
#include "RE/Fallout.h"
#include <boost/json.hpp>
#include <functional>

class Overlay
{
public:
	Overlay() noexcept = default;
	Overlay(const boost::json::object& jsonObject);
	Overlay(const Overlay&) noexcept = default;
	Overlay(Overlay&&) noexcept = default;
	~Overlay() noexcept = default;

	Overlay(
		std::string id,
		RE::NiColorA tint,
		RE::NiPoint2 offsetUV,
		RE::NiPoint2 scaleUV,
		int priority) noexcept;

	Overlay& operator=(const Overlay&) noexcept = default;
	Overlay& operator=(Overlay&&) noexcept = default;
	bool operator==(const Overlay&) const noexcept;
	bool operator<(const Overlay& other) const noexcept;

	// @brief Применяет наложение к актеру. 
	// * Не проверяет интерфейс, актёра и наложение на валидность.
	void remove(RE::Actor* actor) const; ///< Удаляет наложение с актёра
	void apply(RE::Actor* actor) const;
	const std::string& id() const noexcept;
	bool empty() const noexcept;
	void clear() noexcept;

private:

	std::string m_id{};                        ///< Уникальный идентификатор наложения
	RE::NiColorA m_tint{ 0.0f, 0.0f, 0.0f, 1.0f };  ///< Цвет наложения (RGBA)
	RE::NiPoint2 m_offsetUV{ 0.0, 0.0 };            ///< Смещение UV-координат
	RE::NiPoint2 m_scaleUV{ 1.0, 1.0 };             ///< Масштаб UV-координат
	int m_priority{};                               ///< Приоритет наложения (более высокий перекрывает видимость более низкого)

	bool loadFromJsonObject(const boost::json::object& jsonObject) noexcept; ///< Загружает данные из JSON-объекта

	friend struct std::hash<Overlay>; ///< Дружественная функция для хеширования
};

namespace std {
	template<>
	struct hash<Overlay> {
		std::size_t operator()(const Overlay& o) const noexcept {
			std::size_t h = std::hash<std::string>{}(o.id());
			h ^= std::hash<int>{}(o.m_priority) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= std::hash<float>{}(o.m_tint.r) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= std::hash<float>{}(o.m_tint.g) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= std::hash<float>{}(o.m_tint.b) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= std::hash<float>{}(o.m_tint.a) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= std::hash<float>{}(o.m_offsetUV.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= std::hash<float>{}(o.m_offsetUV.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= std::hash<float>{}(o.m_scaleUV.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= std::hash<float>{}(o.m_scaleUV.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
			return h;
		}
	};
}