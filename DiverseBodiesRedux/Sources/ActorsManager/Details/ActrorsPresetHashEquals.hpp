#pragma	once
#include <Preset/Preset.h>

struct actorPresetsHash
{
    inline size_t operator()(const std::shared_ptr<Preset>& preset) const noexcept {
		if (!preset) {
			return 0;
		}
		return static_cast<size_t>(preset->type());
	}
};

struct actorPresetsEquals
{
    inline bool operator()(const std::shared_ptr<Preset>& lhs, const std::shared_ptr<Preset>& rhs) const noexcept {
		if (!lhs || !rhs) {
			return false;
		}
		return lhs->type() == rhs->type();
	}
};
