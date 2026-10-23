import { useMemo, useState } from "react";

import { sendApplyToCopy, sendSetFilter, sendSetType, sendTransfer } from "../bridge";
import type { Translate } from "../i18n/useI18n";
import type { MenuState } from "../types";

interface Props {
    state: MenuState;
    t: Translate;
}

// Presets that fit the previewed character's original: a click puts one on the copy (to see how
// it looks), the Transfer button applies everything tried on the copy to the original and stores it.
export function CopyEditor({ state, t }: Props) {
    const [query, setQuery] = useState("");
    const { pending } = state.preview;

    const shown = useMemo(() => {
        const needle = query.trim().toLowerCase();
        return needle ? state.presets.filter((p) => p.id.toLowerCase().includes(needle)) : state.presets;
    }, [state.presets, query]);

    const onCopy = (id: string) => pending.some((p) => p.type === state.type && p.id === id);

    return (
        <section className="editor">
            <div className="section__head">
                <h2 className="section__title">{t("saved.editCopy")}</h2>
                <span className="section__count">{t("presets.count", { n: state.presets.length })}</span>
            </div>

            <div className="tabs">
                {state.types.map((type) => (
                    <button key={type} className={type === state.type ? "tab tab--on" : "tab"} onClick={() => sendSetType(type)}>
                        {t(`type.${type}`)}
                        {pending.some((p) => p.type === type) ? " •" : ""}
                    </button>
                ))}
            </div>

            <div className="filters">
                <span className="filters__title">{t("filters.title")}</span>
                {state.filters.map((filter) => (
                    <button key={filter.id} className={filter.on ? "chip chip--on" : "chip"} onClick={() => sendSetFilter(filter.id, !filter.on)}>
                        {t(`filter.${filter.id}`)}
                    </button>
                ))}
            </div>

            <input className="input" type="text" placeholder={t("presets.search")} value={query} onChange={(e) => setQuery(e.target.value)} />

            <div className="list">
                {state.excluded && <div className="note note--warn">{t("presets.excluded")}</div>}
                {!state.excluded && state.presets.length === 0 && <div className="note">{t("presets.empty")}</div>}
                {shown.map((preset) => {
                    const classes = ["row"];
                    if (preset.current) classes.push("row--current");
                    if (onCopy(preset.id)) classes.push("row--preview");
                    return (
                        <button key={preset.id} className={classes.join(" ")} onClick={() => sendApplyToCopy(preset.id)}>
                            <span className="row__main">
                                <span className="row__name">{preset.id}</span>
                            </span>
                            {onCopy(preset.id) && <span className="tag tag--warn">{t("saved.onCopy")}</span>}
                            {preset.current && <span className="tag">{t("presets.current")}</span>}
                        </button>
                    );
                })}
            </div>

            <div className="editor__bar">
                <span className="editor__pending">{t("saved.pending", { n: pending.length })}</span>
                <button className="btn btn--accent" disabled={pending.length === 0} onClick={sendTransfer}>
                    {t("saved.transfer")}
                </button>
            </div>
        </section>
    );
}
