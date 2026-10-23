import { useEffect, useMemo, useRef, useState } from "react";

import { sendHover, sendSelect } from "../bridge";
import type { Translate } from "../i18n/useI18n";
import type { Actor } from "../types";

interface Props {
    actors: Actor[];
    selected: string;
    t: Translate;
}

// Hovering a row makes C++ put the highlight shader on that character in the world; leaving the
// list (or clicking a row: it is chosen then) takes it off. The leave is delayed a little so
// moving between two rows doesn't flicker the shader off and on.
export function ActorList({ actors, selected, t }: Props) {
    const [query, setQuery] = useState("");
    const leaveTimer = useRef<number | undefined>(undefined);

    useEffect(() => () => window.clearTimeout(leaveTimer.current), []);

    const shown = useMemo(() => {
        const needle = query.trim().toLowerCase();
        if (!needle) return actors;
        return actors.filter((a) => a.name.toLowerCase().includes(needle) || a.ref.toLowerCase().includes(needle));
    }, [actors, query]);

    const enter = (ref: string) => {
        window.clearTimeout(leaveTimer.current);
        sendHover(ref);
    };
    const leave = () => {
        window.clearTimeout(leaveTimer.current);
        leaveTimer.current = window.setTimeout(() => sendHover(""), 120);
    };

    return (
        <section className="section section--actors">
            <div className="section__head">
                <h2 className="section__title">{t("actors.title")}</h2>
                <span className="section__count">{actors.length}</span>
            </div>
            <input
                className="input"
                type="text"
                placeholder={t("actors.search")}
                value={query}
                onChange={(e) => setQuery(e.target.value)}
            />
            <div className="list" onMouseLeave={leave}>
                {actors.length === 0 && <div className="note">{t("actors.empty")}</div>}
                {actors.length > 0 && shown.length === 0 && <div className="note">{t("actors.noMatch")}</div>}
                {shown.map((actor) => (
                    <button
                        key={actor.ref}
                        className={actor.ref === selected ? "row row--selected" : "row"}
                        onMouseEnter={() => enter(actor.ref)}
                        onClick={() => sendSelect(actor.ref)}
                    >
                        <span className={actor.female ? "dot dot--female" : "dot dot--male"} title={t(actor.female ? "sex.female" : "sex.male")} />
                        <span className="row__main">
                            <span className="row__name">{actor.name || t("actors.unnamed")}</span>
                            <span className="row__sub">{actor.ref}</span>
                        </span>
                        <span className="row__side">{t("metres", { n: actor.distance })}</span>
                    </button>
                ))}
            </div>
            <div className="hint">{t("actors.hint")}</div>
        </section>
    );
}
