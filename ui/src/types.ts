// Wire format of the C++ -> JS pushes (src: Sources/PrismaMenu/PrismaMenu.cpp).

export interface Actor {
    ref: string; // "0x0001A2B3"
    name: string; // "" = unnamed
    distance: number; // metres
    female: boolean;
}

export interface Preset {
    id: string;
    current: boolean; // the preset stored for the selected character and type
}

export interface FilterState {
    id: string; // HasKeyword, HasNotKeyword, InFaction, NotInFaction, FormIDs, NotFormIDs
    on: boolean;
}

export interface SavedPreset {
    type: string; // BodyMorphs, BodyHairs, Head, BodyTattoos, Nails
    id: string;
}

// A character the mod stores presets for (page "saved").
export interface SavedActor {
    ref: string;
    name: string;
    status: "loaded" | "unloaded" | "missing"; // in the scene / known to the game but not loaded / not in memory at all
    previewable: boolean;
    distance: number; // metres, -1 when not loaded
    presets: SavedPreset[];
}

export type PreviewStatus = "none" | "loading" | "ready" | "failed";

export interface PreviewState {
    ref: string; // "" = no preview
    status: PreviewStatus;
    editable: boolean; // the original is in memory: presets can be tried on the copy and transferred
    pending: SavedPreset[]; // applied to the copy, not yet transferred to the original
}

// One exclusion rule: from a JSON file or added in this save; with this save's overrides.
export interface ExclusionRow {
    key: string;
    name: string;
    kind: "actor" | "npc" | "unknown";
    source: "json" | "session";
    base: string[]; // preset types the JSON rule excludes
    allow: string[]; // ... allowed again in this save
    deny: string[]; // ... excluded only in this save
    effective: string[]; // what is excluded now
}

export interface OverridesData {
    exclusions: ExclusionRow[];
    presets: { id: string; disabled: boolean }[]; // presets of the current type
    disabledTotal: number;
    anyOverride: boolean;
}

// The key that opens the menu and how long it must be held (shared by the Prisma UI and the classic menu).
export interface HotkeySettings {
    key: number; // button code (Windows VK_* for the keyboard)
    keyName: string;
    holdSeconds: number; // 0 = opens at once
    defaultKey: number;
    defaultKeyName: string;
    defaultHoldSeconds: number;
    maxHoldSeconds: number;
    language: string; // "auto" = the game's language, else a locale id forced by the player
    gameLanguage: string; // the locale "auto" resolves to
    languages: { id: string; name: string }[]; // each in its own language
}

export interface MenuState {
    page: "main" | "saved" | "overrides" | "settings";
    settings: HotkeySettings;
    saved: SavedActor[];
    overrides: OverridesData;
    preview: PreviewState;
    language: string;
    strings: Record<string, string>;
    actors: Actor[];
    selected: string; // "" = nobody
    types: string[]; // BodyMorphs, BodyHairs, Head, BodyTattoos, Nails
    type: string;
    filters: FilterState[];
    presets: Preset[];
    excluded: boolean; // the selected type is excluded for the selected character
}

export interface CurrentPreset {
    id?: string;
}

export interface ToastMessage {
    ok: boolean;
    key: string; // translation key: toast.applied, toast.error, ...
    name: string; // {name}
    n: number; // {n}
}
