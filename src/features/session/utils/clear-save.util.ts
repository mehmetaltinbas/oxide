import { SAVE_KEY } from 'src/features/session/constants/save-key.constant';

export function clearSave(): void {
    try {
        localStorage.removeItem(SAVE_KEY);
    } catch {
        // ignore
    }
}
