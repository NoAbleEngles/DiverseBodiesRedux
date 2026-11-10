#include "ClothMorphFilter.h"
#include "PatchesMemUtils.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace patches {
	namespace {
		// f4ee.dll v1.7.0.4 BodyMorphProcessor::Process RVA (vtable[1] at 0xC7190 -> 0x14D10).
		// Prologue: 40 55 53 56 57 41 54 41 55 41 56 41 57 (25 bytes of pure register pushes/frame setup).
		constexpr std::uintptr_t kF4EE_BodyMorphProcessor_Process_RVA = 0x14D10;
		constexpr std::uint8_t kExpectedPrologue[13] = {
			0x40, 0x55, 0x53, 0x56, 0x57,
			0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57
		};

		inline char AsciiLower(char c) noexcept
		{
			return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
		}

		inline bool ContainsCaseInsensitive(std::string_view haystack, std::string_view needle) noexcept
		{
			if (needle.empty() || haystack.size() < needle.size()) {
				return false;
			}
			const std::size_t limit = haystack.size() - needle.size();
			for (std::size_t i = 0; i <= limit; ++i) {
				bool match = true;
				for (std::size_t j = 0; j < needle.size(); ++j) {
					if (AsciiLower(haystack[i + j]) != AsciiLower(needle[j])) {
						match = false;
						break;
					}
				}
				if (match) {
					return true;
				}
			}
			return false;
		}

		inline bool HasNiRTTI(const RE::NiObject* obj, std::string_view targetName) noexcept
		{
			if (!obj || !IsReadableFast(obj, sizeof(void*))) {
				return false;
			}
			for (const RE::NiRTTI* r = obj->GetRTTI(); r && IsReadableFast(r, sizeof(RE::NiRTTI)); r = r->baseRTTI) {
				const char* nm = r->GetName();
				if (nm && std::string_view(nm) == targetName) {
					return true;
				}
			}
			return false;
		}

		inline bool HasExtraDataFast(const RE::NiObjectNET* obj, std::string_view targetName)
		{
			if (!obj || !obj->extra) {
				return false;
			}
			RE::BSAutoReadLock lk(obj->extra->lock);
			const auto& arr = obj->extra->data;
			const std::uint32_t sz = arr.size();
			for (std::uint32_t i = 0; i < sz; ++i) {
				const auto& elem = arr[i];
				if (elem && elem->GetName() == targetName) {
					return true;
				}
			}
			return false;
		}

		inline bool RemoveExtraDataByName(RE::NiObjectNET* obj, std::string_view targetName)
		{
			if (!obj || !obj->extra) {
				return false;
			}
			bool removed = false;
			RE::BSAutoWriteLock lk(obj->extra->lock);
			auto& arr = obj->extra->data;
			for (std::uint32_t i = arr.size(); i > 0; --i) {
				const std::uint32_t idx = i - 1;
				if (arr[idx] && arr[idx]->GetName() == targetName) {
					arr.erase(arr.begin() + idx, arr.begin() + idx + 1);
					removed = true;
				}
			}
			return removed;
		}

		inline float HalfToFloat(std::uint16_t h) noexcept
		{
			const std::uint32_t sign = static_cast<std::uint32_t>(h & 0x8000U) << 16;
			std::uint32_t exp = (h >> 10) & 0x1FU;
			std::uint32_t mant = h & 0x03FFU;

			std::uint32_t bits = 0;
			if (exp == 0) {
				if (mant == 0) {
					bits = sign;
				} else {
					exp = 1;
					while ((mant & 0x0400U) == 0) {
						mant <<= 1;
						--exp;
					}
					mant &= 0x03FFU;
					bits = sign | ((exp + 112U) << 23) | (mant << 13);
				}
			} else if (exp == 31U) {
				return 0.0f;
			} else {
				bits = sign | ((exp + 112U) << 23) | (mant << 13);
			}

			float out = 0.0f;
			std::memcpy(&out, &bits, sizeof(float));
			return out;
		}

		inline std::uint64_t PackQuantizedVertex(float x, float y, float z) noexcept
		{
			auto q = [](float v) noexcept -> std::uint64_t {
				if (!std::isfinite(v)) {
					return 1048576ULL;
				}
				std::int64_t iv = static_cast<std::int64_t>(std::llround(static_cast<double>(v) * 100.0));
				iv = std::clamp<std::int64_t>(iv, -1000000LL, 1000000LL);
				return static_cast<std::uint64_t>(iv + 1048576LL) & 0x1FFFFFULL;
			};
			return (q(x) << 42) | (q(y) << 21) | q(z);
		}

		inline std::size_t CountSortedIntersectionAtLeast(
			const std::vector<std::uint64_t>& a,
			const std::vector<std::uint64_t>& b,
			std::size_t threshold) noexcept
		{
			std::size_t i = 0;
			std::size_t j = 0;
			std::size_t count = 0;
			while (i < a.size() && j < b.size()) {
				if (a[i] < b[j]) {
					++i;
				} else if (b[j] < a[i]) {
					++j;
				} else {
					++count;
					if (count >= threshold) {
						return count;
					}
					++i;
					++j;
				}
			}
			return count;
		}

		// Extract target mesh names directly from BSClothExtraData -> hclClothData -> hclBufferDefinition::m_name (+0x10 hkStringPtr).
		// Verified against Fallout4.exe 0x1418A3B18 (mov rbx, [r13 + 0x10]; and rbx, ~1ULL) & 0x1413BD43F.
		// NOTE: Never call hclBufferDefinition vtbl[5] (+0x28) — that is createBuffer(this, hkResult*, hclBuffer*), NOT getMeshName()!
		void CollectClothExtraTargetShapes(const RE::NiObjectNET* obj, std::unordered_set<std::string>& outNames)
		{
			if (!obj || !obj->extra) {
				return;
			}

			// Snapshot BSClothExtraData pointers under read lock, then release the lock immediately
			// so extra->lock is never held while inspecting Havok packfile structures.
			RE::NiPointer<RE::NiExtraData> clothBlocks[16]{};
			std::size_t numClothBlocks = 0;
			{
				RE::BSAutoReadLock lk(obj->extra->lock);
				const auto& arr = obj->extra->data;
				for (std::uint32_t e = 0; e < arr.size() && numClothBlocks < std::size(clothBlocks); ++e) {
					const auto& sp = arr[e];
					if (sp && HasNiRTTI(sp.get(), "BSClothExtraData")) {
						clothBlocks[numClothBlocks++] = sp;
					}
				}
			}

			for (std::size_t idx = 0; idx < numClothBlocks; ++idx) {
				const RE::NiExtraData* ed = clothBlocks[idx].get();
				const auto* edBytes = reinterpret_cast<const std::uint8_t*>(ed);
				if (!IsReadableFast(edBytes + 0x18, 0x18)) {
					continue;
				}
				auto* const* clothDatas = *reinterpret_cast<void* const* const*>(edBytes + 0x18);
				const std::uint32_t numClothDatas = *reinterpret_cast<const std::uint32_t*>(edBytes + 0x28);
				if (!clothDatas || numClothDatas == 0 || numClothDatas > 64 ||
					!IsReadableFast(clothDatas, numClothDatas * sizeof(void*))) {
					continue;
				}
				for (std::uint32_t c = 0; c < numClothDatas; ++c) {
					const auto* cd = reinterpret_cast<const std::uint8_t*>(clothDatas[c]);
					if (!cd || !IsReadableFast(cd + 0x28, 0x10)) {
						continue;
					}
					auto* const* bufDefs = *reinterpret_cast<void* const* const*>(cd + 0x28);
					const std::int32_t numBufDefs = *reinterpret_cast<const std::int32_t*>(cd + 0x30);
					if (!bufDefs || numBufDefs <= 0 || numBufDefs > 128 ||
						!IsReadableFast(bufDefs, static_cast<std::size_t>(numBufDefs) * sizeof(void*))) {
						continue;
					}
					for (std::int32_t b = 0; b < numBufDefs; ++b) {
						const auto* bufBytes = reinterpret_cast<const std::uint8_t*>(bufDefs[b]);
						if (!bufBytes || !IsReadableFast(bufBytes, 0x20)) {
							continue;
						}
						const std::uintptr_t rawStrPtr =
							*reinterpret_cast<const std::uintptr_t*>(bufBytes + 0x10) & ~static_cast<std::uintptr_t>(1U);
						const auto* meshName = reinterpret_cast<const char*>(rawStrPtr);
						if (meshName && IsReadableFast(meshName, 1) && meshName[0] != '\0') {
							outNames.emplace(meshName);
						}
					}
				}
			}
		}

		// Verified Fallout 4 AE BSGeometry / BSLightingShaderProperty offsets:
		//   BSGeometry + 0x130: NiPointer<NiProperty> properties[0] (alpha)
		//   BSGeometry + 0x138: NiPointer<NiProperty> properties[1] (shader)
		//   BSShaderProperty + 0x58: BSShaderMaterial* material (vtable[5] GetFeature(): 4 = kFace, 5 = kSkinTint)
		//   BSLightingShaderProperty + 0xC0: BSFixedString rootName (.bgsm path)
		bool IsSkinTriShape(const RE::BSTriShape* shape)
		{
			const auto* shapeBytes = reinterpret_cast<const std::uint8_t*>(shape);
			if (!IsReadableFast(shapeBytes + 0x130, 0x10)) {
				return false;
			}
			for (std::size_t slot : { 0x138ULL, 0x130ULL }) {
				auto* prop = *reinterpret_cast<RE::NiObjectNET* const*>(shapeBytes + slot);
				if (!prop || !HasNiRTTI(prop, "BSShaderProperty")) {
					continue;
				}
				const std::string_view propName = prop->GetName();
				if (ContainsCaseInsensitive(propName, "human") && ContainsCaseInsensitive(propName, "skin")) {
					return true;
				}
				const auto* propBytes = reinterpret_cast<const std::uint8_t*>(prop);
				if (IsReadableFast(propBytes + 0x58, sizeof(void*))) {
					void* mat = *reinterpret_cast<void* const*>(propBytes + 0x58);
					if (mat && IsReadableFast(mat, sizeof(void*))) {
						void* const* vtbl = *reinterpret_cast<void* const* const*>(mat);
						if (vtbl && IsReadableFast(vtbl + 5, sizeof(void*))) {
							using GetFeature_t = std::int32_t(__fastcall*)(void*);
							const std::int32_t feat = reinterpret_cast<GetFeature_t>(vtbl[5])(mat);
							if (feat == 4 || feat == 5) { // kFace (4) or kSkinTint (5)
								return true;
							}
						}
					}
				}
				if (HasNiRTTI(prop, "BSLightingShaderProperty") && IsReadableFast(propBytes + 0xC0, sizeof(RE::BSFixedString))) {
					const auto& rootMatName = *reinterpret_cast<const RE::BSFixedString*>(propBytes + 0xC0);
					const std::string_view sv(rootMatName);
					if (ContainsCaseInsensitive(sv, "human") && ContainsCaseInsensitive(sv, "skin")) {
						return true;
					}
				}
			}
			return false;
		}

		struct ShapeClothInfo
		{
			RE::NiPointer<RE::BSTriShape> shape;
			std::string name;
			bool isSkin{ false };
			bool isCloth{ false };
			const std::uint8_t* rawVerts{ nullptr };
			std::uint32_t vertexSize{ 0 };
			std::uint16_t numVerts{ 0 };
			bool isFullPrec{ false };
			std::vector<std::uint64_t> quantizedVerts;
		};

		// Verified Fallout 4 AE BSTriShape / BSSkin::Instance / BSGraphics::TriShape offsets:
		//   BSGeometry + 0x140: NiPointer<BSSkin::Instance> skinInstance ([+0x10] = NiAVObject** bones, [+0x20] = uint32_t numBones)
		//   BSGeometry + 0x148: BSGraphics::TriShape* rendererData ([+0x08] = BSGraphics::VertexBuffer* vb; [vb + 0x08] = uint8_t* rawVerts)
		//   BSGeometry + 0x150: uint64_t vertexDesc
		//   BSTriShape + 0x164: uint16_t numVertices
		void InspectTriShape(
			RE::BSTriShape* shape,
			const std::unordered_set<std::string>& directClothNames,
			ShapeClothInfo& out)
		{
			out.shape.reset(shape);
			out.name = std::string(shape->GetName());
			out.isSkin = IsSkinTriShape(shape);

			const auto* shapeBytes = reinterpret_cast<const std::uint8_t*>(shape);
			if (!IsReadableFast(shapeBytes + 0x140, 0x28)) {
				return;
			}

			const std::uint64_t desc = *reinterpret_cast<const std::uint64_t*>(shapeBytes + 0x150);
			const std::uint32_t vsz = static_cast<std::uint32_t>(desc & 0xFULL) * 4U;
			const std::uint32_t skinOff = static_cast<std::uint32_t>((desc >> 28) & 0xFULL) * 4U;
			const std::uint32_t vflags = static_cast<std::uint32_t>((desc >> 44) & 0xFFFULL);
			const bool isSkinned = (vflags & 0x40U) != 0;
			const bool isFullPrec = (vflags & 0x400U) != 0;
			const std::uint16_t numVerts = *reinterpret_cast<const std::uint16_t*>(shapeBytes + 0x164);

			out.vertexSize = vsz;
			out.numVerts = numVerts;
			out.isFullPrec = isFullPrec;

			void* rendererData = *reinterpret_cast<void* const*>(shapeBytes + 0x148);
			if (rendererData && IsReadableFast(rendererData, 0x10)) {
				void* vb = *reinterpret_cast<void* const*>(reinterpret_cast<const std::uint8_t*>(rendererData) + 0x08);
				if (vb && IsReadableFast(vb, 0x10)) {
					const auto* raw = *reinterpret_cast<const std::uint8_t* const*>(reinterpret_cast<const std::uint8_t*>(vb) + 0x08);
					if (raw && vsz >= 8 && numVerts > 0 &&
						IsReadableFast(raw, static_cast<std::size_t>(numVerts) * static_cast<std::size_t>(vsz))) {
						out.rawVerts = raw;
					}
				}
			}

			if (!out.isSkin && !out.name.empty() && directClothNames.contains(out.name)) {
				out.isCloth = true;
				return;
			}

			// Check BSSkin::Instance bone palette for custom cloth/skirt bones (standard FO4 skeleton has 0 such bones)
			// and verify at least one vertex actually has non-zero skin weight (> 0.01) on those cloth bones.
			void* skinInst = *reinterpret_cast<void* const*>(shapeBytes + 0x140);
			if (!skinInst || !IsReadableFast(skinInst, 0x28)) {
				return;
			}
			auto* const* bones = *reinterpret_cast<RE::NiAVObject* const* const*>(reinterpret_cast<const std::uint8_t*>(skinInst) + 0x10);
			const std::uint32_t numBones = *reinterpret_cast<const std::uint32_t*>(reinterpret_cast<const std::uint8_t*>(skinInst) + 0x20);
			if (!bones || numBones == 0 || numBones > 512 || !IsReadableFast(bones, numBones * sizeof(void*))) {
				return;
			}

			bool clothBoneMask[256]{};
			bool anyClothBone = false;
			const std::uint32_t maxPaletteIdx = std::min<std::uint32_t>(numBones, 256U);
			for (std::uint32_t b = 0; b < maxPaletteIdx; ++b) {
				const RE::NiAVObject* bone = bones[b];
				if (!bone || !IsReadableFast(bone, 0x18)) {
					continue;
				}
				const std::string_view bname = bone->GetName();
				if (ContainsCaseInsensitive(bname, "cloth") || ContainsCaseInsensitive(bname, "skirt")) {
					clothBoneMask[b] = true;
					anyClothBone = true;
				}
			}

			if (!anyClothBone) {
				return;
			}

			// Verify per-vertex weight > 0.01 so rigid parts with 0-weight leftover bones (boots, heels, socks) stay morphable.
			if (out.rawVerts && isSkinned && (skinOff + 12U) <= vsz) {
				for (std::uint16_t v = 0; v < numVerts; ++v) {
					const std::uint8_t* vbase = out.rawVerts + static_cast<std::size_t>(v) * vsz;
					const auto* wh = reinterpret_cast<const std::uint16_t*>(vbase + skinOff);
					const std::uint8_t* bis = vbase + skinOff + 8U;
					float clothWeightSum = 0.0f;
					for (int k = 0; k < 4; ++k) {
						const std::uint8_t bidx = bis[k];
						if (clothBoneMask[bidx]) {
							const float w = HalfToFloat(wh[k]);
							if (w > 0.001f) {
								clothWeightSum += w;
							}
						}
					}
					if (clothWeightSum > 0.01f) {
						out.isCloth = true;
						return;
					}
				}
			}
		}

		void PopulateQuantizedVertices(ShapeClothInfo& info)
		{
			if (!info.rawVerts || info.numVerts == 0 || info.vertexSize < 8) {
				return;
			}
			info.quantizedVerts.reserve(info.numVerts);
			for (std::uint16_t v = 0; v < info.numVerts; ++v) {
				const std::uint8_t* vbase = info.rawVerts + static_cast<std::size_t>(v) * info.vertexSize;
				float x = 0.0f;
				float y = 0.0f;
				float z = 0.0f;
				if (info.isFullPrec && info.vertexSize >= 12) {
					std::memcpy(&x, vbase + 0, sizeof(float));
					std::memcpy(&y, vbase + 4, sizeof(float));
					std::memcpy(&z, vbase + 8, sizeof(float));
				} else {
					std::uint16_t hx = 0, hy = 0, hz = 0;
					std::memcpy(&hx, vbase + 0, sizeof(std::uint16_t));
					std::memcpy(&hy, vbase + 2, sizeof(std::uint16_t));
					std::memcpy(&hz, vbase + 4, sizeof(std::uint16_t));
					x = HalfToFloat(hx);
					y = HalfToFloat(hy);
					z = HalfToFloat(hz);
				}
				info.quantizedVerts.push_back(PackQuantizedVertex(x, y, z));
			}
			std::sort(info.quantizedVerts.begin(), info.quantizedVerts.end());
			info.quantizedVerts.erase(
				std::unique(info.quantizedVerts.begin(), info.quantizedVerts.end()),
				info.quantizedVerts.end());
		}

		__declspec(noinline) void ProcessBodyMorphWithClothFilter(
			void* a_this,
			void* a_args,
			const char* a_modelName,
			RE::NiPointer<RE::NiNode>* a_root,
			std::uint32_t* a_typeOut,
			bool& a_origCalled)
		{
			auto orig = BodyMorphProcessor_Process_Hook.GetOriginal();
			if (!a_root || !IsReadableFast(a_root, sizeof(void*))) {
				a_origCalled = true;
				orig(a_this, a_args, a_modelName, a_root, a_typeOut);
				return;
			}

			RE::NiNode* root = a_root->get();
			// Fast path: 99%+ of game NIFs do not carry BODYTRI extra data.
			if (!root || !IsReadableFast(root, sizeof(RE::NiObjectNET)) || !HasExtraDataFast(root, "BODYTRI")) {
				a_origCalled = true;
				orig(a_this, a_args, a_modelName, a_root, a_typeOut);
				return;
			}

			// Collect BSClothExtraData target mesh names and all BSTriShape nodes while CPU geometry is intact.
			std::unordered_set<std::string> directClothNames;
			std::vector<RE::BSTriShape*> triShapes;
			{
				std::vector<RE::NiAVObject*> stack;
				std::unordered_set<RE::NiAVObject*> visited;
				stack.reserve(32);
				stack.push_back(root);
				visited.insert(root);

				while (!stack.empty()) {
					RE::NiAVObject* cur = stack.back();
					stack.pop_back();
					if (!cur || !IsReadableFast(cur, sizeof(RE::NiObjectNET))) {
						continue;
					}

					CollectClothExtraTargetShapes(cur, directClothNames);

					if (RE::BSTriShape* ts = cur->IsTriShape()) {
						triShapes.push_back(ts);
					} else if (RE::NiNode* nd = cur->IsNode()) {
						const auto* childData = nd->children.data();
						const std::uint16_t cap = nd->children.capacity();
						if (childData && cap > 0 && IsReadableFast(childData, static_cast<std::size_t>(cap) * sizeof(void*))) {
							for (std::uint16_t i = 0; i < cap; ++i) {
								RE::NiAVObject* child = childData[i].get();
								if (child && visited.insert(child).second) {
									stack.push_back(child);
								}
							}
						}
					}
				}
			}

			if (triShapes.empty()) {
				a_origCalled = true;
				orig(a_this, a_args, a_modelName, a_root, a_typeOut);
				return;
			}

			std::vector<ShapeClothInfo> infos(triShapes.size());
			std::size_t primaryClothCount = 0;
			std::size_t nonSkinNonClothCount = 0;
			for (std::size_t i = 0; i < triShapes.size(); ++i) {
				InspectTriShape(triShapes[i], directClothNames, infos[i]);
				if (infos[i].isCloth) {
					++primaryClothCount;
				} else if (!infos[i].isSkin) {
					++nonSkinNonClothCount;
				}
			}

			// If no Havok cloth shapes exist in this NIF, run unmodified LooksMenu processing.
			if (primaryClothCount == 0) {
				a_origCalled = true;
				orig(a_this, a_args, a_modelName, a_root, a_typeOut);
				return;
			}

			// Compute transitive welded-seam closure (C*) across non-skin sibling partitions (>= 4 shared seam vertices)
			// so multi-partition 3BBB outfits (e.g. VTScientistFVariant.nif) never tear along cloth/torso seams.
			if (nonSkinNonClothCount > 0) {
				for (auto& info : infos) {
					if (!info.isSkin) {
						PopulateQuantizedVertices(info);
					}
				}

				bool changed = true;
				while (changed) {
					changed = false;
					for (std::size_t i = 0; i < infos.size(); ++i) {
						if (infos[i].isCloth || infos[i].isSkin || infos[i].quantizedVerts.size() < 4) {
							continue;
						}
						for (std::size_t j = 0; j < infos.size(); ++j) {
							if (!infos[j].isCloth || infos[j].quantizedVerts.size() < 4) {
								continue;
							}
							if (CountSortedIntersectionAtLeast(infos[i].quantizedVerts, infos[j].quantizedVerts, 4) >= 4) {
								infos[i].isCloth = true;
								changed = true;
								break;
							}
						}
					}
				}
			}

			// Let f4ee.dll attach MORPH_FILE / MORPH_SHAPE and chain TESProcessor::Process as normal.
			a_origCalled = true;
			orig(a_this, a_args, a_modelName, a_root, a_typeOut);

			// Strip MORPH_SHAPE and MORPH_FILE from all cloth / welded-cloth shapes (C*) so LooksMenu's
			// GetMorphableShapes / ApplyMorphsToShapes skips them and preserves Havok Cloth's dynamic vertex buffer.
			std::size_t strippedCount = 0;
			std::string strippedNames;
			for (auto& info : infos) {
				if (!info.isCloth || !info.shape) {
					continue;
				}
				const bool remShape = RemoveExtraDataByName(info.shape.get(), "MORPH_SHAPE");
				const bool remFile = RemoveExtraDataByName(info.shape.get(), "MORPH_FILE");
				if (remShape || remFile) {
					if (!strippedNames.empty()) {
						strippedNames += ", ";
					}
					strippedNames += info.name;
					++strippedCount;
				}
			}

			if (strippedCount > 0) {
				logger::info(
					"[ClothMorphFilter] Stripped body morph tags from {}/{} shape(s) in '{}' [{}]",
					strippedCount,
					infos.size(),
					a_modelName ? a_modelName : "<unnamed>",
					strippedNames);
			}
		}
	}

	Hooks<BodyMorphProcessor_Process_t> BodyMorphProcessor_Process_Hook{};

	void __fastcall Hooked_BodyMorphProcessor_Process(
		void* a_this,
		void* a_args,
		const char* a_modelName,
		RE::NiPointer<RE::NiNode>* a_root,
		std::uint32_t* a_typeOut)
	{
		bool origCalled = false;
		__try {
			ProcessBodyMorphWithClothFilter(a_this, a_args, a_modelName, a_root, a_typeOut, origCalled);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			logger::error(
				"[ClothMorphFilter] Exception in Hooked_BodyMorphProcessor_Process for '{}'",
				(a_modelName && IsReadableFast(a_modelName, 1)) ? a_modelName : "<unknown>");
			if (!origCalled) {
				auto orig = BodyMorphProcessor_Process_Hook.GetOriginal();
				if (orig) {
					CallOriginalSafe(orig, a_this, a_args, a_modelName, a_root, a_typeOut);
				}
			}
		}
	}

	bool InstallClothMorphFilter()
	{
		if (BodyMorphProcessor_Process_Hook.IsInstalled()) {
			return true;
		}

		HMODULE f4eeModule = GetModuleHandleA("f4ee.dll");
		if (!f4eeModule) {
			logger::warn("[ClothMorphFilter] f4ee.dll is not loaded; skipping BodyMorphProcessor::Process hook.");
			return false;
		}

		const auto targetAddr = reinterpret_cast<std::uintptr_t>(f4eeModule) + kF4EE_BodyMorphProcessor_Process_RVA;
		const auto* targetBytes = reinterpret_cast<const std::uint8_t*>(targetAddr);
		if (!IsReadableFast(targetBytes, sizeof(kExpectedPrologue))) {
			logger::error("[ClothMorphFilter] Target f4ee.dll+0x{:X} (0x{:X}) is not readable!", kF4EE_BodyMorphProcessor_Process_RVA, targetAddr);
			return false;
		}

		if (std::memcmp(targetBytes, kExpectedPrologue, sizeof(kExpectedPrologue)) != 0) {
			logger::warn(
				"[ClothMorphFilter] Prologue mismatch at f4ee.dll+0x{:X} (0x{:X}) — incompatible LooksMenu version or already detoured.",
				kF4EE_BodyMorphProcessor_Process_RVA,
				targetAddr);
			return false;
		}

		BodyMorphProcessor_Process_Hook.Set(targetAddr, Hooked_BodyMorphProcessor_Process);
		if (!BodyMorphProcessor_Process_Hook.Install()) {
			logger::error("[ClothMorphFilter] Failed to install DetourXS hook at f4ee.dll+0x{:X}", kF4EE_BodyMorphProcessor_Process_RVA);
			return false;
		}

		logger::info("[ClothMorphFilter] Installed Havok Cloth BodyMorphProcessor::Process filter at f4ee.dll+0x{:X} (0x{:X}).",
			kF4EE_BodyMorphProcessor_Process_RVA,
			targetAddr);
		return true;
	}
}
