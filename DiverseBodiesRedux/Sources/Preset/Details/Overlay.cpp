#include "Overlay.h"
#include "LooksMenu/LooksMenuInterfaces.h"
#include "LooksMenu/ParseLooksMenuPreset.h"

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

constexpr float EPSILON = 1e-5f;

inline bool operator==(const RE::NiColorA& lhs, const RE::NiColorA& rhs) noexcept {
	auto float_eq = [](float a, float b)->bool { return std::fabs(a - b) < EPSILON; };

	auto point2_eq = [float_eq](const RE::NiColorA& a, const RE::NiColorA& b)->bool {
		return float_eq(a.r, b.r) && float_eq(a.g, b.g) && float_eq(a.b, b.b);
	};

	return  point2_eq(lhs, rhs);
}

inline bool operator==(const RE::NiPoint2& lhs, const RE::NiPoint2& rhs) noexcept {
	auto float_eq = [](float a, float b)->bool { return std::fabs(a - b) < EPSILON; };

	auto point2_eq = [float_eq](const RE::NiPoint2& a, const RE::NiPoint2& b)->bool {
		return float_eq(a.x, b.x) && float_eq(a.y, b.y);
		};

	return point2_eq(lhs,rhs);
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
		m_id.data(),
		m_tint,
		m_offsetUV,
		m_scaleUV
	);
}

const std::string& Overlay::id() const noexcept {
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
		Overlay::clear();
	}

	auto it = find_ci(obj, "template");

	if (it != obj.end()) {
		auto& value = it->value();
		if (value.is_string()) {
			m_id = value.as_string().c_str();
		}
	}

	if (m_id.empty()) {
		logger::error("Overlay::loadFromJsonValue: ID is empty or not found in JSON object.");
		return false; // ID is required
	}

	it = find_ci(obj, "tint");
	bool success = false;

	//"tint": [ 0.0, 0.0, 0.0, 1.0 ]
	if (success = it != obj.end(); success) {
		auto& value = it->value();
		if (value.is_array()) {
			auto array = value.as_array();
			if (array.size() == 4) {
				if (
					(array[0].is_double() || array[0].is_int64()) &&
					(array[1].is_double() || array[1].is_int64()) &&
					(array[2].is_double() || array[2].is_int64()) &&
					(array[3].is_double() || array[3].is_int64())
					) {
					m_tint.r = array[0].is_double() ? static_cast<float>(array[0].as_double()) : static_cast<float>(array[0].as_int64());
					m_tint.g = array[1].is_double() ? static_cast<float>(array[1].as_double()) : static_cast<float>(array[1].as_int64());
					m_tint.b = array[2].is_double() ? static_cast<float>(array[2].as_double()) : static_cast<float>(array[2].as_int64());
					m_tint.a = array[3].is_double() ? static_cast<float>(array[3].as_double()) : static_cast<float>(array[3].as_int64());
				}
				else {
					success = false;
					logger::error("Overlay::loadFromJsonValue: Tint array contains non-double/int values.");
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
	
	it = find_ci(obj, "offsetUV");
	success = false;

	//"offsetUV": [ 0.0, 0.0 ]
	if (success = it != obj.end(); success) {
		auto& value = it->value();
		if (value.is_array()) {
			auto array = value.as_array();
			if (array.size() == 2) {
				if (
					(array[0].is_double() || array[0].is_int64()) &&
					(array[1].is_double() || array[1].is_int64())
					) {
					m_offsetUV.x = array[0].is_double() ? static_cast<float>(array[0].as_double()) : static_cast<float>(array[0].as_int64());
					m_offsetUV.y = array[1].is_double() ? static_cast<float>(array[1].as_double()) : static_cast<float>(array[1].as_int64());
				}
				else {
					success = false;
					logger::error("Overlay::loadFromJsonValue: OffsetUV array contains non-double/int values.");
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

	it = find_ci(obj, "scaleUV");
	success = false;

	//"scaleUV": [ 1.0, 1.0 ]
	if (success = it != obj.end(); success) {
		auto& value = it->value();
		if (value.is_array()) {
			auto array = value.as_array();
			if (array.size() == 2) {
				if (
					(array[0].is_double() || array[0].is_int64()) &&
					(array[1].is_double() || array[1].is_int64())
					) {
					m_scaleUV.x = array[0].is_double() ? static_cast<float>(array[0].as_double()) : static_cast<float>(array[0].as_int64());
					m_scaleUV.y = array[1].is_double() ? static_cast<float>(array[1].as_double()) : static_cast<float>(array[1].as_int64());
				}
				else {
					success = false;
					logger::error("Overlay::loadFromJsonValue: ScaleUV array contains non-double/int values.");
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

	it = find_ci(obj, "priority");
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