// True while the settings page waits for the key to open the menu with: the global shortcuts
// (Esc, the menu key) must stay silent then, every key belongs to the picker.
let active = false;

export const setKeyCapture = (value: boolean) => {
    active = value;
};

export const isKeyCapture = () => active;
