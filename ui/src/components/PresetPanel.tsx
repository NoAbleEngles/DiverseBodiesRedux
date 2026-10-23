import { useEffect, useMemo, useRef, useState } from "react";

import { sendApply, sendPreview, sendSetFilter, sendSetType } from "../bridge";
import type { Translate } from "../i18n/useI18n";
import type { Actor, MenuState } from "../types";

interface Props {
    state: MenuState;
    actor: Actor | undefined;
    t: Translate;
}

const PREVIEW_DELAY_MS = 450;

// Presets that fit the selected character: type tabs, match filters, the list. Hovering a
// preset for a moment shows it on the character (not saved); a click applies and saves it.
export function PresetPanel({ state, actor, t }: Props) {
    const [query, setQuery] = useState("");
    const [hoverPreview, setHoverPreview] = useState(true);
    const [previewId, setPreviewId] = useState<string | null>(null);
    const timer = useRef<number | undefined>(undefined);

    // A different character or type: whatever was previewed no longer applies to the list.
    useEffect(() => {
        setPreviewId(null);
        window.clearTimeout(timer.current);
    }, [state.selected, state.type]);
    useEffect(() => () => window.clearTimeout(timer.current), []);

    const shown = useMemo(() => {
        const needle = query.trim().toLowerCase();
        return needle ? state.presets.filter((p) => p.id.toLowerCase().includes(needle)) : state.presets;
    }, [state.presets, query]);

    const enter = (id: string) => {
        window.clearTimeout(timer.current);
        if (!hoverPreview) return;
        timer.current = window.setTimeout(() => {
            sendPreview(id);
            setPreviewId(id);
        }, PREVIEW_DELAY_MS);
    };
    const leave = () => window.clearTimeout(timer.current);
    const apply = (id: string) => {
        window.clearTimeout(timer.current);
        setPreviewId(null);
        sendApply(id);
    };

    if (!actor) {
        return (
            <section className="section section--presets">
                <div className="note note--center">{t("presets.pick")}</div>
            </section>
        );
    }

    return (
        <section className="section section--presets">
            <div className="card">
                <span className={actor.female ? "dot dot--female" : "dot dot--male"} />
                <span className="card__main">
                    <span className="card__name">{actor.name || t("actors.unnamed")}</span>
                    <span className="card__sub">
                        {actor.ref} · {t(actor.female ? "sex.female" : "sex.male")} · {t("metres", { n: actor.distance })}
                    </span>
                </span>
            </div>

            <div className="tabs">
                {state.types.map((type) => (
                    <button key={type} className={type === state.type ? "tab tab--on" : "tab"} onClick={() => sendSetType(type)}>
                        {t(`type.${type}`)}
                    </button>
                ))}
            </div>

            <div className="filters">
                <span className="filters__title">{t("filters.title")}</span>
                {state.filters.map((filter) => (
                    <button
                        key={filter.id}
                        className={filter.on ? "chip chip--on" : "chip"}
                        onClick={() => sendSetFilter(filter.id, !filter.on)}
                    >
                        {t(`filter.${filter.id}`)}
                    </button>
                ))}
            </div>

            <div className="section__head">
                <h2 className="section__title">{t("presets.title")}</h2>
                <span className="section__count">{t("presets.count", { n: state.presets.length })}</span>
            </div>
            <input
                className="input"
                type="text"
                placeholder={t("presets.search")}
                value={query}
                onChange={(e) => setQuery(e.target.value)}
            />

            <div className="list" onMouseLeave={leave}>
                {state.excluded && <div className="note note--warn">{t("presets.excluded")}</div>}
                {!state.excluded && state.presets.length === 0 && <div className="note">{t("presets.empty")}</div>}
                {!state.excluded && state.presets.length > 0 && shown.length === 0 && <div className="note">{t("actors.noMatch")}</div>}
                {shown.map((preset) => {
                    const classes = ["row"];
                    if (preset.current) classes.push("row--current");
                    if (preset.id === previewId) classes.push("row--preview");
                    return (
                        <button
                            key={preset.id}
                            className={classes.join(" ")}
                            onMouseEnter={() => enter(preset.id)}
                            onClick={() => apply(preset.id)}
                        >
                            <span className="row__main">
                                <span className="row__name">{preset.id}</span>
                                {preset.id === previewId && <span className="row__sub row__sub--warn">{t("presets.previewing")}</span>}
                            </span>
                            {preset.current && <span className="tag">{t("presets.current")}</span>}
                        </button>
                    );
                })}
            </div>

            <label className="toggle">
                <input type="checkbox" checked={hoverPreview} onChange={(e) => setHoverPreview(e.target.checked)} />
                <span>{t("presets.hoverPreview")}</span>
            </label>
            <div className="hint">{t("presets.hint")}</div>
        </section>
    );
}
