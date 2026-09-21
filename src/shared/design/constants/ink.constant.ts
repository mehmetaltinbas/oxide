/**
 * The comic-book pass: ink, halftone, and the weight of a line.
 *
 * The look is printed rather than painted. Every solid shape gets a dark
 * outline the way a panel is inked, flat colour sits inside it, and shading is
 * dots and hatching rather than gradients, because that is what a press could
 * actually do with one or two passes of ink.
 *
 * These are screen-space values: the pen cancels out the camera's transform, so
 * a width here is the width it lands at on the page however far the view is
 * zoomed. Heavier than lumber-wave's, because this camera sits further back and
 * a tree here is half the size one is there: the same pen came out spidery.
 */
export const INK = {
    /** The pen. Not pure black: a very dark green-black sits better on foliage. */
    line: '#14110d',
    /** Ordinary outline weight, for bodies, trunks, rock and built pieces. */
    width: 2.6,
    /** Heavier, for the things a reader's eye should land on first. */
    heavyWidth: 3.6,
    /** Lighter, for ground shapes that would otherwise fence the whole map. */
    groundWidth: 2.2,
    /** For line work inside a shape, which should never compete with its edge. */
    fineWidth: 1.5,
    /**
     * Interior marks: cracks, branches, grain, hatching.
     *
     * Bold, and always solid black. A comic has one ink and no grey: tone comes
     * from how many marks there are and how close together, never from how
     * pale they are. Marks drawn at half opacity read as pencil under the ink,
     * which is the opposite of the look.
     */
    markWidth: 2.2,
    /** A lighter interior mark, still solid black, for dense hatching. */
    hatchMarkWidth: 1.7,
    /** One stipple dot, for texture on stone and open ground. */
    stippleDot: 1.4,
    /**
     * Inside an item icon, the stroke width that separates a detail line from
     * a shape drawn as a line, in the icon's own unit square.
     *
     * Measured across the glyphs: every detail line (a pile's facets, a log's
     * end rings, stitching) is drawn at 0.028 to 0.04, and every shape drawn as
     * a line (an arrow's shaft, a bow's limb, a trap's jaws) at 0.05 or more.
     * 0.045 sits in the gap.
     */
    glyphDetail: 0.045,
    /**
     * The pen on the interface: gauges, slots, cards.
     *
     * Heavier than the world's, because UI sits on top of everything and a
     * 1.9 line that reads as confident on a tree reads as a hairline on a box.
     */
    uiWidth: 3.0,
    /** Dot grid for shaded areas, in world units. */
    halftoneSpacing: 9,
    halftoneDot: 0.55,
    halftoneColor: '#14110d',
    /** Diagonal hatching, for the darker shading a single dot grid cannot carry. */
    hatchSpacing: 9,
    hatchWidth: 1.8,
    hatchColor: '#14110d',
} as const;

/**
 * Grass, drawn as marks rather than as a texture.
 *
 * A few strokes per patch of open ground is what stops a flat green field
 * reading as a flat green field. Laid on a fixed grid and jittered from the
 * cell's own coordinates, so a tuft stays where it was put and the field does
 * not crawl when the camera moves.
 */
export const GRASS = {
    /** One tuft per cell of this size, in world units. */
    spacing: 46,
    /** Blades per tuft. */
    blades: 3,
    /** How tall a blade stands. */
    height: 11,
    width: 1.3,
    color: '#14110d',
} as const;
