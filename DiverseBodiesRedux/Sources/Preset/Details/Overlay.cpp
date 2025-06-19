#include "Overlay.h"
#include "LooksMenu/LooksMenuInterfaces.h"

Overlay::Overlay(const boost::json::object& jsonObject) {
	loadFromJsonObject(jsonObject);
}

Overlay::Overlay(
	std::string id,
	RE::NiColorA tint,
	RE::NiPoint2 offsetUV,
	RE::NiPoint2 scaleUV,
	int priority) noexcept : 
	m_id(std::move(id)), m_tint(tint), m_offsetUV(offsetUV), m_scaleUV(scaleUV), m_priority(priority) {}

bool Overlay::operator<(const Overlay& other) const noexcept {
	if (m_id != other.m_id) {
		return m_priority < other.m_priority;
	}
	else {
		return (m_id < other.m_id);
	}
}

inline bool operator==(const RE::NiColorA& lhs, const RE::NiColorA& rhs) noexcept {
	return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}

inline bool operator==(const RE::NiPoint2& lhs, const RE::NiPoint2& rhs) noexcept {
	return lhs.x == rhs.x && lhs.y == rhs.y;
}

bool Overlay::operator==(const Overlay& other) const noexcept {
	return m_id == other.m_id &&
		m_tint == other.m_tint &&
		m_offsetUV == other.m_offsetUV &&
		m_scaleUV == other.m_scaleUV &&
		m_priority == other.m_priority;
}

void Overlay::apply(RE::Actor* actor) const {
	LooksMenuInterfaces<OverlayInterface>::GetInterface()->AddOverlay(
		actor,
		actor->GetSex() == RE::Actor::Sex::Female,
		m_priority,
		m_id,
		m_tint,
		m_offsetUV,
		m_scaleUV
	);
}

std::string Overlay::id() const noexcept {
	return m_id;
}

bool Overlay::empty() const noexcept {
	return m_id.empty();
}

void Overlay::clear() noexcept {
	*this = std::move(Overlay{});
}

bool Overlay::loadFromJsonObject(const boost::json::object& obj) noexcept {

	if (!this->empty()) {
		this->clear();
	}

	auto it = obj.find("id");

	if (it != obj.end()) {
		auto& value = it->value();
		if (value.is_string()) {
			m_id = value.as_string();
		}
	}

	if (m_id.empty()) {
		logger::error("Overlay::loadFromJsonValue: ID is empty or not found in JSON object.");
		return false; // ID is required
	}

	it = obj.find("tint");
	bool success = false;

	//"tint": [ 0.0, 0.0, 0.0, 1.0 ]
	if (success = it != obj.end(); success) {
		auto& value = it->value();
		if (value.is_array()) {
			auto array = value.as_array();
			if (array.size() == 4) {
				if (array[0].is_double() && array[1].is_double() && array[2].is_double() && array[3].is_double()) {
					m_tint.r = static_cast<float>(array[0].as_double());
					m_tint.g = static_cast<float>(array[1].as_double());
					m_tint.b = static_cast<float>(array[2].as_double());
					m_tint.a = static_cast<float>(array[3].as_double());
				}
				else {
					success = false;
					logger::error("Overlay::loadFromJsonValue: Tint array contains non-double values.");
				}
			} else {
				success = false;
				logger::error("Overlay::loadFromJsonValue: Tint array size is not 4.");
			}
		} else {
			success = false;
			logger::error("Overlay::loadFromJsonValue: Tint is not an array.");
		}
	}

	if (!success) {
		m_tint = RE::NiColorA{ 0.0f, 0.0f, 0.0f, 1.0f }; // Default tint value
	}
	
	it = obj.find("offsetUV");
	success = false;

	//"offsetUV": [ 0.0, 0.0 ]
	if (success = it != obj.end(); success) {
		auto& value = it->value();
		if (value.is_array()) {
			auto array = value.as_array();
			if (array.size() == 2) {
				if (array[0].is_double() && array[1].is_double()) {
					m_offsetUV.x = static_cast<float>(array[0].as_double());
					m_offsetUV.y = static_cast<float>(array[1].as_double());
				} else {
					success = false;
					logger::error("Overlay::loadFromJsonValue: OffsetUV array contains non-double values.");
				}
			} else {
				success = false;
				logger::error("Overlay::loadFromJsonValue: OffsetUV array size is not 2.");
			}
		} else {
			success = false;
			logger::error("Overlay::loadFromJsonValue: OffsetUV is not an array.");
		}
	}

	if (!success) {
		m_offsetUV = RE::NiPoint2{}; // Default offset value
	}

	it = obj.find("scaleUV");
	success = false;

	//"scaleUV": [ 1.0, 1.0 ]
	if (success = it != obj.end(); success) {
		auto& value = it->value();
		if (value.is_array()) {
			auto array = value.as_array();
			if (array.size() == 2) {
				if (array[0].is_double() && array[1].is_double()) {
					m_scaleUV.x = static_cast<float>(array[0].as_double());
					m_scaleUV.y = static_cast<float>(array[1].as_double());
				} else {
					success = false;
					logger::error("Overlay::loadFromJsonValue: ScaleUV array contains non-double values.");
				}
			} else {
				success = false;
				logger::error("Overlay::loadFromJsonValue: ScaleUV array size is not 2.");
			}
		} else {
			success = false;
			logger::error("Overlay::loadFromJsonValue: ScaleUV is not an array.");
		}
	}

	if (!success) {
		m_scaleUV = RE::NiPoint2{ 1.0f, 1.0f }; // Default scale value
	}

	it = obj.find("priority");
	success = false;
	//"priority": 0

	if (success = it != obj.end(); success) {
		auto& value = it->value();
		if (value.is_int64()) {
			m_priority = static_cast<int>(value.as_int64());
		} else if (value.is_double()) {
			m_priority = static_cast<int>(value.as_double()); // Handle double as well
		} else {
			success = false;
			logger::error("Overlay::loadFromJsonValue: Priority is not an integer or double.");
		}
	} 

	if (!success) {
		m_priority = 0; // Default priority value
	}

	return true;
}