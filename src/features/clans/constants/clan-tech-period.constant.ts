/**
 * Clan progress runs on its own clock rather than the day counter. With an
 * hour-long day, keying these off "days survived" would mean no clan ever
 * teched up or came for you inside a normal session.
 */
export const CLAN_TECH_PERIOD = 420;
