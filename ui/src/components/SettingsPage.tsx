import { useEffect, useState } from "react";

import { sendResetHotkey, sendSetHotkey, sendSetLanguage } from "../bridge";
import type { Translate } from "../i18n/useI18n";
import { setKeyCapture } from "../keyCapture";
import type { MenuState } from "../types";

interface Props {
    state: MenuState;
    t: Translate;
}

// The key that opens the menu and how long it must be held (0 = at once). The game side keeps
// them in the mod's ini; they apply to the classic menu too.
export function SettingsPage({ state, t }: Props) {
    const { settings } = state;
    const [capturing, setCapturing] = useState(false);
    const [hold, setHold] = useState(String(settings.holdSeconds));

    useEffect(() => setHold(String(settings.holdSeconds)), [settings.holdSeconds]);

    // Waiting for a key: Esc cancels, any other key becomes the menu key (the game checks it).
    useEffect(() => {
        if (!capturing) return;
        setKeyCapture(true);
        const onKey = (e: KeyboardEvent) => {
            e.preventDefault();
            e.stopPropagation();
            if (e.repeat) return;
            const escape = e.key === "Escape" || e.key === "Esc" || e.keyCode === 27;
            if (!escape) sendSetHotkey({ key: e.keyCode });
            setCapturing(false);
        };
        window.addEventListener("keydown", onKey, true);
        return () => {
            window.removeEventListener("keydown", onKey, true);
            // After the event that ended the capture has been handled everywhere.
            window.setTimeout(() => setKeyCapture(false), 0);
        };
    }, [capturing]);

    const commitHold = () => {
        const value = parseFloat(hold.replace(",", "."));
        if (Number.isNaN(value)) {
            setHold(String(settings.holdSeconds));
            return;
        }
        const clamped = Math.min(Math.max(value, 0), settings.maxHoldSeconds);
        setHold(String(clamped));
        if (clamped !== settings.holdSeconds) sendSetHotkey({ holdSeconds: clamped });
    };

    const gameName = settings.languages.find((l) => l.id === settings.gameLanguage)?.name ?? settings.gameLanguage;

    return (
        <section className="section section--settings">
            <div className="setting setting--stack">
                <span className="setting__label">{t("set.language")}</span>
                <span className="langs">
                    <button className={settings.language === "auto" ? "chip chip--on" : "chip"} onClick={() => sendSetLanguage("auto")}>
                        {t("set.language.auto")} ({gameName})
                    </button>
                    {settings.languages.map((language) => (
                        <button
                            key={language.id}
                            className={settings.language === language.id ? "chip chip--on" : "chip"}
                            onClick={() => sendSetLanguage(language.id)}
                        >
                            {language.name}
                        </button>
                    ))}
                </span>
            </div>
            <div className="hint">{t("set.language.hint")}</div>

            <div className="setting">
                <span className="setting__label">{t("set.key")}</span>
                <span className="setting__control">
                    <span className={capturing ? "keycap keycap--wait" : "keycap"}>{capturing ? t("set.press") : settings.keyName}</span>
                    <button className="btn" onClick={() => setCapturing(!capturing)}>
                        {capturing ? t("ov.add.cancel") : t("set.change")}
                    </button>
                </span>
            </div>

            <div className="setting">
                <span className="setting__label">{t("set.hold")}</span>
                <span className="setting__control">
                    <input
                        className="input input--short"
                        type="text"
                        inputMode="decimal"
                        value={hold}
                        onChange={(e) => setHold(e.target.value)}
                        onBlur={commitHold}
                        onKeyDown={(e) => {
                            if (e.key === "Enter") commitHold();
                        }}
                    />
                </span>
            </div>
            <div className="hint">{t("set.hold.hint")}</div>

            <div className="editor__bar">
                <span />
                <button className="btn" onClick={sendResetHotkey}>
                    {t("set.default")}
                </button>
            </div>
            <div className="hint">{t("set.note")}</div>
        </section>
    );
}
