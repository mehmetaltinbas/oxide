import { Game } from 'src/app/game';
import { SCENARIOS } from 'src/features/dev/scenarios';

/**
 * Put the game into a named scenario, and say so in the console.
 *
 * An unknown name lists what there is rather than failing quietly, because the
 * usual way to get one wrong is to half remember it.
 */
export function runScenario(game: Game, name: string): boolean {
    const scenario = SCENARIOS[name];
    if (!scenario) {
        console.warn(
            `No scenario called "${name}". There is: ${Object.keys(SCENARIOS).join(', ')}`,
        );
        return false;
    }
    scenario.set(game);
    console.info(`Scenario "${name}": ${scenario.what}`);
    return true;
}

/** Every scenario and what it sets up, for the console. */
export function listScenarios(): void {
    console.info('Scenarios: add ?scenario=<name> to the URL, or call oxide.scenario(name).');
    for (const [name, s] of Object.entries(SCENARIOS)) console.info(`  ${name}: ${s.what}`);
}
