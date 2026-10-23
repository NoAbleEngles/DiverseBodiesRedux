import { useEffect, useRef } from "react";

import { sendLift, sendRotate, sendZoom } from "../bridge";

const DEGREES_PER_PIXEL = 0.6;
const UNITS_PER_PIXEL = 0.6;
const SEND_INTERVAL_MS = 40;

// The transparent area left of the panel, where the preview copy stands: dragging left-right
// turns the copy, up-down lifts or lowers it (the copy has no collision, so it can hang in the
// air), the wheel moves the camera closer or further. Mounted only while a copy is ready.
export function Stage() {
    const dragging = useRef(false);
    const last = useRef({ x: 0, y: 0 });
    const pending = useRef({ turn: 0, lift: 0 });
    const timer = useRef<number | undefined>(undefined);

    useEffect(() => () => window.clearTimeout(timer.current), []);

    const flush = () => {
        timer.current = undefined;
        const { turn, lift } = pending.current;
        pending.current = { turn: 0, lift: 0 };
        if (turn !== 0) sendRotate(turn);
        if (lift !== 0) sendLift(lift);
    };

    return (
        <div
            className="stage"
            onMouseDown={(e) => {
                dragging.current = true;
                last.current = { x: e.clientX, y: e.clientY };
            }}
            onMouseMove={(e) => {
                if (!dragging.current) return;
                pending.current.turn += (e.clientX - last.current.x) * DEGREES_PER_PIXEL;
                pending.current.lift -= (e.clientY - last.current.y) * UNITS_PER_PIXEL; // dragging up lifts
                last.current = { x: e.clientX, y: e.clientY };
                // Throttled: every mouse event would be a call into the game.
                if (timer.current === undefined) timer.current = window.setTimeout(flush, SEND_INTERVAL_MS);
            }}
            onMouseUp={() => (dragging.current = false)}
            onMouseLeave={() => (dragging.current = false)}
            onWheel={(e) => sendZoom(e.deltaY < 0 ? 1 : -1)}
        />
    );
}
