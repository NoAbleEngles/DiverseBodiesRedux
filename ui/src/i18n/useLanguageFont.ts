import { useEffect } from "react";

// A CJK UI language gets its Windows system font first; the default stack always follows, so
// glyphs it lacks (Latin, Cyrillic...) still render.
const FALLBACK = '"Segoe UI", Arial, sans-serif';

const LANGUAGE_FONTS: Record<string, string> = {
    ja: "Yu Gothic",
    zhhans: "Microsoft YaHei",
    zhhant: "Microsoft JhengHei",
    ko: "Malgun Gothic",
};

export function useLanguageFont(language: string | undefined) {
    useEffect(() => {
        const font = language ? LANGUAGE_FONTS[language] : undefined;
        document.body.style.fontFamily = font ? `"${font}", ${FALLBACK}` : FALLBACK;
    }, [language]);
}
