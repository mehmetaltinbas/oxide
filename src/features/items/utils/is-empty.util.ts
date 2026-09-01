import { Container } from 'src/features/items/types/container.interface';

export function isEmpty(c: Container): boolean {
    return c.slots.every((s) => s === null);
}
