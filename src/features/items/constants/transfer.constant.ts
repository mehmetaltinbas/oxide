/**
 * How long a right-click transfer takes.
 *
 * Moving a stack is work, not a thought: you are lifting things in and out of a
 * box, not clicking a spreadsheet. The cost scales with how much you are
 * shifting, so a single bandage is nearly instant and a full stack of stone is
 * a real pause you would not want to start with a bear behind you.
 *
 * Below about a third of a second the spinner never registers as a spinner, and
 * above about a second and a half emptying a crate becomes tedious rather than
 * deliberate.
 */
export const TRANSFER = {
    baseSeconds: 0.34,
    perItemSeconds: 0.004,
    maxSeconds: 1.4,
} as const;
