import { SAVE_KEY } from 'src/features/session/constants/save-key.constant';

export function hasSave(): boolean {
    try {
        return localStorage.getItem(SAVE_KEY) !== null;
    } catch {
        return false;
    }
}
