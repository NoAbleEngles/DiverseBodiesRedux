import { useMemo, useState } from "react";

import { sendOvAdd, sendOvPreset, sendOvReset, sendOvResetAll, sendOvToggleType, sendSetType } from "../bridge";
import type { Translate } from "../i18n/useI18n";
import type { ExclusionRow, MenuState } from "../types";

interface Props {
    state: MenuState;
    t: Translate;
}

// Overrides of rules from the JSON files, for this save only (stored in the save, the files stay
// untouched): exclusions can be cancelled per preset type or added for a character, and a preset
// can be switched off for the automatic distribution.
export function OverridesPage({ state, t }: Props) {
    const { overrides } = state;
    const [adding, setAdding] = useState(false);
    const [query, setQuery] = useState("");
    // Exclusion filters: text (name or ID), excluded preset types (all of them / any of them), changed only.
    const [ruleQuery, setRuleQuery] = useState("");
    const [typeFilter, setTypeFilter] = useState<string[]>([]);
    const [matchAll, setMatchAll] = useState(true);
    const [changedOnly, setChangedOnly] = useState(false);

    const presets = useMemo(() => {
        const needle = query.trim().toLowerCase();
        return needle ? overrides.presets.filter((p) => p.id.toLowerCase().includes(needle)) : overrides.presets;
    }, [overrides.presets, query]);

    const touched = (row: ExclusionRow) => row.allow.length > 0 || row.deny.length > 0;

    // "0x00000805", "0x805" and "805" are the same ID: compare without the prefix and leading zeros.
    const normalizeId = (text: string) => text.toLowerCase().replace(/^0x/, "").replace(/^0+(?=.)/, "");

    const rules = useMemo(() => {
        const needle = ruleQuery.trim().toLowerCase();
        const idNeedle = normalizeId(needle);
        return overrides.exclusions.filter((row) => {
            if (changedOnly && !touched(row)) return false;
            if (typeFilter.length > 0) {
                const test = (type: string) => row.effective.includes(type);
                if (!(matchAll ? typeFilter.every(test) : typeFilter.some(test))) return false;
            }
            if (!needle) return true;
            return row.name.toLowerCase().includes(needle) || row.key.toLowerCase().includes(needle) || normalizeId(row.key).includes(idNeedle);
        });
    }, [overrides.exclusions, ruleQuery, typeFilter, matchAll, changedOnly]);

    const toggleType = (type: string) => setTypeFilter((current) => (current.includes(type) ? current.filter((x) => x !== type) : [...current, type]));
    const filtering = ruleQuery.trim() !== "" || typeFilter.length > 0 || changedOnly;

    return (
        <section className="section section--overrides">
            <div className="hint hint--top">{t("ov.intro")}</div>

            <div className="section__head">
                <h2 className="section__title">{t("ov.exclusions")}</h2>
                <span className="section__count">{filtering ? `${rules.length} / ${overrides.exclusions.length}` : overrides.exclusions.length}</span>
                <button className="btn btn--small" onClick={() => setAdding(!adding)}>
                    {adding ? t("ov.add.cancel") : t("ov.add")}
                </button>
            </div>

            {adding && (
                <div className="list list--picker">
                    {state.actors.length === 0 && <div className="note">{t("actors.empty")}</div>}
                    {state.actors.map((actor) => (
                        <button
                            key={actor.ref}
                            className="row"
                            onClick={() => {
                                sendOvAdd(actor.ref);
                                setAdding(false);
                            }}
                        >
                            <span className={actor.female ? "dot dot--female" : "dot dot--male"} />
                            <span className="row__main">
                                <span className="row__name">{actor.name || t("actors.unnamed")}</span>
                                <span className="row__sub">{actor.ref}</span>
                            </span>
                        </button>
                    ))}
                </div>
            )}

            <input className="input" type="text" placeholder={t("ov.search")} value={ruleQuery} onChange={(e) => setRuleQuery(e.target.value)} />
            <div className="filters">
                <span className="filters__title">{t("ov.filter.types")}</span>
                {state.types.map((type) => (
                    <button key={type} className={typeFilter.includes(type) ? "chip chip--excluded" : "chip"} onClick={() => toggleType(type)}>
                        {t(`type.${type}`)}
                    </button>
                ))}
                <button
                    className="chip chip--mode"
                    disabled={typeFilter.length < 2}
                    title={t(matchAll ? "ov.filter.all.tip" : "ov.filter.any.tip")}
                    onClick={() => setMatchAll(!matchAll)}
                >
                    {t(matchAll ? "ov.filter.all" : "ov.filter.any")}
                </button>
                <button className={changedOnly ? "chip chip--on" : "chip"} onClick={() => setChangedOnly(!changedOnly)}>
                    {t("ov.filter.changed")}
                </button>
            </div>

            <div className="list list--rules">
                {overrides.exclusions.length === 0 && <div className="note">{t("ov.exclusions.empty")}</div>}
                {overrides.exclusions.length > 0 && rules.length === 0 && <div className="note">{t("actors.noMatch")}</div>}
                {rules.map((row) => (
                    <div key={row.key} className="rule">
                        <div className="rule__head">
                            <span className="rule__main">
                                <span className="row__name">{row.name || row.key}</span>
                                <span className="row__sub">
                                    {row.key} · {t(row.source === "json" ? "ov.source.json" : "ov.source.session")}
                                </span>
                            </span>
                            {touched(row) && (
                                <button className="btn btn--small" onClick={() => sendOvReset(row.key)}>
                                    {t("ov.reset")}
                                </button>
                            )}
                        </div>
                        <div className="rule__types">
                            {state.types.map((type) => {
                                const excluded = row.effective.includes(type);
                                const changed = row.allow.includes(type) || row.deny.includes(type);
                                const classes = ["chip", excluded ? "chip--excluded" : "chip--open"];
                                if (changed) classes.push("chip--changed");
                                return (
                                    <button key={type} className={classes.join(" ")} onClick={() => sendOvToggleType(row.key, type)}>
                                        {t(`type.${type}`)}
                                        {changed ? ` · ${t("ov.overridden")}` : ""}
                                    </button>
                                );
                            })}
                        </div>
                    </div>
                ))}
            </div>
            <div className="hint">{t("ov.legend")}</div>

            <div className="section__head section__head--gap">
                <h2 className="section__title">{t("ov.presets")}</h2>
                <span className="section__count">{overrides.disabledTotal}</span>
            </div>
            <div className="tabs">
                {state.types.map((type) => (
                    <button key={type} className={type === state.type ? "tab tab--on" : "tab"} onClick={() => sendSetType(type)}>
                        {t(`type.${type}`)}
                    </button>
                ))}
            </div>
            <input className="input" type="text" placeholder={t("presets.search")} value={query} onChange={(e) => setQuery(e.target.value)} />
            <div className="list list--presets">
                {presets.map((preset) => (
                    <button
                        key={preset.id}
                        className={preset.disabled ? "row row--off" : "row"}
                        onClick={() => sendOvPreset(preset.id, !preset.disabled)}
                    >
                        <span className="row__main">
                            <span className="row__name">{preset.id}</span>
                        </span>
                        <span className={preset.disabled ? "tag tag--off" : "tag"}>{t(preset.disabled ? "ov.off" : "ov.on")}</span>
                    </button>
                ))}
            </div>
            <div className="hint">{t("ov.presets.hint")}</div>

            <div className="editor__bar">
                <span />
                <button className="btn" disabled={!overrides.anyOverride} onClick={sendOvResetAll}>
                    {t("ov.resetAll")}
                </button>
            </div>
        </section>
    );
}
