import { useEffect, useRef, useState } from "react";

import {
    notifyUiReady,
    sendClose,
    sendPage,
    sendRefresh,
    subscribeCurrent,
    subscribePreview,
    subscribeShown,
    subscribeState,
    subscribeToast,
} from "./bridge";
import { ActorList } from "./components/ActorList";
import { OverridesPage } from "./components/OverridesPage";
import { PresetPanel } from "./components/PresetPanel";
import { SavedPage } from "./components/SavedPage";
import { SettingsPage } from "./components/SettingsPage";
import { Stage } from "./components/Stage";
import { useI18n } from "./i18n/useI18n";
import { useLanguageFont } from "./i18n/useLanguageFont";
import { isKeyCapture } from "./keyCapture";
import type { MenuState, ToastMessage } from "./types";

const TOAST_MS = 2600;
// The menu opens on a held key; its key-repeat must not close it right away: at least this long after
// opening, and for as long as the hold time asks (plus a margin).
const OPEN_KEY_GRACE_MS = 900;
const HOLD_MARGIN_MS = 600;

const PAGES = [
    { page: "main", label: "tab.presets" },
    { page: "saved", label: "tab.saved" },
    { page: "overrides", label: "tab.overrides" },
    { page: "settings", label: "tab.settings" },
] as const;

export function App() {
    const [state, setState] = useState<MenuState | null>(null);
    const [toast, setToast] = useState<(ToastMessage & { seq: number }) | null>(null);
    const page = useRef<MenuState["page"]>("main");
    page.current = state?.page ?? "main";
    const hotkey = useRef({ key: 111, holdSeconds: 0.3 });
    if (state) hotkey.current = { key: state.settings.key, holdSeconds: state.settings.holdSeconds };

    const t = useI18n(state?.strings);
    useLanguageFont(state?.language);

    useEffect(() => {
        let seq = 0;
        let shownAt = 0;
        subscribeState(setState);
        subscribeShown(() => {
            shownAt = Date.now();
        });
        subscribePreview((preview) => setState((s) => (s ? { ...s, preview } : s)));
        // The stored preset of the selected character changed: move the "current" mark.
        subscribeCurrent((current) =>
            setState((s) => (s ? { ...s, presets: s.presets.map((p) => ({ ...p, current: !!current.id && p.id === current.id })) } : s))
        );
        subscribeToast((message) => setToast({ ...message, seq: ++seq }));
        notifyUiReady();

        // Esc steps back (any other page -> main page -> closed); the key that opens the menu closes
        // it. Ultralight doesn't always report Escape as key === "Escape" -- also accept the legacy
        // "Esc" name and keyCode 27. While a new menu key is being picked every key belongs to that.
        const onKey = (e: KeyboardEvent) => {
            if (isKeyCapture()) return;
            const escape = e.key === "Escape" || e.key === "Esc" || e.keyCode === 27;
            if (escape) {
                if (page.current !== "main") sendPage("main");
                else sendClose();
                return;
            }
            if (e.keyCode !== hotkey.current.key || e.repeat) return;
            // A key that types text must not close the menu while a search box has the focus.
            const typing = e.target instanceof HTMLInputElement && e.target.type === "text";
            const printable = e.keyCode === 32 || (e.keyCode >= 48 && e.keyCode <= 90) || (e.keyCode >= 186 && e.keyCode <= 222);
            if (typing && printable) return;
            const grace = Math.max(OPEN_KEY_GRACE_MS, hotkey.current.holdSeconds * 1000 + HOLD_MARGIN_MS);
            if (Date.now() - shownAt > grace) sendClose();
        };
        window.addEventListener("keydown", onKey);
        return () => window.removeEventListener("keydown", onKey);
    }, []);

    useEffect(() => {
        if (!toast) return;
        const timer = window.setTimeout(() => setToast(null), TOAST_MS);
        return () => window.clearTimeout(timer);
    }, [toast]);

    if (!state) return null;

    const actor = state.actors.find((a) => a.ref === state.selected);

    return (
        <>
            {state.page === "saved" && state.preview.status === "ready" && <Stage />}
            <div className="panel">
                <header className="panel__header">
                    <h1 className="panel__title">{t("title")}</h1>
                    {state.page === "main" && (
                        <button className="btn btn--icon" title={t("refresh")} onClick={sendRefresh}>
                            <svg viewBox="0 0 16 16" width="16" height="16" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round">
                                <path d="M13.5 8a5.5 5.5 0 1 1-1.8-4.07" />
                                <path d="M13.6 2.2v3.3h-3.3" />
                            </svg>
                        </button>
                    )}
                    <button className="btn btn--icon" title={t("close")} onClick={sendClose}>
                        <svg viewBox="0 0 16 16" width="16" height="16" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round">
                            <path d="M3.5 3.5l9 9M12.5 3.5l-9 9" />
                        </svg>
                    </button>
                </header>
                <nav className="pages">
                    {PAGES.map((entry) => (
                        <button
                            key={entry.page}
                            className={state.page === entry.page ? "tab tab--on" : "tab"}
                            onClick={() => state.page !== entry.page && sendPage(entry.page)}
                        >
                            {t(entry.label)}
                        </button>
                    ))}
                </nav>
                <div className="panel__body">
                    {state.page === "saved" && <SavedPage state={state} t={t} />}
                    {state.page === "overrides" && <OverridesPage state={state} t={t} />}
                    {state.page === "settings" && <SettingsPage state={state} t={t} />}
                    {state.page === "main" && (
                        <>
                            <ActorList actors={state.actors} selected={state.selected} t={t} />
                            <PresetPanel state={state} actor={actor} t={t} />
                        </>
                    )}
                </div>
                <footer className="panel__footer">{t("footer.close", { key: state.settings.keyName })}</footer>
                {toast && (
                    <div key={toast.seq} className={toast.ok ? "toast toast--ok" : "toast toast--error"}>
                        {t(toast.key, { name: toast.name, n: toast.n })}
                    </div>
                )}
            </div>
        </>
    );
}
