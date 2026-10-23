// Typed wrapper around the PrismaUI JS<->C++ bridge.
//
// C++ -> JS: InteropCall invokes window.onState / onCurrent / onToast / onShown.
// JS -> C++: BindUIEvent injects window.dbr* / onUiReady; they don't exist outside the game
// (e.g. `vite dev`), so every call is guarded.
import type { CurrentPreset, MenuState, PreviewState, ToastMessage } from "./types";

type BoundFunction =
    | "dbrClose"
    | "dbrRefresh"
    | "dbrSelect"
    | "dbrSetType"
    | "dbrSetFilter"
    | "dbrHover"
    | "dbrPreview"
    | "dbrApply"
    | "dbrPage"
    | "dbrPreviewActor"
    | "dbrStopPreview"
    | "dbrApplyToCopy"
    | "dbrTransfer"
    | "dbrPrune"
    | "dbrForget"
    | "dbrRotate"
    | "dbrZoom"
    | "dbrLift"
    | "dbrOvToggleType"
    | "dbrOvAdd"
    | "dbrOvReset"
    | "dbrOvPreset"
    | "dbrOvResetAll"
    | "dbrSetHotkey"
    | "dbrResetHotkey"
    | "dbrSetLanguage";

declare global {
    interface Window {
        onState?: (json: string) => void;
        onCurrent?: (json: string) => void;
        onToast?: (json: string) => void;
        onShown?: (json: string) => void;
        onPreview?: (json: string) => void;
        onUiReady?: (json: string) => void;
    }
}

function subscribe<T>(name: "onState" | "onCurrent" | "onToast" | "onShown" | "onPreview", handler: (value: T) => void): void {
    window[name] = (json: string) => {
        try {
            handler(JSON.parse(json) as T);
        } catch (err) {
            console.error(`${name}: failed to parse payload`, err);
        }
    };
}

export const subscribeState = (handler: (state: MenuState) => void) => subscribe("onState", handler);
export const subscribeCurrent = (handler: (current: CurrentPreset) => void) => subscribe("onCurrent", handler);
export const subscribeToast = (handler: (toast: ToastMessage) => void) => subscribe("onToast", handler);
export const subscribePreview = (handler: (preview: PreviewState) => void) => subscribe("onPreview", handler);
// The menu was just opened (the key that opens it may still be held down).
export const subscribeShown = (handler: () => void) => subscribe<object>("onShown", () => handler());

// Tells C++ the receivers above exist, so it can (re)push state. DOM-ready fires before React's
// first useEffect, and pushes made then are dropped. onUiReady itself is bound by C++ only after
// DOM-ready, hence the poll.
export function notifyUiReady(): void {
    const deadline = Date.now() + 15000;
    const attempt = () => {
        if (window.onUiReady) {
            window.onUiReady("{}");
            return;
        }
        if (Date.now() < deadline) window.setTimeout(attempt, 100);
    };
    attempt();
}

// Calls a C++-bound function as a plain method call on window. NOT `window.fn?.(arg)`: the
// bundler compiles optional calls to `fn.call(window, arg)`, and the functions PrismaUI
// (Ultralight) injects via BindUIEvent are native objects with no Function.prototype.call.
function invoke(name: BoundFunction, payload: object = {}): void {
    const w = window as unknown as Record<string, ((json: string) => void) | undefined>;
    if (typeof w[name] !== "undefined") {
        w[name]!(JSON.stringify(payload));
    }
}

export const sendClose = () => invoke("dbrClose");
export const sendRefresh = () => invoke("dbrRefresh");
export const sendSelect = (ref: string) => invoke("dbrSelect", { ref });
export const sendSetType = (type: string) => invoke("dbrSetType", { type });
export const sendSetFilter = (id: string, on: boolean) => invoke("dbrSetFilter", { id, on });
export const sendHover = (ref: string) => invoke("dbrHover", { ref }); // "" = back to the selected one
export const sendPreview = (id: string) => invoke("dbrPreview", { id });
export const sendApply = (id: string) => invoke("dbrApply", { id });
export const sendPage = (page: "main" | "saved" | "overrides" | "settings") => invoke("dbrPage", { page });
export const sendPreviewActor = (ref: string) => invoke("dbrPreviewActor", { ref });
export const sendStopPreview = () => invoke("dbrStopPreview");
export const sendApplyToCopy = (id: string) => invoke("dbrApplyToCopy", { id });
export const sendTransfer = () => invoke("dbrTransfer");
export const sendPrune = () => invoke("dbrPrune");
export const sendForget = (ref: string, name: string) => invoke("dbrForget", { ref, name });
export const sendRotate = (degrees: number) => invoke("dbrRotate", { value: degrees });
export const sendZoom = (steps: number) => invoke("dbrZoom", { value: steps });
export const sendLift = (units: number) => invoke("dbrLift", { value: units });
export const sendOvToggleType = (key: string, type: string) => invoke("dbrOvToggleType", { key, type });
export const sendOvAdd = (ref: string) => invoke("dbrOvAdd", { ref });
export const sendOvReset = (key: string) => invoke("dbrOvReset", { key });
export const sendOvPreset = (id: string, disabled: boolean) => invoke("dbrOvPreset", { id, disabled });
export const sendOvResetAll = () => invoke("dbrOvResetAll");
export const sendSetHotkey = (change: { key?: number; holdSeconds?: number }) => invoke("dbrSetHotkey", change);
export const sendResetHotkey = () => invoke("dbrResetHotkey");
export const sendSetLanguage = (language: string) => invoke("dbrSetLanguage", { language });
