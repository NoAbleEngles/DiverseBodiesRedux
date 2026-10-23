import { useCallback } from "react";

import { EN_STRINGS } from "./en";

// t(key, vars): active locale string, else built-in English, else the key itself.
// {name} placeholders are replaced from vars.
export function useI18n(strings: Record<string, string> | undefined) {
    return useCallback(
        (key: string, vars?: Record<string, string | number>): string => {
            let text = strings?.[key] ?? EN_STRINGS[key] ?? key;
            if (vars) {
                for (const [name, value] of Object.entries(vars)) {
                    text = text.split(`{${name}}`).join(String(value));
                }
            }
            return text;
        },
        [strings]
    );
}

export type Translate = ReturnType<typeof useI18n>;
