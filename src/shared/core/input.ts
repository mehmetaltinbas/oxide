export class Input {
    private down = new Set<string>();
    private pressedThisFrame = new Set<string>();
    private releasedThisFrame = new Set<string>();

    mouseX = 0;
    mouseY = 0;
    mouseDown = false;
    mouseClicked = false;
    mouseReleased = false;
    rightClicked = false;
    /** Right button held, for drawing a bow. */
    rightDown = false;
    wheel = 0;
    /**
     * Modifiers as they were at the moment of the click. Read from the mouse
     * event rather than from the key set, because ctrl-down never reaches the
     * key handler: the browser keeps its own ctrl shortcuts.
     */
    shiftClick = false;
    ctrlClick = false;
    /** Printable characters typed this frame, for the odd text field. */
    typed = '';
    /** Editing keys pressed this frame, in order: 'back', 'enter', 'escape'. */
    edits: ('back' | 'enter' | 'escape')[] = [];

    constructor(canvas: HTMLCanvasElement) {
        window.addEventListener('keydown', (e) => {
            // Let the browser keep its own shortcuts (reload, devtools, tab switching).
            if (e.metaKey || e.ctrlKey) return;
            const code = e.code;
            if (e.key.length === 1) this.typed += e.key;
            else if (e.key === 'Backspace') this.edits.push('back');
            else if (e.key === 'Enter') this.edits.push('enter');
            else if (e.key === 'Escape') this.edits.push('escape');
            if (!this.down.has(code)) this.pressedThisFrame.add(code);
            this.down.add(code);
            if (code === 'Space' || code.startsWith('Arrow') || code === 'Tab') e.preventDefault();
        });

        window.addEventListener('keyup', (e) => {
            this.down.delete(e.code);
            this.releasedThisFrame.add(e.code);
        });

        window.addEventListener('blur', () => {
            this.down.clear();
            this.mouseDown = false;
            this.rightDown = false;
        });

        canvas.addEventListener('mousemove', (e) => {
            const r = canvas.getBoundingClientRect();
            this.mouseX = e.clientX - r.left;
            this.mouseY = e.clientY - r.top;
        });

        canvas.addEventListener('mousedown', (e) => {
            e.preventDefault();
            if (e.button === 0) {
                this.mouseDown = true;
                this.mouseClicked = true;
                this.shiftClick = e.shiftKey;
                this.ctrlClick = e.ctrlKey || e.metaKey;
            } else if (e.button === 2) {
                this.rightClicked = true;
                this.rightDown = true;
            }
        });

        window.addEventListener('mouseup', (e) => {
            if (e.button === 0) {
                this.mouseDown = false;
                this.mouseReleased = true;
            } else if (e.button === 2) {
                this.rightDown = false;
            }
        });

        canvas.addEventListener('contextmenu', (e) => e.preventDefault());

        canvas.addEventListener(
            'wheel',
            (e) => {
                e.preventDefault();
                this.wheel += Math.sign(e.deltaY);
            },
            { passive: false },
        );
    }

    isDown(code: string): boolean {
        return this.down.has(code);
    }

    justPressed(code: string): boolean {
        return this.pressedThisFrame.has(code);
    }

    justReleased(code: string): boolean {
        return this.releasedThisFrame.has(code);
    }

    /** Normalized WASD / arrow-key movement vector. */
    moveAxis(): { x: number; y: number } {
        let x = 0;
        let y = 0;
        if (this.isDown('KeyA') || this.isDown('ArrowLeft')) x -= 1;
        if (this.isDown('KeyD') || this.isDown('ArrowRight')) x += 1;
        if (this.isDown('KeyW') || this.isDown('ArrowUp')) y -= 1;
        if (this.isDown('KeyS') || this.isDown('ArrowDown')) y += 1;
        const len = Math.hypot(x, y);
        if (len > 0) {
            x /= len;
            y /= len;
        }
        return { x, y };
    }

    /** Clear one-frame edges. Call at the very end of each frame. */
    endFrame(): void {
        this.pressedThisFrame.clear();
        this.releasedThisFrame.clear();
        this.mouseClicked = false;
        this.mouseReleased = false;
        this.shiftClick = false;
        this.ctrlClick = false;
        this.typed = '';
        this.edits.length = 0;
        this.rightClicked = false;
        this.wheel = 0;
    }
}
