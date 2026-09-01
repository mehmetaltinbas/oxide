import { Container } from 'src/features/items/types/container.interface';

export function makeContainer(size: number): Container {
    return { slots: new Array(size).fill(null) };
}
