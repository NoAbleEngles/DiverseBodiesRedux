#pragma once
#include "F4SE/F4SE.h"
#include "RE/Fallout.h"
#include <boost/json.hpp>

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
};