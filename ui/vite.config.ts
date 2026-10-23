import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

// PrismaUI serves the view via file:// under Ultralight, so the page must be ONE
// self-contained JS/CSS bundle with relative paths (no shared chunks, no dynamic imports).
// The output goes straight to the mod layout under assets/ (deployed over the mod root by
// tools/deploy_assets.ps1).
export default defineConfig({
    plugins: [react()],
    base: "./",
    build: {
        outDir: "../assets/PrismaUI_F4/views/DiverseBodiesRedux",
        emptyOutDir: true,
        assetsDir: "assets",
        target: "es2019",
        cssCodeSplit: false,
        rollupOptions: {
            input: "index.html",
            output: {
                entryFileNames: "assets/app.js",
                assetFileNames: "assets/[name][extname]",
                inlineDynamicImports: true,
            },
        },
    },
});
