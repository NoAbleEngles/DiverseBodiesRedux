#pragma	once
#include <memory>
#include <unordered_set>
#include <Preset/Preset.h>

struct actorPresetsHash
{
    size_t operator()(const std::shared_ptr<Preset>& preset) const noexcept {
        if (!preset) {
            return 0;
        }
        // Получаем хеш от типа (int) и id (std::string)
        size_t h1 = std::hash<int>()(static_cast<int>(preset->type()));
        size_t h2 = std::hash<std::string>()(preset->id());
        // Комбинируем два хеша
        return h1 ^ (h2 << 1);
    }
};

struct actorPresetsEquals
{
    inline bool operator()(const std::shared_ptr<Preset>& lhs, const std::shared_ptr<Preset>& rhs) const noexcept {
		if (!lhs || !rhs) {
			logger::error("ActorPresetsEquals: One of the presets is null");
			return false;
		}
		return lhs->type() == rhs->type();
	}
};

namespace ActorsManagerDefs {
    using Presets = std::unordered_set<std::shared_ptr<Preset>, actorPresetsHash, actorPresetsEquals>;
    using ActorPreset = std::pair<uint32_t, Presets>;
}
