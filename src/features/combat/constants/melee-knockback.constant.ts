/**
 * How hard a landed melee hit shoves what it hit.
 *
 * Nothing: a hit staggers no one here. Being knocked back turned every fight
 * into a shoving match, and against anything smaller than a bear it read as a
 * bug. An explosion still throws you, which is passed at its own call site.
 */
export const MELEE_KNOCKBACK = 0;
