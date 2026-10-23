import { useMemo, useState } from "react";

import { sendForget, sendPrune, sendPreviewActor, sendStopPreview } from "../bridge";
import type { Translate } from "../i18n/useI18n";
import type { MenuState } from "../types";
import { CopyEditor } from "./CopyEditor";

interface Props {
    state: MenuState;
    t: Translate;
}

// Characters the mod stores presets for. A preview (an exact copy of the character with their
// equipment and everything LooksMenu holds for them, standing in the world) is built only on
// the Preview button or a double click -- never on hover or a single click. Characters that
// aren't loaded (or aren't in memory at all) can be previewed too: their stored presets are put
// on the copy.
export function SavedPage({ state, t }: Props) {
    const [query, setQuery] = useState("");
    const [picked, setPicked] = useState("");

    const shown = useMemo(() => {
        const needle = query.trim().toLowerCase();
        if (!needle) return state.saved;
        return state.saved.filter(
            (a) =>
                a.name.toLowerCase().includes(needle) ||
                a.ref.toLowerCase().includes(needle) ||
                a.presets.some((p) => p.id.toLowerCase().includes(needle))
        );
    }, [state.saved, query]);

    const { ref: previewRef, status, editable } = state.preview;
    const active = previewRef !== "" && status !== "none";
    const ready = active && status === "ready";

    return (
        <section className={ready && editable ? "section section--saved section--split" : "section section--saved"}>
            {active && (
                <div className={status === "failed" ? "status status--error" : "status"}>
                    <span className="status__text">
                        {status === "loading" && t("saved.loading")}
                        {status === "ready" && t("saved.ready")}
                        {status === "failed" && t("saved.failed")}
                    </span>
                    <button className="btn" onClick={sendStopPreview}>
                        {t("saved.stop")}
                    </button>
                </div>
            )}

            {ready && editable && <CopyEditor state={state} t={t} />}
            {ready && !editable && <div className="note note--warn">{t("saved.noOriginal")}</div>}

            <div className="section__head">
                <h2 className="section__title">{t("saved.title")}</h2>
                <span className="section__count">{t("saved.count", { n: state.saved.length })}</span>
                <button className="btn btn--small" onClick={sendPrune}>
                    {t("saved.cleanup")}
                </button>
            </div>
            <input className="input" type="text" placeholder={t("saved.search")} value={query} onChange={(e) => setQuery(e.target.value)} />

            <div className="list list--saved">
                {state.saved.length === 0 && <div className="note">{t("saved.empty")}</div>}
                {state.saved.length > 0 && shown.length === 0 && <div className="note">{t("actors.noMatch")}</div>}
                {shown.map((actor) => {
                    const classes = ["saved"];
                    if (actor.ref === picked) classes.push("saved--picked");
                    if (actor.ref === previewRef && status !== "none") classes.push("saved--preview");
                    if (actor.status === "missing") classes.push("saved--dim");
                    return (
                        <div
                            key={actor.ref}
                            className={classes.join(" ")}
                            onClick={() => setPicked(actor.ref)}
                            onDoubleClick={() => actor.previewable && sendPreviewActor(actor.ref)}
                        >
                            <div className="saved__head">
                                <span className="saved__main">
                                    <span className="row__name">{actor.name || t("actors.unnamed")}</span>
                                    <span className="row__sub">
                                        {actor.ref}
                                        {actor.status === "loaded" ? ` · ${t("metres", { n: actor.distance })}` : ""}
                                    </span>
                                </span>
                                {actor.status === "unloaded" && <span className="tag tag--dim">{t("saved.notLoaded")}</span>}
                                {actor.status === "missing" && <span className="tag tag--dim">{t("saved.missing")}</span>}
                                {actor.status === "missing" && (
                                    <button
                                        className="btn btn--small"
                                        onClick={(e) => {
                                            e.stopPropagation();
                                            sendForget(actor.ref, actor.name);
                                        }}
                                    >
                                        {t("saved.forget")}
                                    </button>
                                )}
                                <button
                                    className="btn"
                                    disabled={!actor.previewable}
                                    onClick={(e) => {
                                        e.stopPropagation();
                                        sendPreviewActor(actor.ref);
                                    }}
                                >
                                    {t("saved.preview")}
                                </button>
                            </div>
                            <div className="saved__presets">
                                {actor.presets.map((preset) => (
                                    <span key={`${preset.type}:${preset.id}`} className="chip chip--static" title={preset.id}>
                                        <span className="chip__type">{t(`type.${preset.type}`)}</span> {preset.id}
                                    </span>
                                ))}
                            </div>
                        </div>
                    );
                })}
            </div>
            <div className="hint">{t("saved.hint")}</div>
            <div className="hint">{t("saved.note")}</div>
        </section>
    );
}
